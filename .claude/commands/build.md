---
description: Build the colcon workspace (optionally a single package) and report the outcome.
argument-hint: "[package_name] [extra colcon args]"
allowed-tools: ["Bash"]
---

Build the current ROS 2 workspace from its root (the directory that
contains `src/`, `install/`, `build/`, `log/` — if you are in a package
subdir, `cd` up until you find that root).

Argument handling (`$ARGUMENTS`):
* If empty:
  `colcon build --symlink-install --event-handlers console_cohesion+`
* If the first token does **not** start with `-`, treat it as a package
  name:
  `colcon build --symlink-install --packages-up-to <pkg> $REST`
* If the first token starts with `-`, pass everything through verbatim.

Always:
* Use `--symlink-install` unless the user passes `--no-symlink`.
* Use `RelWithDebInfo` for CMake packages unless overridden:
  `--cmake-args ' -DCMAKE_BUILD_TYPE=RelWithDebInfo'`.
* On failure, show the last ~40 lines of the failing package's
  `log/latest_build/<pkg>/stderr.log` so the user can see the real
  error.
* On success, remind the user to source the overlay:
  `source install/setup.bash`.

Do **not** wipe `build/`, `install/`, `log/` automatically. If a
configure-time problem looks stale, ask whether to clean first.
