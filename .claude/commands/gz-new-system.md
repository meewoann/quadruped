---
description: Scaffold a new gz-sim system plugin under src/systems/<name>/ with header, source, CMake glue, plugin registration, and an integration test stub.
argument-hint: "<snake_case_name> [CamelCaseClass]"
allowed-tools: ["Bash", "Read", "Write", "Edit"]
---

You're scaffolding a new system in `gz-sim`.

Arguments:
* First token of `$ARGUMENTS` → directory name in `snake_case` (required).
* Second token (optional) → class name in `CamelCase`. If missing, derive
  by converting the snake_case to CamelCase.

If `$ARGUMENTS` is empty, ask the user for the names before doing anything.

Steps:
1. Read the `new-system` skill end to end.
2. Verify `src/systems/<snake_case_name>/` does not already exist.
3. Create the directory plus `CamelCase.hh`, `CamelCase.cc`, and
   `CMakeLists.txt` using the templates in the skill.
4. Add `add_subdirectory(<snake_case_name>)` to `src/systems/CMakeLists.txt`,
   keeping the list alphabetical.
5. Create a stub `test/integration/<snake_case_name>_system.cc` and add the
   target to `test/integration/CMakeLists.txt`.
6. Run `pre-commit run --files <every file you touched>` and fix anything
   that comes back.
7. Report the list of new files and the build command the user should run.

Do **not** start `cmake --build` yourself unless the user asks — the
scaffold should compile, but they may want to inspect first.
