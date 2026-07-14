> ⚠️ Warning<br>
> The library is currently being tested.

# CFS: cross-platform filesystem API in C11

A single header implementation of `std::filesystem`/`Boost.Filesystem` in `C11`.

## Usage

This section follows the GNU documentation convention: it starts with the
programmer-facing synopsis, then describes installation, compilation,
configuration, diagnostics, and examples.

### Name

**CFS** — a single-header C11 filesystem API modeled after C++17
`std::filesystem` and Boost.Filesystem.

### Synopsis

Use the declarations in every translation unit that calls CFS:

```c
#include <cfs/cfs.h>
```

Emit the implementation in **exactly one** translation unit:

```c
/* cfs_impl.c -- the only file that defines CFS_IMPLEMENTATION. */
#define CFS_IMPLEMENTATION
#include <cfs/cfs.h>
```

or include the convenience implementation header in that one file:

```c
/* cfs_impl.c -- the only file that includes cfs_impl.h. */
#include <cfs/cfs_impl.h>
```

Compile that implementation object with the rest of your program:

```sh
cc -std=c11 -I/path/to/cfs/include -c cfs_impl.c
cc -std=c11 -I/path/to/cfs/include -c main.c
cc main.o cfs_impl.o -o program
```

### Installation

CFS does not require a library archive, linker flag, package-config file, or
generated source. Copy or vendor the `include/cfs` directory somewhere on your
compiler include path, then include `<cfs/cfs.h>`.

For a local checkout, the include path is this repository's `include`
directory:

```sh
cc -std=c11 -I/path/to/cfs/include ...
```

In CMake, add the include directory to each target that uses CFS and add one
implementation source file to the final program or library:

```cmake
target_include_directories(my_program PRIVATE /path/to/cfs/include)
target_sources(my_program PRIVATE cfs_impl.c)
```

where `cfs_impl.c` contains either the `CFS_IMPLEMENTATION` form or the
`<cfs/cfs_impl.h>` form shown in the synopsis.

### Include discipline

`cfs.h` uses the single-header style: the same file contains declarations and,
when requested, definitions. The declarations are guarded by `CFS_H`; the
implementation is guarded separately by `CFS_IMPLEMENTATION_ONCE`.

- Include `<cfs/cfs.h>` normally in headers and source files that need the API.
- Define `CFS_IMPLEMENTATION` before including `<cfs/cfs.h>` in exactly one
  `.c` file, or include `<cfs/cfs_impl.h>` in exactly one `.c` file.
- Do not define `CFS_IMPLEMENTATION` in more than one translation unit; doing
  so emits duplicate external symbols at link time.
- Do not define `CFS_IMPLEMENTATION` in a public header; every source file that
  includes that header would emit the implementation.

Because the implementation has its own one-shot guard, it is harmless if another
header included `<cfs/cfs.h>` for declarations before the implementation file
defines `CFS_IMPLEMENTATION`; the function bodies are still emitted once.

### Configuration macros

Define CFS configuration macros before the first include of `<cfs/cfs.h>` in
the translation unit that emits the implementation.

| Macro                                             | Meaning                                                                                                                                                                                                                   |
| :------------------------------------------------ | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `CFS_IMPLEMENTATION`                              | Emit function bodies from `cfs.h`; define in one translation unit only.                                                                                                                                                   |
| `CFS_VALIDATE_ARGUMENTS`                          | Defaults to `1`. When enabled, required `NULL` path arguments report `fs_cfs_error_invalid_argument` instead of being dereferenced, even with `NDEBUG`. Define to `0` only if unchecked release-build behavior is wanted. |
| `_WIN32_WINNT`                                    | Selects the Windows API level used by the implementation. Older Windows support can be checked by lowering this value.                                                                                                    |
| `_GNU_SOURCE`, `_POSIX_C_SOURCE`, `_XOPEN_SOURCE` | Optional POSIX feature-test macros. Define them before any system header if your program wants the native interfaces they expose. CFS builds without requiring GNU extensions.                                            |

### Description

CFS follows the C++17 [`std::filesystem`](https://en.cppreference.com/w/cpp/filesystem.html)
lexical path model and filesystem operation names where practical, expressed as
C11 functions and plain data types. The corresponding standard clauses are
[`[fs.path]`](https://eel.is/c++draft/fs.path) for paths and
[`[fs.op.funcs]`](https://eel.is/c++draft/fs.op.funcs) for filesystem
operations.

Common equivalents are:

| `std::filesystem`                 | CFS                                                       |
| :-------------------------------- | :-------------------------------------------------------- |
| `std::filesystem::path`           | `fs_path_t` (owned), `fs_cpath_t` (borrowed/read-only)    |
| `p / q`, `p /= q`                 | `fs_path_append(p, q, ec)`, `fs_path_append_s(&p, q, ec)` |
| `p += q`                          | `fs_path_concat(p, q, ec)`, `fs_path_concat_s(&p, q, ec)` |
| `p.lexically_normal()`            | `fs_path_lexically_normal(p, ec)`                         |
| `p.lexically_relative(base)`      | `fs_path_lexically_relative(p, base, ec)`                 |
| `p.lexically_proximate(base)`     | `fs_path_lexically_proximate(p, base, ec)`                |
| `p.parent_path()`, `p.filename()` | `fs_path_parent_path(p, ec)`, `fs_path_filename(p, ec)`   |
| `p.stem()`, `p.extension()`       | `fs_path_stem(p, ec)`, `fs_path_extension(p, ec)`         |
| `exists(p)`, `status(p)`          | `fs_exists(p, ec)`, `fs_status(p, ec)`                    |
| `copy(from, to, options)`         | `fs_copy_opt(from, to, options, ec)`                      |
| `directory_iterator(p)`           | `fs_directory_iterator(p, ec)`                            |
| `recursive_directory_iterator(p)` | `fs_recursive_directory_iterator(p, ec)`                  |

See [API inventory](#api-inventory) for the complete function-by-function
mapping.

### Path names and ownership

`fs_char_t` is `wchar_t` on Windows and `char` on POSIX. Use `FS_MAKE_PATH`
for portable path literals:

```c
fs_cpath_t config = FS_MAKE_PATH("config/settings.ini");
```

Use `fs_make_path` to convert a runtime `char *` to a native path, and
`fs_path_get` to convert a native path back to `char *`. Both return allocated
memory.

Every function returning `fs_path_t` returns an owned path unless documented
otherwise. Release owned paths with `free`. Do not free `FS_MAKE_PATH` literals
or borrowed `fs_cpath_t` values.

Iterator entries are borrowed paths owned by the iterator. When done, free
each `it.elems[i]` and then `it.elems` itself (see the directory example
below), or call the matching `FS_DESTROY_DIR_ITER` / `FS_DESTROY_RDIR_ITER`
macro, which performs the same teardown. Use `FS_DESTROY_PATH_ITER` for
path-component iterators.

Path decomposition and `fs_path_lexically_*` functions are lexical; like
`std::filesystem::path`, they do not access the filesystem. Existing separator
spelling is preserved. `fs_path_append` inserts `FS_PREFERRED_SEPARATOR` only
when a separator is needed; call `fs_path_make_preferred(&path, &ec)` to rewrite
all separators explicitly.

### Diagnostics

CFS has no throwing overloads. Operations that can fail take a trailing
`fs_error_code_t *ec` argument. Pass `NULL` to ignore detailed diagnostics.
When `ec` is supplied, test `ec.type` after the call:

```c
fs_error_code_t ec;
fs_umax_t size;

size = fs_file_size(FS_MAKE_PATH("data.bin"), &ec);
if (ec.type != fs_error_type_none) {
        fprintf(stderr, "file_size failed (%d): %s\n", ec.code, ec.msg);
        return 1;
}
```

`ec.type` identifies the code domain:

- `fs_error_type_none`: success.
- `fs_error_type_cfs`: portable `fs_cfs_error_t` code.
- `fs_error_type_system`: native Win32 or POSIX error code.

As with `std::filesystem::exists(p, ec)`, a path that does not exist normally
returns `FS_FALSE` without making nonexistence itself an error.

### Examples

Build and print a path:

```c
#include <stdlib.h>
#include <stdio.h>
#include <cfs/cfs.h>

int main(void)
{
        fs_error_code_t ec;
        fs_path_t path;
        char *display;

        path = fs_path_append(FS_MAKE_PATH("output"),
                              FS_MAKE_PATH("report.txt"),
                              &ec);
        if (ec.type != fs_error_type_none) {
                fprintf(stderr, "path error: %s\n", ec.msg);
                return 1;
        }

        display = fs_path_get(path);
        printf("%s\n", display);

        free(display);
        free(path);
        return 0;
}
```

Copy a tree recursively, replacing existing files:

```c
fs_error_code_t ec;
fs_copy_options_t options;

options = fs_copy_options_recursive | fs_copy_options_overwrite_existing;
fs_copy_opt(FS_MAKE_PATH("assets"), FS_MAKE_PATH("backup/assets"), options, &ec);
if (ec.type != fs_error_type_none) {
        fprintf(stderr, "copy failed: %s\n", ec.msg);
        return 1;
}
```

With no options, copying a directory copies its immediate entries but does not
descend into nested directories. Add `fs_copy_options_recursive` to copy the
complete tree, matching `std::filesystem::copy`.

List directory entries:

```c
fs_error_code_t ec;
fs_dir_iter_t it;
ptrdiff_t i;

it = fs_directory_iterator(FS_MAKE_PATH("."), &ec);
if (ec.type != fs_error_type_none) {
        fprintf(stderr, "directory_iterator failed: %s\n", ec.msg);
        return 1;
}

/* `it.elems` is a NULL-terminated array of borrowed entry paths; the NULL
 * terminator is the loop's stop condition, so plain indexing is enough. */
for (i = 0; it.elems[i]; i++) {
        char *display = fs_path_get(it.elems[i]);
        printf("%s\n", display);
        free(display);
}

/* entries are owned by the iterator: free each, then the array itself. */
for (i = 0; it.elems[i]; i++)
        free((void *)it.elems[i]);
free((void *)it.elems);
```

Unlike C++ `std::filesystem::directory_iterator`, which lazily streams one
entry per `++it` and yields `directory_entry` objects, CFS materializes the
whole directory up front into a `NULL`-terminated `it.elems` array of
borrowed `fs_cpath_t` entry paths. Iterate it by direct index — the `NULL`
terminator is the stop condition — and free each `it.elems[i]` and finally
`it.elems` (the `FS_DESTROY_DIR_ITER(name, it)` macro performs the same
teardown). For a recursive walk, use `fs_recursive_directory_iterator` (same
`fs_dir_iter_t` shape, same indexing), or the `FOR_EACH_ENTRY_IN_RDIR` /
`FS_DESTROY_RDIR_ITER` macros. `fs_directory_options_follow_directory_symlink`
and `fs_directory_options_skip_permission_denied` correspond to the similarly
named `std::filesystem::directory_options` values.

### Testing this checkout

To run the bundled CTest suite from a checkout:

```sh
cmake -S tests -B tests/.build/readme -DCMAKE_C_STANDARD=11
cmake --build tests/.build/readme
ctest --test-dir tests/.build/readme
```

If you use Nix, enter the development shell first with `nix develop`.

## OS requirements

| Windows           | Linux | macOS              |
| :---------------- | :---- | :----------------- |
| Windows **95+**\* | Any   | macOS (**Darwin**) |

Older Windows versions are checked by modifying the `_WIN32_WINNT` value.

Linux, macOS, and Windows are tested in CI with CTest (see
`.github/workflows/`). Some specific fixes are implemented for `FreeBSD`, whose
compatibility is not covered by CI.

## Differences with std::filesystem

`std::filesystem` implementation across compilers is _extremely_ inconsistent. This
library adopts the most **common** or **logical** way across various implementations,
or a **custom** one.

- On `Windows`, paths above `MAX_PATH` _(260 chars)_ length are supported.
- Empty paths `""` are **not** transformed in `"."`. `NULL` path arguments are
  treated as **fs_cfs_error_invalid_argument** while argument validation is
  enabled, including in release builds by default.
- `fs_file_time_type` is based on the **UNIX** epoch on **all** OSs.
- `fs_hard_link_count` never includes the file itself as a link, for
  consistency across operating systems.

## API inventory

A function-by-function mapping of the C11 API declared in `cfs.h` to
[`std::filesystem`](https://en.cppreference.com/w/cpp/filesystem.html)
(C++17, `[fs.path]` and `[fs.op.funcs]`). Conventions:

- Every operation takes a trailing `fs_error_code_t *ec` (pass `NULL` to
  ignore); C++'s throwing and `std::error_code` overloads collapse into this
  single `ec` form.
- The `_s` suffix marks the `fs_file_status_t`-based overload, mirroring C++'s
  `is_*(file_status)` and `exists(file_status)` forms. (`fs_status_known(s)` is
  the status-only form and has no `_s` sibling.)
- The `_opt` suffix marks the options-bearing overload:
  `fs_copy_opt`, `fs_copy_file_opt`, `fs_permissions_opt`,
  `fs_directory_iterator_opt`, `fs_recursive_directory_iterator_opt`.
- `fs_create_directory_cp` copies attributes from an `existing` path.
- Owned results (`fs_path_t`) are `malloc`-allocated; `free` them. Borrowed
  inputs and results (`fs_cpath_t`, iterator entries) are not freed by the caller.

### Core types

| `std::filesystem`           | CFS                                          | Notes                                               |
| :-------------------------- | :------------------------------------------- | :-------------------------------------------------- |
| `bool`                      | `fs_bool_t`                                  | `FS_TRUE` / `FS_FALSE`                              |
| `path::value_type`          | `fs_char_t`                                  | `wchar_t` on Windows, `char` on POSIX               |
| `std::filesystem::path`     | `fs_path_t` (owned), `fs_cpath_t` (borrowed) | `fs_path_t` is `malloc`'d; `free` when done         |
| `path::preferred_separator` | `FS_PREFERRED_SEPARATOR`                     | `FS_PREFERRED_SEPARATOR_S` for string form          |
| `file_type`                 | `fs_file_type_t`                             | adds `fs_file_type_junction`                        |
| `perms`                     | `fs_perms_t`                                 | same bit layout                                     |
| `perm_options`              | `fs_perm_options_t`                          |                                                     |
| `copy_options`              | `fs_copy_options_t`                          |                                                     |
| `directory_options`         | `fs_directory_options_t`                     |                                                     |
| `file_status`               | `fs_file_status_t`                           |                                                     |
| `file_time_type`            | `fs_file_time_type_t`                        | UNIX epoch on all OSs                               |
| `space_info`                | `fs_space_info_t`                            |                                                     |
| `error_code`                | `fs_error_code_t`                            | `type` / `code` / `msg`; replaces C++ error channel |
| —                           | `fs_error_type_t`, `fs_cfs_error_t`          | portable CFS error domain                           |

### Path construction & conversion

| `std::filesystem`               | CFS               | Notes                                                         |
| :------------------------------ | :---------------- | :------------------------------------------------------------ |
| `path(const char*)` constructor | `fs_make_path(p)` | locale-dependent narrow→native (`mbstowcs` on Windows)        |
| `path::string()`                | `fs_path_get(p)`  | native→narrow `char*` (`wcstombs` on Windows); caller `free`s |

### Path operations

| `std::filesystem::path`          | CFS                                              |
| :------------------------------- | :----------------------------------------------- |
| `path(const path&)` (copy)       | `fs_path_dupe(p, ec)`                            |
| `operator/=(other)`              | `fs_path_append_s(&p, other, ec)`                |
| `operator/(p, q)`                | `fs_path_append(p, other, ec)`                   |
| `operator+=(other)`              | `fs_path_concat_s(&p, other, ec)`                |
| `operator+(p, q)`                | `fs_path_concat(p, other, ec)`                   |
| `clear()`                        | `fs_path_clear(&p, ec)`                          |
| `make_preferred()`               | `fs_path_make_preferred(&p, ec)`                 |
| `remove_filename()`              | `fs_path_remove_filename(&p, ec)`                |
| `replace_filename(replacement)`  | `fs_path_replace_filename(&p, replacement, ec)`  |
| `replace_extension(replacement)` | `fs_path_replace_extension(&p, replacement, ec)` |
| `compare(other)`                 | `fs_path_compare(p, other, ec)`                  |
| `lexically_normal()`             | `fs_path_lexically_normal(p, ec)`                |
| `lexically_relative(base)`       | `fs_path_lexically_relative(p, base, ec)`        |
| `lexically_proximate(base)`      | `fs_path_lexically_proximate(p, base, ec)`       |
| `root_name()`                    | `fs_path_root_name(p, ec)`                       |
| `root_directory()`               | `fs_path_root_directory(p, ec)`                  |
| `root_path()`                    | `fs_path_root_path(p, ec)`                       |
| `relative_path()`                | `fs_path_relative_path(p, ec)`                   |
| `parent_path()`                  | `fs_path_parent_path(p, ec)`                     |
| `filename()`                     | `fs_path_filename(p, ec)`                        |
| `stem()`                         | `fs_path_stem(p, ec)`                            |
| `extension()`                    | `fs_path_extension(p, ec)`                       |
| `has_root_path()`                | `fs_path_has_root_path(p, ec)`                   |
| `has_root_name()`                | `fs_path_has_root_name(p, ec)`                   |
| `has_root_directory()`           | `fs_path_has_root_directory(p, ec)`              |
| `has_relative_path()`            | `fs_path_has_relative_path(p, ec)`               |
| `has_parent_path()`              | `fs_path_has_parent_path(p, ec)`                 |
| `has_filename()`                 | `fs_path_has_filename(p, ec)`                    |
| `has_stem()`                     | `fs_path_has_stem(p, ec)`                        |
| `has_extension()`                | `fs_path_has_extension(p, ec)`                   |
| `is_absolute()`                  | `fs_path_is_absolute(p, ec)`                     |
| `is_relative()`                  | `fs_path_is_relative(p, ec)`                     |

### Path iteration

| `std::filesystem::path`        | CFS                        |
| :----------------------------- | :------------------------- |
| `path::iterator` state         | `fs_path_iter_t`           |
| `begin()`                      | `fs_path_begin(p, ec)`     |
| `end()`                        | `fs_path_end(p)`           |
| `iterator::operator++`         | `fs_path_iter_next(&it)`   |
| `iterator::operator--`         | `fs_path_iter_prev(&it)`   |
| `*it`                          | `FS_DEREF_PATH_ITER(it)`   |
| range-for over components      | `FOR_EACH_PATH_ITER(it)`   |
| (iterator cleanup / RAII dtor) | `FS_DESTROY_PATH_ITER(it)` |

### Filesystem operations (free functions)

| `std::filesystem`                            | CFS                                             |
| :------------------------------------------- | :---------------------------------------------- |
| `absolute(p, ec)`                            | `fs_absolute(p, ec)`                            |
| `canonical(p, ec)`                           | `fs_canonical(p, ec)`                           |
| `weakly_canonical(p, ec)`                    | `fs_weakly_canonical(p, ec)`                    |
| `relative(p, base, ec)`                      | `fs_relative(p, base, ec)`                      |
| `proximate(p, base, ec)`                     | `fs_proximate(p, base, ec)`                     |
| `copy(from, to, ec)`                         | `fs_copy(from, to, ec)`                         |
| `copy(from, to, options, ec)`                | `fs_copy_opt(from, to, options, ec)`            |
| `copy_file(from, to, ec)`                    | `fs_copy_file(from, to, ec)`                    |
| `copy_file(from, to, options, ec)`           | `fs_copy_file_opt(from, to, options, ec)`       |
| `copy_symlink(from, to, ec)`                 | `fs_copy_symlink(from, to, ec)`                 |
| `create_directory(p, ec)`                    | `fs_create_directory(p, ec)`                    |
| `create_directory(p, existing, ec)`          | `fs_create_directory_cp(p, existing, ec)`       |
| `create_directories(p, ec)`                  | `fs_create_directories(p, ec)`                  |
| `create_hard_link(target, link, ec)`         | `fs_create_hard_link(target, link, ec)`         |
| `create_symlink(target, link, ec)`           | `fs_create_symlink(target, link, ec)`           |
| `create_directory_symlink(target, link, ec)` | `fs_create_directory_symlink(target, link, ec)` |
| `current_path(ec)`                           | `fs_current_path(ec)`                           |
| `current_path(p, ec)`                        | `fs_set_current_path(p, ec)`                    |
| `exists(file_status)`                        | `fs_exists_s(s)`                                |
| `exists(p, ec)`                              | `fs_exists(p, ec)`                              |
| `equivalent(p1, p2, ec)`                     | `fs_equivalent(p1, p2, ec)`                     |
| `file_size(p, ec)`                           | `fs_file_size(p, ec)`                           |
| `hard_link_count(p, ec)`                     | `fs_hard_link_count(p, ec)`                     |
| `last_write_time(p, ec)`                     | `fs_last_write_time(p, ec)`                     |
| `last_write_time(p, new_time, ec)`           | `fs_set_last_write_time(p, new_time, ec)`       |
| `permissions(p, prms, ec)`                   | `fs_permissions(p, prms, ec)`                   |
| `permissions(p, prms, opts, ec)`             | `fs_permissions_opt(p, prms, opts, ec)`         |
| `read_symlink(p, ec)`                        | `fs_read_symlink(p, ec)`                        |
| `remove(p, ec)`                              | `fs_remove(p, ec)`                              |
| `remove_all(p, ec)`                          | `fs_remove_all(p, ec)`                          |
| `rename(old_p, new_p, ec)`                   | `fs_rename(old_p, new_p, ec)`                   |
| `resize_file(p, size, ec)`                   | `fs_resize_file(p, size, ec)`                   |
| `space(p, ec)`                               | `fs_space(p, ec)`                               |
| `status(p, ec)`                              | `fs_status(p, ec)`                              |
| `symlink_status(p, ec)`                      | `fs_symlink_status(p, ec)`                      |
| `temp_directory_path(ec)`                    | `fs_temp_directory_path(ec)`                    |

### File status & type queries

The `_s` overloads take a `fs_file_status_t` (C++'s `file_status` forms); the
path overloads match C++'s `path, ec` forms.

| `std::filesystem`   | CFS (`file_status`)         | CFS (path)                    |
| :------------------ | :-------------------------- | :---------------------------- |
| `is_block_file`     | `fs_is_block_file_s(s)`     | `fs_is_block_file(p, ec)`     |
| `is_character_file` | `fs_is_character_file_s(s)` | `fs_is_character_file(p, ec)` |
| `is_directory`      | `fs_is_directory_s(s)`      | `fs_is_directory(p, ec)`      |
| `is_fifo`           | `fs_is_fifo_s(s)`           | `fs_is_fifo(p, ec)`           |
| `is_other`          | `fs_is_other_s(s)`          | `fs_is_other(p, ec)`          |
| `is_regular_file`   | `fs_is_regular_file_s(s)`   | `fs_is_regular_file(p, ec)`   |
| `is_socket`         | `fs_is_socket_s(s)`         | `fs_is_socket(p, ec)`         |
| `is_symlink`        | `fs_is_symlink_s(s)`        | `fs_is_symlink(p, ec)`        |
| `status_known`      | `fs_status_known(s)`        | —                             |
| `is_empty(p, ec)`   | —                           | `fs_is_empty(p, ec)`          |

### Directory iteration

| `std::filesystem`                              | CFS                                                          |
| :--------------------------------------------- | :----------------------------------------------------------- |
| `directory_iterator` state                     | `fs_dir_iter_t`                                              |
| `directory_iterator(p, ec)`                    | `fs_directory_iterator(p, ec)`                               |
| `directory_iterator(p, options, ec)`           | `fs_directory_iterator_opt(p, options, ec)`                  |
| `directory_iterator::operator++`               | `fs_dir_iter_next(&it)`                                      |
| `directory_iterator::operator--`               | `fs_dir_iter_prev(&it)`                                      |
| `recursive_directory_iterator` state           | `fs_recursive_dir_iter_t`                                    |
| `recursive_directory_iterator(p, ec)`          | `fs_recursive_directory_iterator(p, ec)`                     |
| `recursive_directory_iterator(p, options, ec)` | `fs_recursive_directory_iterator_opt(p, options, ec)`        |
| `recursive_directory_iterator::operator++`     | `fs_recursive_dir_iter_next(it)` (alias)                     |
| `recursive_directory_iterator::operator--`     | `fs_recursive_dir_iter_prev(it)` (alias)                     |
| `*it` (entry path)                             | `FS_DEREF_DIR_ITER(it)` / `FS_DEREF_RDIR_ITER(it)`           |
| range-for over entries                         | `FOR_EACH_ENTRY_IN_DIR(name, it)` / `FOR_EACH_ENTRY_IN_RDIR` |
| (iterator cleanup / RAII dtor)                 | `FS_DESTROY_DIR_ITER(name, it)` / `FS_DESTROY_RDIR_ITER`     |

### Convenience macros

| `std::filesystem`     | CFS                    |
| :-------------------- | :--------------------- |
| `true` / `false`      | `FS_TRUE` / `FS_FALSE` |
| portable path literal | `FS_MAKE_PATH(p)`      |

### Not modeled

These parts of `std::filesystem` have no direct CFS equivalent and are
intentionally omitted; the differences are summarized above.

- `directory_entry` — iteration yields borrowed entry paths directly via
  `FS_DEREF_DIR_ITER`; query an entry's attributes with `fs_status(entry, ec)`
  etc. There is no cached-attribute wrapper.
- `filesystem_error` — failures surface through `fs_error_code_t`.
- `path::native()` / `path::c_str()` — the `fs_path_t` / `fs_cpath_t` value _is_
  the native `fs_char_t` string; no conversion function is needed.
- `path::u8string()` / `path::wstring()` — use `fs_path_get` for the narrow
  `char*` form; the native `fs_char_t` value is available directly.
- Throwing overloads of `current_path`, `last_write_time`, `permissions`,
  `status`, `symlink_status`, `file_size`, etc. — only the `ec` forms are
  provided.
