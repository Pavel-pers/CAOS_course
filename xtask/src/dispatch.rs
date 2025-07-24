use crate::compose::{ComposeArgs, do_compose};
use crate::test::{TestArgs, do_test};
use anyhow::Result;
use clap::Parser;

#[derive(Debug, Parser)]
pub enum Command {
    Compose(ComposeArgs),
    Test(TestArgs),
}

pub fn execute(cmd: Command) -> Result<()> {
    match cmd {
        Command::Compose(args) => do_compose(args),
        Command::Test(args) => do_test(args),
    }
}
