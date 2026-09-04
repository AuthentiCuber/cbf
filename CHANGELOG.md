# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased

### Added

- REPL can now specify input and output streams.
- `interpret_cmds` can now specify input and output streams.
- `run_bf` can now specify input, output and error streams.

### Changed

- Begin using `stdbool.h` for boolean values.
- Factored repl logic into its own function.
- New `collapse_result` enum for repeated token logic.
- Factored parsing logic into individual functions.
- Cleaner error handling with `parse_result` struct.
- Cleaner error handling with `run_result` struct.
- Moved core logic into `libcbf.c`, `libcbf.h`.
- Better debug info display.

### Fixed

- `#undef MAKE_ARRAY_TYPE` after use.
- Renamed parameters in `tokenise()`, `parse()` to make their purpose clearer.
- Post-increment that should have been a pre-incerement.
- Data pointer out of bounds not being caught because of negative `numtimes`.
- Off-by-one error in upper out of bounds check.
- Free tokens, token_types allocated in `run_bf`.

## [1.0.0] - 2026-07-22

### Changed

- Begin using [clap.h](https://github.com/AuthentiCuber/clap) 1.1.0 for CLI argument parsing.
- Make all functions and variables snake_case (were camelCase).
- Nomenclature change: `token_type` are now consistently referred to as such, `command`s are now `token`s.

### Fixed

- Remove unnecessary stddef header include.
