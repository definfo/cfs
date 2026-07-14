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

Boost-originated material is covered by `third_party/boost/LICENSE_1_0.txt`.
