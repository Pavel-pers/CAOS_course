use std::{
    collections::BTreeSet,
    fs,
    io::{self, BufRead},
    ops::Range,
    path::{Path, PathBuf},
};

use annotate_snippets::{Level, Renderer, Snippet, renderer};
use anyhow::{Context, Result, anyhow, bail};
use clap::Parser;
use gix::{self, Repository};
use globset::{Glob, GlobSetBuilder};
use serde::{Deserialize, Serialize};
use walkdir::WalkDir;
use xshell::{Shell, cmd};

#[derive(Debug, Parser)]
pub struct TestArgs {}

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
    bin: PathBuf,
}

#[derive(Debug, Serialize, Deserialize)]
struct ForbiddenPatternsGroup {
    patterns: Vec<String>,
    hint: Option<String>,
}

#[derive(Debug, Serialize, Deserialize)]
struct ForbiddenPatterns {
    groups: Vec<ForbiddenPatternsGroup>,

    #[serde(flatten)]
    patterns: Option<ForbiddenPatternsGroup>,
}

impl ForbiddenPatterns {
    // No monads in rust =(
    fn for_each_group(&self, mut callback: impl FnMut(&ForbiddenPatternsGroup)) {
        if let Some(ref group) = self.patterns {
            callback(group);
        }

        for group in &self.groups {
            callback(group);
        }
    }
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(tag = "type", rename_all = "kebab-case")]
enum TestStep {
    CatchUnit(CatchUnit),
    ForbiddenPatterns(ForbiddenPatterns),
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
    // Relative to project_root
    task_path: PathBuf,
    config: TestConfig,
    shell: Shell,
}

struct ForbiddenStuff {
    span: Range<usize>,
    message: String,
    hint: Option<String>,
}

impl TestContext {
    fn new() -> Result<Self> {
        let repo = gix::discover(".")?;

        let cwd = fs::canonicalize(".")?;
        let repo_root = repo
            .workdir()
            .ok_or_else(|| anyhow!("Failed to get working directory"))?
            .canonicalize()?;
        let task = cwd.strip_prefix(&repo_root).with_context(|| {
            format!(
                "{} is not a prefix of {}",
                repo_root.display(),
                cwd.display(),
            )
        })?;

        let config_path = cwd.join(CONFIG_FILENAME);
        let config_file = fs::OpenOptions::new()
            .read(true)
            .open(&config_path)
            .with_context(|| format!("failed to open {}", config_path.display()))?;
        let config = serde_yml::from_reader(config_file)
            .with_context(|| format!("failed to parse {}", config_path.display()))?;

        let shell = Shell::new().with_context(|| format!("failed to create shell"))?;

        Ok(TestContext {
            repo,
            repo_root,
            task_path: task.to_owned(),
            config,
            shell,
        })
    }

    fn build_binary(&self, binary: &Path, profile: BuildProfile) -> Result<PathBuf> {
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
            .run()
            .with_context(|| "while running cmake")?;
        }

        let cpus_str = format!("{}", num_cpus::get());
        cmd!(
            self.shell,
            "cmake --build {build_dir} --target {binary} -j {cpus_str}"
        )
        .run()
        .with_context(|| "while building")?;

        Ok(build_dir.join(binary))
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
        }
    }

    fn run_unit(&self, cfg: &CatchUnit) -> Result<()> {
        for &profile in &cfg.profiles {
            let path = self.build_binary(&cfg.bin, profile)?;
            cmd!(self.shell, "{path}")
                .run()
                .with_context(|| format!("while running {}", path.display()))?;
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

    fn find_forbidden_stuff(
        &self,
        path: &Path,
        mut check_line: impl FnMut(&str) -> Vec<ForbiddenStuff>,
    ) -> Result<Vec<String>> {
        // Print one line above and below
        const ERROR_CTX: usize = 1;

        let file = fs::OpenOptions::new()
            .read(true)
            .open(path)
            .with_context(|| format!("failed to open {}", path.display()))?;

        let lines = io::BufReader::new(file)
            .lines()
            .collect::<Result<Vec<_>, _>>()?;

        let renderer =
            Renderer::styled().help(renderer::AnsiColor::BrightBlue.on_default().bold().into());
        let mut messages = vec![];

        for i in 0..lines.len() {
            let forbidden = check_line(&lines[i]);
            if forbidden.is_empty() {
                continue;
            }

            let first_line = i.saturating_sub(ERROR_CTX);
            let lines = &lines[first_line..(i + ERROR_CTX + 1).min(lines.len())];
            let target_line = i - first_line;
            let snippet = lines.join("\n");
            let line_offset = lines[..target_line]
                .iter()
                .map(|s| s.len() + 1)
                .sum::<usize>();

            for ForbiddenStuff {
                span,
                message,
                hint,
            } in forbidden
            {
                let local_path = path
                    .strip_prefix(&self.repo_root)?
                    .strip_prefix(&self.task_path)?
                    .to_str()
                    .ok_or_else(|| {
                        anyhow!("Couldn't convert {} to UTF-8 string", path.display())
                    })?;

                let span = (span.start + line_offset)..(span.end + line_offset);
                let snippet = {
                    let mut snip = Snippet::source(&snippet)
                        .line_start(first_line + 1)
                        .origin(local_path);
                    if let Some(ref hint) = hint {
                        snip = snip.annotation(Level::Help.span(span).label(hint));
                    }
                    snip
                };
                let msg = Level::Error.title(&message).snippet(snippet);
                messages.push(renderer.render(msg).to_string());
            }
        }

        Ok(messages)
    }

    fn check_forbidden_patterns_file(
        &self,
        path: &Path,
        cfg: &ForbiddenPatterns,
    ) -> Result<Vec<String>> {
        self.find_forbidden_stuff(path, |line| {
            let mut forbidden_stuff = vec![];
            cfg.for_each_group(|group| {
                for pattern in &group.patterns {
                    if let Some(idx) = line.find(pattern) {
                        forbidden_stuff.push(ForbiddenStuff {
                            span: idx..(idx + pattern.len()),
                            message: format!("Found forbidden pattern: {pattern}"),
                            hint: group.hint.clone(),
                        });
                    }
                }
            });

            forbidden_stuff
        })
    }

    fn check_forbidden_patterns(&self, cfg: &ForbiddenPatterns) -> Result<()> {
        const MAX_MESSAGES: usize = 20;

        let editable_files = self.find_editable_files()?;
        let mut messages = vec![];
        for path in &editable_files {
            let m = self.check_forbidden_patterns_file(path, cfg)?;
            messages.extend(m);
        }

        if messages.is_empty() {
            return Ok(());
        }

        let printed_messages = &messages[..messages.len().min(MAX_MESSAGES)];

        for m in printed_messages {
            anstream::println!("{m}");
        }

        if printed_messages.len() != messages.len() {
            anstream::println!(
                "Showing only first {MAX_MESSAGES} out of {}",
                messages.len()
            );
        }

        bail!("{} forbidden patters found in your files", messages.len());
    }
}

pub fn do_test(_args: TestArgs) -> Result<()> {
    TestContext::new()?.run()
}
