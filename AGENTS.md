# Repository Guidelines

## Project Structure & Module Organization
- `cpp/` — C++ core (CMake). Sources in `src/`, headers in `include/`, build helpers in `tools/`. External binaries and models live in `cpp/.source/` (not committed).
- `api/` — client bindings and demos for `python/`, `node.js/`, and `PowerShell/`.
- `build/` — local build outputs (e.g., `build/standard/bin/PaddleOCR-json`).
- `docs/` — usage and platform guides; start with `docs/Detailed-Usage-Guide.md`.
- `justfile` — unified build/dev tasks; prefer it over ad‑hoc scripts.

## Build, Test, and Development Commands
- Install tools: `just check-just` (or install per hint), optional: `trunk install`.
- Fetch deps: `just download` (idempotent; skips if cached in `cpp/.source`).
- Move stray downloads: `just tidy` (relocates root zips/dirs into `cpp/.source`).
- Build: `just build` (standard), `just build-static`, `just build-musl`, `just build-cross ARCH=aarch64`.
- Run binary: `just run -image_path=./api/python/test.jpg -models_path=./cpp/.source/models -config_path=./cpp/.source/models/config_chinese.txt`.
- Smoke tests: `just test` (verifies binary and C API artifacts).
- Docker: `just docker` then `just docker-test` for a quick OCR check.
- Clean builds only: `just clean` (preserves downloads). Purge cache: `just clean-downloads`.

## Coding Style & Naming Conventions
- Match existing C++ style in `cpp/src` and `include` (4‑space indent, self‑contained headers, clear namespaces). Filenames: `snake_case.cpp/.h`; classes: `PascalCase`; functions/vars: `lower_snake_case`.
- Python/Node docs and examples should be formatted. Use Trunk where available: `trunk check --fix` (markdownlint, black, isort, ruff, prettier, shellcheck, shfmt).

## Testing Guidelines
- Primary check: `just test` and manual runs with sample images.
- Node demo: `cd api/node.js/test && npm start` (uploads to localhost:3000).
- Python demos: `python api/python/demo1.py` (adjust paths as needed).
- Add reproducible inputs to `api/.../test` and keep assets small (<2MB).

## Commit & Pull Request Guidelines
- Commits: clear, imperative subject; scope tags like `feat:`, `fix:`, `docs:` preferred.
- PRs: include description, platforms tested (Linux/Windows), steps to verify, and before/after output where relevant. Link issues.
- Do not commit large archives or vendor artifacts. Exclude `cpp/.source/`, `build/`, `dist/`.
- Update related docs (`README.md`, `docs/`) when behavior or flags change.

## Security & Configuration Tips
- Avoid hardcoded absolute paths; prefer CLI flags and env (e.g., `LD_LIBRARY_PATH`).
- Do not commit model files or proprietary libs; use `just download` and `.source/`.
- Keep secrets out of code, configs, and commit history.
