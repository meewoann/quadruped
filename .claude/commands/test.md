---
description: Run colcon tests for the workspace (or a single package) and report pass / fail counts.
argument-hint: "[package_name] [pytest -k pattern or gtest --gtest_filter]"
allowed-tools: ["Bash"]
---

Run tests in the current ROS 2 workspace.

Argument handling (`$ARGUMENTS`):
* Empty → `colcon test --event-handlers console_cohesion+`
* First token = package name →
  `colcon test --packages-select <pkg> --event-handlers console_direct+`
  followed by `colcon test-result --verbose --test-result-base build/<pkg>`.
* If a pytest filter (`-k ...`) or gtest filter is given as the second
  argument, pass it via `--pytest-args` / `--ctest-args` accordingly.

After every run:
1. `colcon test-result --all` and parse the summary.
2. Print a one-liner like
   `"148 passed, 0 failed, 2 skipped (12m13s) — see log/latest_test/"`.
3. On any failure, list the first 3 failing test IDs and offer to:
   * Re-run with `--event-handlers console_direct+` for full output.
   * Drop into pytest directly:
     `pytest src/<pkg>/test/test_foo.py -k 'name'`.
   * Drop into gtest binary directly:
     `./build/<pkg>/test_foo --gtest_filter='Bar.*' --gtest_repeat=5`.

Never claim "tests pass" without showing counts and `colcon test`'s
exit code. If linting / formatting tests were skipped, mention it.
