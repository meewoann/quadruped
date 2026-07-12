---
description: Scaffold a new ECS component header under include/gz/sim/components/ and wire it into the install list.
argument-hint: "<CamelCaseComponentName> [underlying-cpp-type]"
allowed-tools: ["Bash", "Read", "Write", "Edit"]
---

Scaffold a new component in `gz-sim`.

Arguments:
* First token of `$ARGUMENTS` → `CamelCase` component name (required).
* Second token (optional) → underlying C++ type (default: `double`).

If `$ARGUMENTS` is empty, ask the user before doing anything.

Steps:
1. Read the `new-component` skill end to end.
2. Verify `include/gz/sim/components/<CamelCase>.hh` does not already
   exist.
3. Write the header using the no-custom-serializer template from the
   skill (or ask the user if a `msgs` type is needed).
4. Add the new header alphabetically to
   `include/gz/sim/components/CMakeLists.txt`.
5. If the component is part of the public API, append a one-line entry
   under the next-release section of `Migration.md`.
6. Run `pre-commit run --files <every file you touched>`.
7. Report new files + suggest a unit test if behaviour was added.

Do not register a custom serializer unless explicitly asked — most
components don't need one.
