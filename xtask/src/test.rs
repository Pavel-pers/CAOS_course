use std::{
    borrow::Cow,
    collections::HashMap,
    ffi::{OsStr, OsString},
    fs, io,
    path::{Path, PathBuf},
    process,
    rc::Rc,
};

use crate::task::{
    BuildProfile, ForbiddenPatterns, ForbiddenPatternsGroup, RunCmd, TaskContext, TestStep,
};
use crate::util::PathExt;
use annotate_snippets::{Level, Renderer, Snippet, renderer};
use anyhow::{Context, Result, bail};
use clap::Parser;
use gix::{self, Repository};
use itertools::Itertools;
use regex::Regex;
use serde_json::Value as Json;
use strum::IntoEnumIterator;
use tracing::{info, warn};

#[derive(Debug, Parser)]
pub struct TestArgs {
    /// Only build necessary binaries without running any tests
    #[arg(long)]
    build_only: bool,
}

struct ProfileRepresentations {
    cmake_build_type: &'static str,
    build_dir: &'static str,
    short: &'static str,
}

impl ProfileRepresentations {
    fn new(cmake_build_type: &'static str, build_dir: &'static str, short: &'static str) -> Self {
        ProfileRepresentations {
            cmake_build_type,
            build_dir,
            short,
        }
    }
}

impl BuildProfile {
    fn representations(self) -> ProfileRepresentations {
        use BuildProfile::*;
        match self {
            Release => ProfileRepresentations::new("Release", "build_release", "rel"),
            Debug => ProfileRepresentations::new("Debug", "build_debug", "dbg"),
            ASan => ProfileRepresentations::new("Asan", "build_asan", "asan"),
            TSan => ProfileRepresentations::new("Tsan", "build_tsan", "tsan"),
        }
    }

    fn cmake_build_type(self) -> &'static str {
        self.representations().cmake_build_type
    }

    fn build_dir(self) -> &'static str {
        self.representations().build_dir
    }

    fn short(self) -> &'static str {
        self.representations().short
    }
}

impl ForbiddenPatternsGroup {
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

#[derive(Debug)]
struct RepoConfig {
    tools: HashMap<&'static str, &'static str>,
    repo_root: Rc<Path>,
}

impl RepoConfig {
    fn new(repo_root: Rc<Path>) -> Self {
        Self {
            tools: HashMap::from_iter([
                ("nej-runner", "common/tools/test.py"),
                ("clang-fmt-runner", "common/tools/run-clang-format.py"),
            ]),
            repo_root,
        }
    }

    fn get_tool(&self, name: &str) -> Result<&'static str> {
        self.tools
            .get(name)
            .copied()
            .with_context(|| format!("tool {name} not found"))
    }

    fn get_tool_path(&self, name: &str) -> Result<PathBuf> {
        Ok(self.repo_root.join(self.get_tool(name)?))
    }
}

#[derive(Debug)]
struct TestContext {
    #[allow(dead_code)]
    repo: Repository,
    repo_root: Rc<Path>,
    repo_config: RepoConfig,
    task_context: TaskContext,
    args: TestArgs,
}

impl TestContext {
    fn new(args: TestArgs) -> Result<Self> {
        let repo = gix::discover(".")?;

        let task_abs_path = fs::canonicalize(".")?;
        let repo_root = repo
            .workdir()
            .context("failed to get working directory")?
            .canonicalize()?;
        let repo_root = Rc::<Path>::from(repo_root);

        let task_context = TaskContext::new(&task_abs_path, Rc::clone(&repo_root))?;

        Ok(TestContext {
            repo,
            repo_config: RepoConfig::new(Rc::clone(&repo_root)),
            repo_root,
            task_context,
            args,
        })
    }

    fn run_cmake(
        &self,
        build_dir: &Path,
        profile: BuildProfile,
        fresh: bool,
    ) -> io::Result<process::ExitStatus> {
        let mut cmd = process::Command::new("cmake");
        cmd.arg("-B")
            .arg(build_dir)
            .arg("-S")
            .arg(self.repo_root.as_ref())
            .arg(format!("-DCMAKE_BUILD_TYPE={}", profile.cmake_build_type()));
        if fresh {
            cmd.arg("--fresh");
        }
        cmd.status()
    }

    fn build_target(&self, target: &str, profile: BuildProfile) -> Result<PathBuf> {
        let build_dir = self.repo_root.join(profile.build_dir());
        if !build_dir.exists() {
            fs::create_dir(&build_dir)
                .with_context(|| format!("failed to create {}", build_dir.display()))?;
            self.run_cmake(&build_dir, profile, false)?;
        }

        let cpus_str = format!("{}", num_cpus::get());
        let build = || {
            process::Command::new("cmake")
                .arg("--build")
                .arg(&build_dir)
                .arg("--target")
                .arg(target)
                .arg("-j")
                .arg(&cpus_str)
                .status()
        };

        let target_path = build_dir.join(target);

        let status = build()?;
        match status.exit_ok() {
            Ok(()) => return Ok(target_path),
            Err(e) => warn!("Failed to build target: {e}. Going to re-run cmake"),
        }

        self.run_cmake(&build_dir, profile, true)?;

        build()?.exit_ok().context("running cmake")?;

        Ok(target_path)
    }

    fn run(self) -> Result<()> {
        for step in &self.task_context.test_config.tests {
            self.run_step(step)?;
        }
        self.check_format()?;
        Ok(())
    }

    #[allow(clippy::type_complexity)]
    fn cmd_arg_processors<'a>(
        &'a self,
    ) -> HashMap<
        Cow<'static, str>,
        Box<dyn Fn(&TestContext, BuildProfile, &str) -> Result<OsString> + 'a>,
    > {
        let mut processors = HashMap::<
            Cow<'static, str>,
            Box<dyn Fn(&TestContext, BuildProfile, &str) -> Result<OsString>>,
        >::new();

        let find_tool = {
            |_: &TestContext, _: BuildProfile, arg: &str| {
                Ok(self.repo_config.get_tool_path(arg)?.into())
            }
        };
        processors.insert(Cow::from("tool"), Box::new(find_tool));

        let local = |ctx: &TestContext, _: BuildProfile, arg: &str| -> Result<OsString> {
            Ok(ctx.task_context.path_of(arg).into())
        };
        processors.insert(Cow::from("local"), Box::new(local));

        let build = |ctx: &TestContext, profile: BuildProfile, target: &str| {
            ctx.build_target(target, profile).map(Into::into)
        };
        processors.insert(Cow::from("build"), Box::new(build));

        for profile in BuildProfile::iter() {
            let build_profile = move |ctx: &TestContext, _: BuildProfile, target: &str| {
                ctx.build_target(target, profile).map(Into::into)
            };
            processors.insert(
                Cow::from(format!("build({})", profile.short())),
                Box::new(build_profile),
            );
        }

        processors
    }

    fn run_cmd_profile(&self, cfg: &RunCmd, profile: BuildProfile) -> Result<()> {
        let process_cmd_arg = {
            let processors = self.cmd_arg_processors();

            let f: impl for<'a> Fn(&'a str) -> Result<Cow<'a, OsStr>> = move |arg: &str| {
                let pos = if let Some(pos) = arg.find(':') {
                    pos
                } else {
                    return Ok(Cow::from(OsStr::new(arg)));
                };

                let processor = &arg[..pos];
                let arg = &arg[pos + 1..];

                let processor = processors
                    .get(processor)
                    .with_context(|| format!("processor {} not found", processor))?;
                processor(self, profile, arg).map(Cow::from)
            };
            f
        };

        let args = cfg
            .cmd
            .iter()
            .map(|p| process_cmd_arg(p))
            .collect::<Result<Vec<_>, _>>()?;

        if self.args.build_only {
            return Ok(());
        }

        if cfg.echo {
            info!("Running {}", args.iter().map(|s| s.display()).join(" "));
        }

        let bin = args.first().context("no cmd was given")?;

        let mut cmd = process::Command::new(bin);
        for arg in &args[1..] {
            cmd.arg(arg);
        }

        cmd.status()?.exit_ok()?;

        Ok(())
    }

    fn run_cmd(&self, cfg: &RunCmd) -> Result<()> {
        for &profile in &cfg.profiles {
            self.run_cmd_profile(cfg, profile)?;
        }
        Ok(())
    }

    fn run_step(&self, step: &TestStep) -> Result<()> {
        use TestStep::*;

        match step {
            ForbiddenPatterns(cfg) => self.check_forbidden_patterns(cfg),
            RunCmd(cfg) => self.run_cmd(cfg),
        }
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

                let local_path = self
                    .task_context
                    .strip_task_root_logged(path)?
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

        let editable_files = self.task_context.find_editable_files()?;
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
            warn!(
                "Showing only first {MAX_MESSAGES} out of {}",
                messages_count
            );
        }

        bail!("{} forbidden patters found in your files", messages_count);
    }

    fn major_clang_format_version() -> Result<u8> {
        let out = process::Command::new("clang-format")
            .arg("--version")
            .output()
            .map_err(anyhow::Error::from)
            .and_then(|o| o.exit_ok().map_err(From::from))
            .context("running clang-format")?;

        let out = String::try_from(out.stdout).context("parsing clang-format output")?;

        let re = Regex::new(r"\d+")?;
        let major = re
            .find(&out)
            .context("no version in clang-format output")?
            .as_str()
            .parse()?;
        Ok(major)
    }

    fn check_format(&self) -> Result<()> {
        const MINIMAL_SUPPORTED: u8 = 11;
        const DESIRED: u8 = 15;

        let solution_files = self.task_context.find_editable_files()?;
        if solution_files.is_empty() {
            warn!("No solution files were found. Skipping formatting step");
            return Ok(());
        }

        let version = Self::major_clang_format_version()?;
        let mut use_old = false;
        if version < MINIMAL_SUPPORTED {
            bail!(
                "Your version of clang-format ({}) is less than minimal supported ({}). Consider upgrading it",
                version,
                MINIMAL_SUPPORTED
            );
        }
        if version < DESIRED {
            warn!(
                "Your version of clang-format ({}) is less than desired ({}). Going to use convervative config. Consider upgrading clang-format",
                version, DESIRED
            );
            use_old = true;
        }

        let fmt_runner = self.repo_config.get_tool_path("clang-fmt-runner")?;

        let mut cmd = process::Command::new("python3");
        cmd.arg(fmt_runner);

        if use_old {
            let old_config_path = self.repo_root.join(".clang-format-11");
            let f = fs::File::open(&old_config_path)
                .with_context(|| format!("failed to open {}", old_config_path.display()))?;
            let f = io::BufReader::new(f);
            let cfg: Json = serde_yml::from_reader(f)
                .with_context(|| format!("failed to parse {}", old_config_path.display()))?;
            let cfg = serde_json::to_string(&cfg)?;

            cmd.arg("--style").arg(cfg);
        } else {
            cmd.arg(format!(
                "--style=file:{}/.clang-format",
                self.repo_root.display()
            ));
        }

        cmd.args(solution_files);

        cmd.status()?.exit_ok()?;

        Ok(())
    }
}

pub fn do_test(args: TestArgs) -> Result<()> {
    TestContext::new(args)?.run()
}
