# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased

### Changed

- Begin using [clap.h](https://github.com/AuthentiCuber/clap) for CLI argument parsing.
- Make all functions and variables snake_case (were camelCase).

### Removed

- CLI flag aliases (clap doesn't support that yet).
