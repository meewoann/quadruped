---
description: Run ament linters (uncrustify, cpplint, flake8, pep257, xmllint, copyright) and pre-commit against changed files.
argument-hint: "[--all | <file paths>]"
allowed-tools: ["Bash"]
---

Lint the workspace.

Argument handling (`$ARGUMENTS`):
* Empty → only changed-vs-main:
  `pre-commit run --from-ref origin/main --to-ref HEAD`
* `--all` → `pre-commit run --all-files`
* Else treat as a space-separated file list:
  `pre-commit run --files $ARGUMENTS`

For ament-specific checks (when `pre-commit` is not configured for a
package), run the relevant tools directly:

| File kind | Command |
|-----------|---------|
| C++       | `ament_uncrustify --reformat <files>` then `ament_cpplint <files>` |
| Python    | `ament_flake8 <files>` then `ament_pep257 <files>` |
| XML / launch / package.xml | `ament_xmllint <files>` |
| Any source| `ament_copyright <files>` |

Rules:
* Never pass `--no-verify` or `--no-ament-copyright`. If a hook fails,
  fix the root cause.
* If a formatter auto-fixed a file, tell the user explicitly so they
  can stage the change.
* For C++ packages with a compile DB, also offer to run:
  `clang-tidy -p build/<pkg> <files>`.

End with a one-line summary: `"3 files, 0 issues, 2 auto-fixed"`.
