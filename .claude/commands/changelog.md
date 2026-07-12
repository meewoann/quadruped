---
description: Summarize commits on the current branch (vs origin/main) into a CHANGELOG.rst-style block ready to paste into a package's CHANGELOG.
argument-hint: "[base-ref, default origin/main] [package_name]"
allowed-tools: ["Bash"]
---

Produce a CHANGELOG-friendly summary for the current branch.

1. Determine the base ref: first token of `$ARGUMENTS` if provided,
   else `origin/main`.
2. Optional second token = package name → restrict commits to those
   touching `src/<package>/`:
   `git log --oneline <base>..HEAD -- src/<package>`
3. Group commits into the standard ROS 2 CHANGELOG headings:
   * **Features** — new public behaviour.
   * **Bug Fixes** — references to a bug or regression.
   * **Refactor / Cleanup** — no behaviour change.
   * **Tests** — only test files changed.
   * **CI / Build** — only `.github/`, `CMakeLists.txt`, `package.xml`,
     `setup.py`, etc.
   * **Documentation** — `.md`, `.rst`, doxygen.
4. Emit a block in the existing CHANGELOG.rst style:

   ```rst
   X.Y.Z (YYYY-MM-DD)
   ------------------
   * Short imperative summary (#PR)
   * Another summary (#PR)
   * Contributors: Name1, Name2
   ```

5. Print the block — do **not** edit any CHANGELOG.rst automatically.
   Ask whether to insert it under the unreleased / upcoming section of
   the chosen package.
6. Flag any commits that look like they require a migration note
   (API removals, default-value changes, parameter renames, plugin
   renames).
