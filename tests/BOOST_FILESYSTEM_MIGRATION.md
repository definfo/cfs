# Boost.Filesystem test migration notes

Source: <https://github.com/boostorg/filesystem/tree/develop/test>

The Boost test suite is C++/Boost.Build based. CFS tests are C89 executables
registered with CTest, so tests are migrated as focused C programs rather than
copied verbatim.

## Initial migration

- `boost_operations_smoke_test.c` adapts smoke-test scenarios from Boost's
  `test/operations_unit_test.cpp`:
  - status and symlink-status queries
  - missing-file error-code behavior
  - directory and recursive-directory iterator smoke coverage
  - space and equivalent checks
- `boost_path_lexical_test.c` adapts focused lexical path scenarios from
  Boost's `test/path_test.cpp` and `test/relative_test.cpp`:
  - decomposition examples (`parent_path`, `filename`, `stem`, `extension`)
  - representative `lexically_normal` cases
  - representative `lexically_relative` and `lexically_proximate` cases

Boost-originated material is covered by `third_party/boost/LICENSE_1_0.txt`.
