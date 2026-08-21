# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased

### Added


### Changed

- `#undef MAKE_ARRAY_TYPE` after use.
- Renamed parameters in `tokenise()`, `parse()` to make their purpose clearer.
- Begin using `stdbool.h` for boolean values.
- Factored repl logic into its own function.
- Now using `collapse_result` enum for repeated token logic.

### Fixed


## [1.0.0] - 2026-07-22

### Changed

- Begin using [clap.h](https://github.com/AuthentiCuber/clap) 1.1.0 for CLI argument parsing.
- Make all functions and variables snake_case (were camelCase).
- Nomenclature change: `token_type` are now consistently referred to as such, `command`s are now `token`s.

### Fixed

- Remove unnecessary stddef header include.
