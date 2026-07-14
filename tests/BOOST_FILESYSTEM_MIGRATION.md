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
- `boost_path_modifiers_test.c` adapts focused modifier/comparison scenarios
  from Boost's `test/path_test.cpp`:
  - append and concat cases
  - `replace_filename`, `replace_extension`, and `make_preferred`
  - representative `compare` sign checks
- `boost_copy_test.c` adapts core copy operation scenarios from Boost's
  `test/copy_test.cpp`:
  - default file and directory copies
  - recursive and directories-only copies
  - representative error-code paths

Boost-originated material is covered by `third_party/boost/LICENSE_1_0.txt`.
