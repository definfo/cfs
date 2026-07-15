# CFS: cross-platform filesystem API in C11

A single-header C11 implementation of `std::filesystem`/`Boost.Filesystem`, with a separate [`cio.h`](include/cfs/cio.h) byte-level file-I/O extension.

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
/* cfs.c -- the only file that defines CFS_IMPLEMENTATION. */
#define CFS_IMPLEMENTATION
#include <cfs/cfs.h>
```

Compile that implementation object with the rest of your program:

```sh
cc -std=c11 -I/path/to/cfs/include -c cfs.c
cc -std=c11 -I/path/to/cfs/include -c main.c
cc main.o cfs.o -o program
```

### Installation

Copy or vendor the `include/cfs` directory somewhere on your
compiler include path, then include `<cfs/cfs.h>`.

```sh
cc -std=c11 -I/path/to/cfs/include ...
```

In CMake, add the include directory to each target that uses CFS and add one
implementation source file to the final program or library:

```cmake
target_include_directories(my_program PRIVATE /path/to/cfs/include)
target_sources(my_program PRIVATE cfs.c)
```

where `cfs.c` contains `#define CFS_IMPLEMENTATION` as shown in the synopsis.

### Include discipline

`cfs.h` uses the single-header style: the same file contains declarations and,
when requested, definitions. The declarations are guarded by `CFS_H`; the
implementation is guarded separately by `CFS_IMPLEMENTATION_ONCE`.

- Include `<cfs/cfs.h>` normally in headers and source files that need the API.
- Define `CFS_IMPLEMENTATION` before including `<cfs/cfs.h>` in exactly one
  `.c` file.
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
[`[fs.class.path]`](https://eel.is/c++draft/fs.class.path) for paths and
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

`fs_char_t` is `wchar_t` on Windows and `char` on POSIX, so a native path is
wide on Windows and narrow elsewhere. CFS has two ways to obtain one from a
`char` spelling, differing in ownership:

|            | `FS_PATH("…")`         | `fs_make_path(p)`                      |
| :--------- | :-------------------------- | :------------------------------------- |
| Kind       | macro — a path literal      | function — an owned path               |
| Input      | compile-time string literal | runtime `char *` (`argv`, a buffer, …) |
| Result     | `fs_cpath_t` (borrowed)     | `fs_path_t` (owned)                    |
| Storage    | static (string literal)     | heap (`malloc`/`calloc`)               |
| On Windows | auto-wide (`L"…"`)          | `mbstowcs` narrow→wide (locale)        |
| `free`?    | never                       | caller                                 |

```c
fs_cpath_t lit = FS_PATH("config/settings.ini"); /* literal; do not free */

fs_path_t runtime = fs_make_path(argv[1]);            /* owned; free when done */
```

`fs_path_get` is the reverse — it converts a native path back to a narrow
`char *`, heap-allocated (`free` it). Its locale-independent, lossless
counterpart is `fs_path_u8` (always UTF-8); pair it with `fs_make_path_u8`
(UTF-8 in) for a lossless round trip regardless of the C locale.

Every function returning `fs_path_t` returns an owned path unless documented
otherwise; release owned paths with `free`. Do not free `FS_PATH`
literals or borrowed `fs_cpath_t` values.

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
`fs_error_code_t *ec` argument. Pass `NULL` when you do not need failure
details and will instead rely on the return value (see [Error handling](#error-handling)); pass `&ec`
whenever you branch on or report a failure. When `ec` is supplied, test
`ec.type` after the call:

```c
fs_error_code_t ec;
fs_umax_t size;

size = fs_file_size(FS_PATH("data.bin"), &ec);
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

See [`examples/build_and_print_path.c`](examples/build_and_print_path.c).

Copy a tree recursively, replacing existing files:

See [`examples/copy_tree.c`](examples/copy_tree.c).

With no options, copying a directory copies its immediate entries but does not
descend into nested directories. Add `fs_copy_options_recursive` to copy the
complete tree, matching `std::filesystem::copy`.

List directory entries:

See [`examples/list_directory_entries.c`](examples/list_directory_entries.c).

Unlike C++ `std::filesystem::directory_iterator`, which lazily streams one
entry per `++it` and yields `directory_entry` objects, CFS materializes the
whole directory up front into a `NULL`-terminated `it.elems` array of
borrowed `fs_cpath_t` entry paths. Iterate it by direct index — the `NULL`
terminator is the stop condition — and free each `it.elems[i]` and finally
`it.elems` (the `FS_DESTROY_DIR_ITER(name, it)` macro performs the same
teardown). For a recursive walk, `fs_recursive_directory_iterator(p, &ec)`
constructs the iterator (same `fs_dir_iter_t` shape, same indexing); then walk
by direct index, or with `FOR_EACH_ENTRY_IN_RDIR(name, it)` and
`FS_DESTROY_RDIR_ITER(name, it)` for teardown. `fs_directory_options_follow_directory_symlink`
and `fs_directory_options_skip_permission_denied` correspond to the similarly
named `std::filesystem::directory_options` values.

Convert a user-supplied `char *` path and canonicalize it:

See [`examples/canonicalize_path.c`](examples/canonicalize_path.c).

This is the round trip a program takes with a path read from `argv` or a
buffer. `fs_make_path` accepts a runtime `char *` and widens it on Windows, so it
replaces the `realpath` / `_fullpath` shim a portable program otherwise repeats
in every translation unit. `fs_canonical` borrows its argument and returns a new
owned path, so free the pre-canonical copy separately. Prefer `fs_absolute` when
symlinks should not be resolved, and `fs_weakly_canonical` for a lenient form
that leaves a missing tail intact. Join components with `fs_path_append` /
`fs_path_append_s` (in place) and normalize separators with
`fs_path_make_preferred`, instead of hand-inserting separators.

Judge a file's type, then read its bytes:

`cfs.h` mirrors `std::filesystem`, which — by design — does not read or write
file contents; that is left to `<fstream>`/`<cstdio>`. The separate extension
header [`cio.h`](include/cfs/cio.h) (beyond `std::filesystem`) provides
the byte-level layer: `io_read_file` (whole-file), `io_file_open` /
`io_file_read` / `io_file_write` / `io_file_close` (streaming), and
`io_file_gets` (line). File contents are bytes (`char`), independent of the
path encoding (`fs_char_t`).

- **Whole-file.** See [`examples/read_file_size_based.c`](examples/read_file_size_based.c).
  `io_read_file` opens, sizes, and reads the file into one `malloc`'d,
  NUL-terminated buffer (the size-based read used throughout `sac_c_parser`'s
  `mio_read_file`, `ToString`, `Paras`). It fails cleanly on a non-openable
  path (e.g. a directory → `EISDIR`).
- **Stream (non-seekable or unbounded).** See [`examples/read_file_stream.c`](examples/read_file_stream.c).
  `io_file_read` in fixed chunks, looping until EOF — for pipes, fifos,
  sockets, and any source with no size to preallocate.
- **Raw lines.** See [`examples/read_file_raw_lines.c`](examples/read_file_raw_lines.c).
  `io_file_gets` reads up to a newline as a NUL-terminated `char *` without
  interpreting bytes as a charset — safe for text where only line boundaries
  matter.

Byte-level character-encoding decode (e.g. UTF-8 via `mbrtowc`, wide `fgetwc`)
is out of even `cio`'s scope — keep the byte stream from `cio` and decode
in the caller. (`fs_char_t` is the _path_ encoding, not a file-content
encoding.)

`cfs.h`'s role around a read is the same object-level prelude — type, size,
existence, and a symlink's target via `fs_read_symlink` — plus
`fs_exists(candidate, &ec)` to probe a search path.

Walk a directory tree recursively:

See [`examples/walk_directory_tree.c`](examples/walk_directory_tree.c).

The recursive iterator has the same `fs_dir_iter_t` shape and direct-indexing
loop as the flat one; `FOR_EACH_ENTRY_IN_RDIR` and `FS_DESTROY_RDIR_ITER` are the
recursive forms of `FOR_EACH_ENTRY_IN_DIR` and `FS_DESTROY_DIR_ITER`, and both
take the loop's entry variable as their first argument.
`fs_directory_options_follow_directory_symlink` recurses through symlinked
directories (the analogue of the same `std::filesystem::directory_options`
value). Unlike C++ `recursive_directory_iterator`, CFS materializes the whole
tree eagerly rather than streaming with a live stack lazily.

Write an output file, backing up or overwriting an existing one:

See [`examples/write_output_file.c`](examples/write_output_file.c).

CFS handles the object-level decisions around a write — ensure the parent with
`fs_create_directories`, snapshot a collision with `fs_copy_file_opt` (pass
`fs_copy_options_overwrite_existing` or `fs_copy_options_skip_existing` to
control the outcome), and remove for an overwrite — then `io_write_file` /
`io_append_file` (cio) write the bytes.

Tell whether two paths name the same filesystem object:

See [`examples/same_filesystem_object.c`](examples/same_filesystem_object.c).

`fs_equivalent` replaces the `realpath(p, NULL)` + `strcmp` idiom: two paths are
equivalent when they resolve to the same filesystem object (same inode and
device, matching `std::filesystem::equivalent`). `fs_status` follows a symlink
(reports the target's status); `fs_symlink_status` does not (reports the link
itself).

Content comparison is out of scope for `cfs.h`: `fs_equivalent` reports whether
two paths name the _same filesystem object_, not whether their bytes are
equal. `cfs.h` exposes no `fs_*` that reads file contents — `fs_read_symlink`
reads a symlink target, not file bytes. For byte equality, short-circuit on
unequal `fs_file_size`, then compare contents incrementally with
`io_file_open` / `io_file_read` and `memcmp` — see
`compare_then_write.c` and `write_tmpfile_compare_then_write.c`.

Stream a file to a file:

See [`examples/write_file_stream.c`](examples/write_file_stream.c).
The write counterpart of `read_file_stream.c`: `io_file_open` / `read` /
`write` (cio) stream the bytes in chunks; `cfs.h` builds the path.

Compare, then write (skip if unchanged):

See [`examples/compare_then_write.c`](examples/compare_then_write.c).
Compare an in-memory string buffer against the target file: read the target
as a binary `io_file_read` stream (cio), compare each chunk against the
buffer, and overwrite only if they differ (or the target is missing). In text
mode, use `io_file_gets` to compare line by line instead.

Write to a temp file, compare, then commit (atomic update):

See [`examples/write_tmpfile_compare_then_write.c`](examples/write_tmpfile_compare_then_write.c).
Write an in-memory string buffer to a temp file beside the target first,
stream-compare temp and target with `io_file_read`, then `fs_rename` the temp
over the target — readers see either the old or the new file, never a partial
one. In text mode, use `io_file_gets` to compare line by line instead.

### Testing this checkout

To run the bundled CTest suite from a checkout:

```sh
cmake -S tests -B tests/.build
cmake --build tests/.build
ctest --test-dir tests/.build
```

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
- `fs_file_time_type` is based on the UNIX epoch on **all** OSs.
- `fs_hard_link_count` never includes the file itself as a link, for
  consistency across operating systems.

## API inventory

A function-by-function mapping of the C11 API declared in `cfs.h` to
[`std::filesystem`](https://en.cppreference.com/w/cpp/filesystem.html)
(C++17, `[fs.class.path]` and `[fs.op.funcs]`). Conventions:

- Every operation takes a trailing `fs_error_code_t *ec`; C++'s throwing and
  `std::error_code` overloads collapse into this single form (see
  [Error handling](#error-handling)).
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

### Error handling

`std::filesystem` exposes **two** error paradigms per operation, and the
caller picks one at each call site: a throwing overload (`f(p)`) and a
`std::error_code` overload (`f(p, ec)`); uncaught failures throw
`std::filesystem::filesystem_error`. C (no overloading, no exceptions)
collapses both into a **single** function with an **optional** trailing
`fs_error_code_t *ec`:

| `std::filesystem`                       | CFS                                                                      |
| :-------------------------------------- | :----------------------------------------------------------------------- |
| throwing overload (`f(p)`)              | same function, `&ec`; check `ec.type`, then abort or propagate           |
| `std::error_code` overload (`f(p, ec)`) | same function, `&ec`                                                     |
| `filesystem_error` exception            | `fs_error_code_t` (`type`/`code`/`msg`); caller decides whether to abort |
| (no way to ignore)                      | `NULL` — discard details, rely on the return                             |

`ec.type` selects the domain (`fs_error_type_none` / `_cfs` / `_system`;
see [Diagnostics](#diagnostics)). Passing `NULL` is crash-safe but does
**not** mean the call cannot fail — it discards the failure details, so you
must rely on the return value. It is correct only when that return is an
unambiguous, checked sentinel:

- `fs_exists` / `fs_is_*` / `fs_status` → `FS_FALSE` / `fs_file_type_not_found`
- `fs_file_size` → `(fs_umax_t)-1`
- `fs_remove` / `fs_create_directory` → `FS_FALSE`
- `fs_path_append` / `fs_canonical` / `fs_path_dupe` → `NULL`

Do **not** pass `NULL` and ignore the return — the failure is then silent.
Prefer `&ec` whenever you branch on, log, or propagate a failure.

> Caveat: `fs_absolute` (and a few path-returners) yield a non-`NULL` empty
> path on some errors, so `NULL` plus `if (p)` does not detect them — pass
> `&ec` (or check the result is non-empty) for those.

C++ (two overloads) vs CFS (one function):

```cpp
// C++ — the caller picks the paradigm per call site
namespace fs = std::filesystem;
fs::file_size(p);        // throws filesystem_error
std::error_code e;
fs::file_size(p, e);     // no throw; e set on failure
```

```c
/* CFS — one function; pass &ec for details, or NULL and check the return */
fs_error_code_t ec = {0};
fs_umax_t sz = fs_file_size(p, &ec);
if (ec.type != fs_error_type_none) {
        /* report ec.code / ec.msg, then abort or propagate — like a throw */
        return 1;
}
/* best-effort: discard details, check the sentinel */
if (fs_file_size(p, NULL) == (fs_umax_t)-1)
        return 1;
```

### Path construction & conversion

| `std::filesystem`               | CFS               | Notes                                                         |
| :------------------------------ | :---------------- | :------------------------------------------------------------ |
| `path(const char*)` constructor | `fs_make_path(p)`   | locale-dependent narrow→native (`mbstowcs` on Windows)        |
| `path(std::u8string)` / `u8path()` | `fs_make_path_u8(p)` | UTF-8→native, locale-independent, lossless (`MultiByteToWideChar(CP_UTF8)` on Windows) |
| `path::string()`                | `fs_path_get(p)`    | native→narrow `char*`, locale-dependent (`wcstombs` on Windows); caller `free`s |
| `path::u8string()`              | `fs_path_u8(p)`     | native→UTF-8 `char*`, locale-independent, lossless (`WideCharToMultiByte(CP_UTF8)` on Windows); caller `free`s |

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

NOTE: `fs_recursive_directory_iterator` (and `_opt`) construct the iterator —
they materialize the whole tree into the same `NULL`-terminated `it.elems`
array as `fs_directory_iterator`, so `fs_recursive_dir_iter_t` is a typedef
of `fs_dir_iter_t`. `FOR_EACH_ENTRY_IN_RDIR`, `FS_DESTROY_RDIR_ITER`, and
`fs_recursive_dir_iter_next` are **aliases** of the non-recursive
`FOR_EACH_ENTRY_IN_DIR` / `FS_DESTROY_DIR_ITER` / `fs_dir_iter_next`; walk
and teardown are identical, only construction differs. Construct with the
function, then walk by direct index or the macro — unlike C++
`recursive_directory_iterator`, CFS does not stream with a live stack.

### Convenience macros

| `std::filesystem`     | CFS                    |
| :-------------------- | :--------------------- |
| `true` / `false`      | `FS_TRUE` / `FS_FALSE` |
| portable path literal | `FS_PATH(p)`      |

### File I/O (`cio.h`, beyond `std::filesystem`)

`cfs.h` mirrors `std::filesystem`, which does not read or write file
contents. The separate header `cio.h` provides that byte-level layer so a
program can do all its file/path I/O through CFS. It is a single-header
extension with its own `CIO_IMPLEMENTATION` guard (define it in exactly one
TU); its bodies call `fs_path_get`, so either define `CFS_IMPLEMENTATION` in
the same TU or link a TU that does. File contents are bytes (`char`),
independent of the path encoding (`fs_char_t`).

| CFS (`cio.h`)                       | Purpose                                                   |
| :------------------------------------- | :-------------------------------------------------------- |
| `io_read_file(p, &len, ec)`         | whole-file read -> malloc'd, NUL-terminated buffer        |
| `io_write_file(p, buf, len, ec)`    | whole-file write (create/truncate)                        |
| `io_append_file(p, buf, len, ec)`   | whole-file append                                         |
| `io_file_open(p, mode, ec)`         | open a streaming handle (`io_file_t`)                  |
| `io_file_read` / `io_file_write` | chunked read/write via a handle                           |
| `io_file_getc` / `io_file_gets`  | byte / line read via a handle                             |
| `io_file_close`                     | close + free a handle                                     |
| `io_file_mode_t`                    | `io_file_mode_read` / `_write` / `_append` (+ `_text`) |

### Not modeled

These parts of `std::filesystem` have no direct CFS equivalent and are
intentionally omitted; the differences are summarized above.

- `directory_entry` — iteration yields borrowed entry paths directly via
  `FS_DEREF_DIR_ITER`; query an entry's attributes with `fs_status(entry, ec)`
  etc. There is no cached-attribute wrapper.
- `filesystem_error` — failures surface through `fs_error_code_t`.
- `path::native()` / `path::c_str()` — the `fs_path_t` / `fs_cpath_t` value _is_
  the native `fs_char_t` string; no conversion function is needed.
- `path::wstring()` — on Windows the `fs_path_t` / `fs_cpath_t` value is already
  `wchar_t*` (= `native()`); on POSIX there is no narrow→wide converter (use the
  native `char*` value, or `fs_path_get` / `fs_path_u8`). `path::native()` /
  `path::c_str()` are the path value itself; `path::string()` → `fs_path_get`
  (locale-dependent); `path::u8string()` → `fs_path_u8` (locale-independent,
  lossless).
- Throwing overloads of `current_path`, `last_write_time`, `permissions`,
  `status`, `symlink_status`, `file_size`, etc. — only the `ec` forms are
  provided.
