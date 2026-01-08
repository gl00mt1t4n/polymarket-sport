.PHONY: build run clean release

build:
	cargo build

release:
	cargo build --release

run: build
	cargo run

clean:
	cargo clean
