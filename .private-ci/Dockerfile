FROM rustlang/rust:nightly

RUN apt-get update && apt-get install -y \
    cmake clang-19 clang-format g++ && \
    apt-get clean

RUN rustup component add clippy rustfmt
