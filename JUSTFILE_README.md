# PaddleOCR-json Justfile

This repository uses [Just](https://github.com/casey/just) as a command runner to orchestrate builds and development tasks.

## Installation

First, install Just:

```bash
# Option 1: Install script
curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash -s -- --to ~/.local/bin

# Option 2: Cargo (if you have Rust)
cargo install just

# Option 3: Package managers
snap install just          # Ubuntu/Snap
brew install just          # macOS/Homebrew
```

Add `~/.local/bin` to your PATH if using option 1.

## Quick Start

```bash
# Check installation
just check-just

# Setup development environment
just dev-setup

# Build standard version
just build

# Test the build
just test

# Build all variants
just build-all
```

## Available Commands

### Setup & Dependencies

- `just setup` - Install system dependencies
- `just download` - Download required libraries and models
- `just dev-setup` - Complete development environment setup

### Building

- `just build` - Build standard shared library version
- `just build-static` - Build static binary
- `just build-musl` - Build musl static binary
- `just build-cross ARCH` - Build for specific architecture (e.g., `aarch64`)
- `just build-all` - Build all variants

### Testing & Quality

- `just test` - Run basic tests
- `just c-api-test` - Test C API compilation
- `just benchmark` - Compare build variants

### Docker

- `just docker` - Build Docker image
- `just docker-build MODE` - Build Docker image with specific mode
- `just docker-run` - Run Docker container
- `just docker-test` - Run container with test image

### Distribution

- `just package` - Create distribution packages
- `just install` - Install to system

### Utilities

- `just status` - Show build status
- `just clean` - Clean build artifacts
- `just help` - Show all available commands

## Examples

```bash
# Complete workflow
just dev-setup
just build-all
just test
just package

# Cross-compilation for ARM
just build-cross aarch64

# Docker development
just docker-build musl
just docker-test

# CI/CD pipeline
just ci
```

## Build Variants

| Command | Binary Size | Dependencies | Use Case |
|---------|-------------|--------------|----------|
| `just build` | ~50MB | Many | Development, system integration |
| `just build-static` | ~150MB | None | Deployment, containers |
| `just build-musl` | ~25MB | None | Embedded, minimal environments |
| `just build-cross` | Varies | None | ARM devices, different architectures |

## Configuration

The justfile uses these variables:

- `CPP_DIR`: C++ source directory (default: "cpp")
- `SOURCE_DIR`: Dependencies directory (default: "cpp/.source")
- `BUILD_DIR`: Build output directory (default: "build")

## Integration

The justfile integrates with:

- CMake build system
- Docker for containerization
- Cross-compilation toolchains
- C API testing
- Distribution packaging

For more details, see `cpp/BUILD_IMPROVEMENTS.md`.
