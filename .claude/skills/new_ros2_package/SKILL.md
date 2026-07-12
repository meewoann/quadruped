---
name: new_ros2_package
description: Scaffold a new ROS 2 package (ament_python or ament_cmake) inside a colcon workspace, restructured to follow this template's Clean Architecture layout. Trigger when the user asks to create a new ROS 2 package or to bootstrap a Clean-Architecture-compliant codebase.
---

# Scaffolding a new ROS 2 package

This is the canonical recipe for adding a new package under `src/` of a
colcon workspace. It assumes the workspace already exists; if it does
not, ask the user to run `mkdir -p ~/<ws>/src && cd ~/<ws>` first.

## Decide first

1. **Language** — `ament_python` or `ament_cmake`. If the package
   contains heavy computation, hardware drivers, or Nav 2 plugins,
   choose `ament_cmake`. If it's an application-level orchestrator
   (lifecycle launchers, mission scripts, glue code), `ament_python`
   is fine.
2. **Domain vs Infra mix** — even a thin package gets all four Clean
   Architecture layers. The cost of empty `domain/` and `application/`
   folders is zero, and it forces correct placement when behaviour
   grows.
3. **Interfaces package** — if the package needs custom messages,
   services, or actions, create a separate `<name>_msgs` package
   *first* (ament_cmake with `rosidl_default_generators`). The
   implementation package then `<depend>`s on it.

## Step 1 — generate the skeleton

```bash
# from <workspace>/src
ros2 pkg create \
  --build-type ament_python \
  --license Apache-2.0 \
  --maintainer-name  "$(git config user.name)" \
  --maintainer-email "$(git config user.email)" \
  <package_name>
```

For C++ replace `ament_python` with `ament_cmake`. The default
`ros2 pkg create` output is a starting point, not a destination — we
override the structure in the next step.

## Step 2 — impose the Clean Architecture layout

### Python (`ament_python`)

```
src/<name>/
├── package.xml
├── setup.py
├── setup.cfg
├── resource/<name>
├── <name>/
│   ├── __init__.py
│   ├── domain/                  # pure logic
│   │   ├── __init__.py
│   │   ├── entities/
│   │   ├── value_objects/
│   │   └── ports/               # abstract interfaces
│   ├── application/             # use cases
│   │   ├── __init__.py
│   │   └── use_cases/
│   ├── infrastructure/          # ROS 2 adapters
│   │   ├── __init__.py
│   │   ├── nodes/
│   │   ├── publishers/
│   │   ├── subscribers/
│   │   └── tf/
│   └── presentation/            # CLI, launch entrypoints
│       ├── __init__.py
│       └── main.py
├── launch/                      # *.launch.py
├── config/                      # *.yaml parameter files
└── test/
    ├── unit/
    ├── integration/
    └── launch/
```

### C++ (`ament_cmake`)

```
src/<name>/
├── package.xml
├── CMakeLists.txt
├── include/<name>/
│   ├── domain/
│   ├── application/
│   ├── infrastructure/
│   └── presentation/
├── src/
│   ├── domain/
│   ├── application/
│   ├── infrastructure/
│   └── presentation/
├── launch/
├── config/
└── test/
    ├── unit/
    ├── integration/
    └── launch/
```

Use `.gitkeep` files to hold empty directories so the layout survives
git operations.

## Step 3 — `package.xml` essentials

Always include the default ament linters as test deps so `colcon test`
exercises them:

```xml
<test_depend>ament_lint_auto</test_depend>
<test_depend>ament_lint_common</test_depend>
```

For Python add `<exec_depend>rclpy</exec_depend>` and
`<exec_depend>launch_ros</exec_depend>`. For C++ add
`<depend>rclcpp</depend>` (and `<depend>rclcpp_lifecycle</depend>` if
any node is lifecycle-managed).

Add only the deps you actually use. The reviewer agent will flag
orphans.

## Step 4 — `setup.py` (Python) install rules

```python
data_files=[
    ('share/ament_index/resource_index/packages',
     ['resource/' + package_name]),
    ('share/' + package_name, ['package.xml']),
    (os.path.join('share', package_name, 'launch'),
     glob('launch/*.launch.py')),
    (os.path.join('share', package_name, 'config'),
     glob('config/*.yaml')),
],
entry_points={
    'console_scripts': [
        # Filled in by /new-node
    ],
},
```

## Step 5 — `CMakeLists.txt` (C++) install rules

```cmake
install(DIRECTORY launch config DESTINATION share/${PROJECT_NAME})

ament_export_include_directories(include)
# Library targets and rclcpp_components_register_nodes are added by
# /new-node when the first node lands.
```

For pluginlib-exposed code add:
```cmake
pluginlib_export_plugin_description_file(<base_pkg> plugins.xml)
```

## Step 6 — placeholder tests

Drop in a single passing test so `colcon test` is meaningful from day
one:

```python
# test/unit/test_smoke.py
def test_smoke():
    assert True
```

```cpp
// test/unit/smoke_test.cpp
#include <gtest/gtest.h>
TEST(Smoke, Compiles) { SUCCEED(); }
```

Wire them into `setup.py` (pytest auto-discovers) or `CMakeLists.txt`:

```cmake
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
  ament_add_gtest(${PROJECT_NAME}_smoke_test test/unit/smoke_test.cpp)
endif()
```

## Step 7 — register with the workspace

Nothing to do at the workspace level (`colcon` autodiscovers). Just
run `/build <name>` to verify the skeleton compiles before adding
behaviour.

## Don't forget

* `pre-commit run --files <every file you touched>` before declaring
  done.
* If creating an interfaces package, generate it **first** and verify
  it builds — downstream packages will fail loudly otherwise.
* Add an entry to top-level README or `CHANGELOG.rst` if the project
  publishes one.
