use anyhow::Result;
use clap::Parser;
use xtask::dispatch::{Command, execute};

#[derive(Debug, Parser)]
#[command(author, version, about, long_about = None)]
struct Args {
    #[command(subcommand)]
    cmd: Command,
}

fn main() -> Result<()> {
    let args = Args::parse();
    execute(args.cmd)
}
