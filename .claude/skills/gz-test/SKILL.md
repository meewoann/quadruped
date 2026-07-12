---
name: gz-test
description: Run unit, integration, performance, and benchmark tests for gz-sim, filter by name, and debug a single failing case. Trigger when the user asks to test, rerun a test, debug a flaky test, or check that a change didn't regress something.
---

# Running gz-sim tests

`gz-sim` ships three kinds of tests, all wired through CTest:

| Kind | Source root | Target prefix |
|------|-------------|---------------|
| Unit (next to source) | `src/**/<Name>_TEST.cc` | `UNIT_<Name>` |
| Integration | `test/integration/<name>.cc` | `INTEGRATION_<name>` |
| Performance | `test/performance/<name>.cc` | `PERFORMANCE_<name>` |
| Benchmark | `test/benchmark/<name>.cc` | `BENCHMARK_<name>` |
| Plugins (helpers) | `test/plugins/` | built as test fixtures, not run directly |

## Run everything

```sh
ctest --test-dir build --output-on-failure -j"$(nproc)"
```

CI requires this to be clean before merge.

## Run a subset

```sh
# only integration tests
ctest --test-dir build -R '^INTEGRATION_' --output-on-failure

# one specific suite
ctest --test-dir build -R '^UNIT_EntityComponentManager$' -V

# exclude slow/flaky ones during local iteration
ctest --test-dir build -E 'PERFORMANCE_|BENCHMARK_'
```

## Run with the gtest binary directly

This is the fastest way to debug a single failing case, because you get
full gtest filter / repeat / shuffle flags:

```sh
./build/bin/INTEGRATION_diff_drive_system \
  --gtest_filter='DiffDriveTest.*' \
  --gtest_repeat=10 \
  --gtest_shuffle
```

## Headless / CI environments

Most GUI tests need a display. Wrap with xvfb:

```sh
xvfb-run -s '-screen 0 1280x1024x24' \
  ctest --test-dir build -R '^INTEGRATION_' --output-on-failure
```

## Debugging a single failure

1. Build only that target: `cmake --build build -j$(nproc) --target INTEGRATION_foo`.
2. Reproduce: `./build/bin/INTEGRATION_foo --gtest_filter=Bar.*`.
3. For pointer issues run under ASan; for races use TSan (rebuild with
   `-DCMAKE_CXX_FLAGS="-fsanitize=thread"`).
4. Under gdb:
   ```sh
   gdb --args ./build/bin/INTEGRATION_foo --gtest_filter=Bar.NameOfCase
   ```
5. Check for SDF/world fixtures under `test/worlds/` — many integration
   tests load a world from there.

## Performance / benchmarks

```sh
./build/bin/BENCHMARK_each --benchmark_min_time=2 \
  --benchmark_out=/tmp/each.json --benchmark_out_format=json
```

Avoid running benchmarks under load — they're noisy on shared machines.

## Python tests

```sh
ctest --test-dir build -R '^PYTHON_'
```

These exercise the pybind11 bindings.

## After a green run

Tell the user **what** you ran and which counts came back (e.g.
"148 pass / 0 fail / 2 skipped"). Don't claim success without numbers.
