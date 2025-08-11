use std::{
    collections::BTreeSet,
    fs,
    path::{Path, PathBuf},
};

use annotate_snippets::{Level, Renderer, Snippet, renderer};
use anyhow::{Context, Result, anyhow, bail};
use clap::Parser;
use gix::{self, Repository};
use globset::{Glob, GlobSetBuilder};
use itertools::Itertools;
use regex::Regex;
use serde::{Deserialize, Serialize};
use walkdir::WalkDir;
use xshell::{Shell, cmd};

#[derive(Debug, Parser)]
pub struct TestArgs {
    /// Only build necessary binaries without running any tests
    #[arg(long)]
    build_only: bool,
}

const CONFIG_FILENAME: &str = "testing.yaml";

#[derive(Debug, Serialize, Deserialize, Clone, Copy)]
#[serde(rename_all = "lowercase")]
enum BuildProfile {
    Release,
    Debug,
    ASan,
    TSan,
}

impl BuildProfile {
    fn to_cmake_build_type(self) -> &'static str {
        use BuildProfile::*;
        match self {
            Release => "Release",
            Debug => "Debug",
            ASan => "Asan",
            TSan => "Tsan",
        }
    }

    fn to_build_dir(self) -> &'static str {
        use BuildProfile::*;
        match self {
            Release => "build_release",
            Debug => "build_debug",
            ASan => "build_asan",
            TSan => "build_tsan",
        }
    }
}

#[derive(Debug, Serialize, Deserialize)]
struct CatchUnit {
    profiles: Vec<BuildProfile>,

    bin: String,
}

#[derive(Debug, Serialize, Deserialize)]
struct ForbiddenPatternsGroup {
    #[serde(default)]
    substring: Vec<String>,

    #[serde(default)]
    regex: Vec<String>,

    #[serde(default)]
    token: Vec<String>,

    hint: Option<String>,
}

impl ForbiddenPatternsGroup {
    fn is_empty(&self) -> bool {
        [&self.substring, &self.regex, &self.token]
            .into_iter()
            .all(Vec::is_empty)
    }

    fn build_regex(&self) -> Result<Regex> {
        let substrings = self
            .substring
            .iter()
            .map(|lit| format!("({})", regex::escape(lit)));

        let tokens = self.token.iter().map(|lit| {
            format!(
                "(?:^|[^[:alnum:]])({})(?:[^[:alnum:]]|$)",
                regex::escape(lit)
            )
        });

        let regexes = self.regex.iter().map(Clone::clone);

        let pattern = substrings.chain(tokens).chain(regexes).join("|");

        let pattern = format!("(?ms){pattern}");

        Regex::new(&pattern).with_context(|| format!("compiling regex pattern: {pattern}"))
    }
}

#[derive(Debug, Serialize, Deserialize)]
struct ForbiddenPatterns {
    #[serde(default)]
    groups: Vec<ForbiddenPatternsGroup>,

    #[serde(flatten)]
    patterns: Option<ForbiddenPatternsGroup>,
}

impl ForbiddenPatterns {
    fn groups(&self) -> impl Iterator<Item = &ForbiddenPatternsGroup> {
        self.patterns
            .iter()
            .chain(&self.groups)
            .filter(|g| !g.is_empty())
    }
}

const fn default_retests_count() -> u16 {
    1
}

const fn default_retries_count() -> u16 {
    1
}

#[derive(Debug, Serialize, Deserialize)]
struct NejudgeParams {
    #[serde(default)]
    extra_args: Vec<String>,

    #[serde(default = "default_retests_count")]
    retests_count: u16,

    #[serde(default = "default_retries_count")]
    retries_count: u16,

    #[serde(default)]
    checker: Option<String>,

    #[serde(default)]
    interactor: Option<String>,

    solution: String,

    profiles: Vec<BuildProfile>,
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(tag = "type", rename_all = "kebab-case")]
enum TestStep {
    CatchUnit(CatchUnit),
    ForbiddenPatterns(ForbiddenPatterns),
    Nejudge(NejudgeParams),
}

#[derive(Debug, Serialize, Deserialize)]
struct TestConfig {
    tests: Vec<TestStep>,
    editable: Vec<String>,
}

#[derive(Debug)]
struct TestContext {
    #[allow(dead_code)]
    repo: Repository,
    repo_root: PathBuf,
    /// Relative to project_root
    task_path: PathBuf,
    config: TestConfig,
    args: TestArgs,
    shell: Shell,
}

trait PathExt {
    fn to_str_logged(&self) -> Result<&str>;

    fn strip_prefix_logged<P: AsRef<Path>>(&self, prefix: P) -> Result<&Path>;
}

impl PathExt for Path {
    fn to_str_logged(&self) -> Result<&str> {
        self.to_str()
            .ok_or_else(|| anyhow!("couldn't convert {} to UTF-8 string", self.display()))
    }

    fn strip_prefix_logged<P: AsRef<Path>>(&self, prefix: P) -> Result<&Path> {
        self.strip_prefix(&prefix).with_context(|| {
            format!(
                "{} is not a prefix of {}",
                self.display(),
                prefix.as_ref().display(),
            )
        })
    }
}

impl TestContext {
    fn new(args: TestArgs) -> Result<Self> {
        let repo = gix::discover(".")?;

        let cwd = fs::canonicalize(".")?;
        let repo_root = repo
            .workdir()
            .ok_or_else(|| anyhow!("Failed to get working directory"))?
            .canonicalize()?;
        let task = cwd.strip_prefix_logged(&repo_root)?;

        let config_path = cwd.join(CONFIG_FILENAME);
        let config_file = fs::OpenOptions::new()
            .read(true)
            .open(&config_path)
            .with_context(|| format!("failed to open {}", config_path.display()))?;
        let config = serde_yml::from_reader(config_file)
            .with_context(|| format!("failed to parse {}", config_path.display()))?;

        let shell = Shell::new().context("failed to create shell")?;

        Ok(TestContext {
            repo,
            repo_root,
            task_path: task.to_owned(),
            config,
            args,
            shell,
        })
    }

    fn build_target(&self, target: &str, profile: BuildProfile) -> Result<PathBuf> {
        let build_dir = self.repo_root.join(profile.to_build_dir());
        if !build_dir.exists() {
            fs::create_dir(&build_dir)
                .with_context(|| format!("failed to create {}", build_dir.display()))?;
            let repo_path = &self.repo_root;
            let cmake_build_type = profile.to_cmake_build_type();
            cmd!(
                self.shell,
                "cmake -B {build_dir} -S {repo_path} -DCMAKE_BUILD_TYPE={cmake_build_type}"
            )
            .run()?;
        }

        let cpus_str = format!("{}", num_cpus::get());
        cmd!(
            self.shell,
            "cmake --build {build_dir} --target {target} -j {cpus_str}"
        )
        .run()?;

        Ok(build_dir.join(target))
    }

    fn nejudge_runner_path(&self) -> PathBuf {
        let mut path = self.repo_root.clone();
        // TODO: Get this value from global config
        // or get rid of this python script entirely
        path.push("common/tools/test.py");
        path
    }

    fn build_nejudge_infra(&self, target: &str) -> Result<PathBuf> {
        if let Some(s) = target.strip_prefix("std:") {
            return Ok(PathBuf::from(s));
        }
        if let Some(s) = target.strip_prefix("bin:") {
            return self.build_target(s, BuildProfile::Release);
        }
        // TODO: bail?
        Ok(PathBuf::from(target))
    }

    fn run_nejudge_profile(&self, cfg: &NejudgeParams, profile: BuildProfile) -> Result<()> {
        let interactor_path = cfg
            .interactor
            .as_ref()
            .map(|path| self.build_nejudge_infra(path))
            .transpose()?;
        let checker_path = cfg
            .checker
            .as_ref()
            .map(|checker| self.build_nejudge_infra(checker))
            .transpose()?;
        let solution_path = self.build_target(&cfg.solution, profile)?;

        if self.args.build_only {
            return Ok(());
        }

        let runner_path = self.nejudge_runner_path();
        let mut cmd = vec![runner_path.to_str_logged()?];

        for (flag, value) in [
            ("--checker", &checker_path),
            ("--interactor", &interactor_path),
        ] {
            let path = match value {
                Some(v) => v,
                None => continue,
            };
            cmd.extend([flag, path.to_str_logged()?]);
        }

        let retests_count = format!("{}", cfg.retests_count);
        cmd.extend(["--retests-count", &retests_count]);

        let retries_count = format!("{}", cfg.retries_count);
        cmd.extend(["--retries-count", &retries_count]);

        cmd.extend(["--run-cmd", solution_path.to_str_logged()?]);

        cmd!(self.shell, "python3 {cmd...}").run()?;

        Ok(())
    }

    fn run_nejudge(&self, cfg: &NejudgeParams) -> Result<()> {
        for &profile in &cfg.profiles {
            self.run_nejudge_profile(cfg, profile)?;
        }
        Ok(())
    }

    fn run(self) -> Result<()> {
        for step in &self.config.tests {
            self.run_step(step)?;
        }
        Ok(())
    }

    fn run_step(&self, step: &TestStep) -> Result<()> {
        use TestStep::*;

        match step {
            CatchUnit(cfg) => self.run_unit(cfg),
            ForbiddenPatterns(cfg) => self.check_forbidden_patterns(cfg),
            Nejudge(cfg) => self.run_nejudge(cfg),
        }
    }

    fn run_unit(&self, cfg: &CatchUnit) -> Result<()> {
        for &profile in &cfg.profiles {
            let path = self.build_target(&cfg.bin, profile)?;
            if self.args.build_only {
                continue;
            }
            cmd!(self.shell, "{path}").run()?;
        }

        Ok(())
    }

    fn find_editable_files(&self) -> Result<BTreeSet<PathBuf>> {
        let task_path = self.repo_root.join(&self.task_path);
        let is_editable = {
            let mut builder = GlobSetBuilder::new();
            for path in &self.config.editable {
                builder.add(
                    Glob::new(path)
                        .with_context(|| format!("creating glob pattern from \"{path}\""))?,
                );
            }
            builder.build().context("creating globset")?
        };

        let mut editable = BTreeSet::new();

        for item in WalkDir::new(&task_path).into_iter().filter_entry(|entry| {
            entry.file_type().is_dir()
                || entry
                    .path()
                    .strip_prefix(&task_path)
                    .map(|p| is_editable.is_match(p))
                    .unwrap_or(false)
        }) {
            let item = item?;
            if item.file_type().is_dir() {
                continue;
            }
            editable.insert(item.path().to_path_buf());
        }
        Ok(editable)
    }

    fn check_forbidden_patterns_file<'a>(
        &self,
        cfg: &'a ForbiddenPatterns,
        path: &'a Path,
        text: &'a str,
    ) -> Result<Vec<annotate_snippets::Message<'a>>> {
        const ERROR_CTX: usize = 1;

        let line_starts = text.lines().scan(0, |state, line| {
            *state += line.len() + 1;
            Some(*state)
        });
        let line_starts = std::iter::once(0).chain(line_starts).collect::<Vec<_>>();

        let line_count = line_starts.len() - 1;

        let line_index_for_byte_offset = |offset: usize| -> usize {
            match line_starts.binary_search(&offset) {
                Ok(i) => i,
                Err(i) => i - 1, // Should never be 0
            }
        };

        let mut messages = vec![];

        for group in cfg.groups() {
            let re = group.build_regex()?;

            for caps in re.captures_iter(text) {
                let target = caps
                    .iter()
                    .flatten()
                    .last()
                    .expect("regex match has at least one group");

                let span_start = target.start();
                let span_end = target.end();

                let start_line = line_index_for_byte_offset(span_start);
                let end_line = line_index_for_byte_offset(span_end - 1);

                let first_line = start_line.saturating_sub(ERROR_CTX);
                let last_line = (end_line + ERROR_CTX + 1).min(line_count);

                let snippet_start = line_starts[first_line];
                let snippet_end = line_starts[last_line];
                let snippet_str = &text[snippet_start..snippet_end];

                let local_start = span_start - snippet_start;
                let local_end = span_end - snippet_start;
                let local_span = local_start..local_end;

                let local_path = path
                    .strip_prefix_logged(&self.repo_root)?
                    .strip_prefix_logged(&self.task_path)?
                    .to_str_logged()?;

                let mut snip = Snippet::source(snippet_str)
                    .line_start(first_line + 1)
                    .origin(local_path)
                    .annotation(Level::Error.span(local_span.clone()).label(""));
                if let Some(ref hint) = group.hint {
                    snip = snip.annotation(Level::Help.span(local_span).label(hint));
                }

                let msg = Level::Error.title("Found forbidden pattern").snippet(snip);
                messages.push(msg);
            }
        }

        Ok(messages)
    }

    fn check_forbidden_patterns(&self, cfg: &ForbiddenPatterns) -> Result<()> {
        const MAX_MESSAGES: usize = 20;

        let editable_files = self.find_editable_files()?;
        let mut messages = vec![];
        let editable_files_content = editable_files
            .iter()
            .map(|path| -> Result<_> {
                Ok((
                    path,
                    fs::read_to_string(path)
                        .with_context(|| format!("failed to read {}", path.display()))?,
                ))
            })
            .collect::<Result<Vec<_>>>()?;

        for (path, content) in &editable_files_content {
            let m = self.check_forbidden_patterns_file(cfg, path, content)?;
            messages.extend(m);
        }

        if messages.is_empty() {
            return Ok(());
        }

        let messages_count = messages.len();
        let renderer = Renderer::styled().help(renderer::AnsiColor::BrightBlue.on_default().bold());

        for m in messages.into_iter().take(MAX_MESSAGES) {
            anstream::println!("{}", renderer.render(m));
        }

        if MAX_MESSAGES < messages_count {
            anstream::println!(
                "NOTE: Showing only first {MAX_MESSAGES} out of {}",
                messages_count
            );
        }

        bail!("{} forbidden patters found in your files", messages_count);
    }
}

pub fn do_test(args: TestArgs) -> Result<()> {
    TestContext::new(args)?.run()
}
