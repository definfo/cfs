> ⚠️ Warning<br>
> The library is currently being tested.

# CFS: cross-platform filesystem API in C89

A single header implementation of `std::filesystem`/`Boost.Filesystem` in `C89`.

## Usage

### Add the implementation

`cfs.h` is a single-header library in the [stb](https://github.com/nothings/stb)
style: the same file provides both the declarations and the function bodies.
The API is guarded by `#ifndef CFS_H`, and the implementation lives behind a
separate `#ifdef CFS_IMPLEMENTATION` block with its own one-shot guard, so the
header is safe to `#include` as many times and in any order you like.

To pull in the implementation, define **CFS_IMPLEMENTATION** in **exactly one**
translation unit before including the header. Every other file just includes it
normally to get the declarations:

```c
/* cfs_impl.c -- the ONLY file that defines CFS_IMPLEMENTATION. */
#define CFS_IMPLEMENTATION
#include <cfs/cfs.h>
```

```c
/* any_other_file.c / your_header.h -- declarations only, no macro. */
#include <cfs/cfs.h>
```

If you would rather not spell out the macro at all, include the companion
header `cfs_impl.h` in that one implementation file instead. It simply defines
`CFS_IMPLEMENTATION` for you and includes `cfs.h`:

```c
/* cfs_impl.c -- the ONLY file that includes cfs_impl.h. */
#include <cfs/cfs_impl.h>
```

The same one-TU rule applies: include `cfs_impl.h` in exactly one translation
unit, and use `cfs.h` everywhere else.

Because the implementation carries its own guard, it no longer matters whether
another header expands `<cfs/cfs.h>` (declarations only) before the file that
defines `CFS_IMPLEMENTATION` — the bodies are still emitted exactly once.

> ⚠️ Do **not** define `CFS_IMPLEMENTATION` in more than one TU (duplicate
> symbols at link time), and do **not** define it in a header (it would leak the
> implementation into every file that includes that header).

### Relationship to `std::filesystem`

CFS follows the C++17 [`std::filesystem`](https://en.cppreference.com/w/cpp/filesystem.html)
API and lexical path model where practical, expressed as C89 functions and
plain data types. The corresponding standard clauses are
[`[fs.path]`](https://eel.is/c++draft/fs.path) for paths and
[`[fs.op.funcs]`](https://eel.is/c++draft/fs.op.funcs) for filesystem
operations. The intentional differences are listed below.

Common equivalents are:

| `std::filesystem` | CFS |
|:------------------|:----|
| `std::filesystem::path` | `fs_path_t` (owned), `fs_cpath_t` (borrowed/read-only) |
| `p / q`, `p /= q` | `fs_path_append(p, q, ec)`, `fs_path_append_s(&p, q, ec)` |
| `p += q` | `fs_path_concat(p, q, ec)`, `fs_path_concat_s(&p, q, ec)` |
| `p.lexically_normal()` | `fs_path_lexically_normal(p, ec)` |
| `p.lexically_relative(base)` | `fs_path_lexically_relative(p, base, ec)` |
| `p.lexically_proximate(base)` | `fs_path_lexically_proximate(p, base, ec)` |
| `p.parent_path()`, `p.filename()` | `fs_path_parent_path(p, ec)`, `fs_path_filename(p, ec)` |
| `p.stem()`, `p.extension()` | `fs_path_stem(p, ec)`, `fs_path_extension(p, ec)` |
| `exists(p)`, `status(p)` | `fs_exists(p, ec)`, `fs_status(p, ec)` |
| `copy(from, to, options)` | `fs_copy_opt(from, to, options, ec)` |
| `directory_iterator(p)` | `fs_directory_iterator(p, ec)` |
| `recursive_directory_iterator(p)` | `fs_recursive_directory_iterator(p, ec)` |

Unlike C++, CFS has no throwing overloads. Operations report failures through
an optional `fs_error_code_t *`.

### Paths and ownership

The examples below assume:

```c
#include <stdlib.h>
#include <stdio.h>
#include <cfs/cfs.h>
```

`fs_char_t` is `wchar_t` on Windows and `char` on POSIX. Use `FS_MAKE_PATH`
for portable path literals:

```c
fs_cpath_t config = FS_MAKE_PATH("config/settings.ini");
```

Use `fs_make_path` for a runtime `char *`, and `fs_path_get` when a path must
be converted back to `char *`. Both return allocated memory.

Every function returning `fs_path_t` returns an owned path unless documented
otherwise. Release it with `free`. Do not free `FS_MAKE_PATH` literals or
borrowed `fs_cpath_t` values.

```c
fs_error_code_t ec;
fs_path_t path;
char *display;

path = fs_path_append(FS_MAKE_PATH("output"), FS_MAKE_PATH("report.txt"), &ec);
if (ec.type != fs_error_type_none) {
        fprintf(stderr, "path error: %s\n", ec.msg);
        return 1;
}

display = fs_path_get(path);
printf("%s\n", display);

free(display);
free(path);
```

Path decomposition and `fs_path_lexically_*` functions are lexical: like
`std::filesystem::path`, they do not access the filesystem. Existing separator
spelling is preserved. `fs_path_append` inserts `FS_PREFERRED_SEPARATOR` only
when a separator is needed; call `fs_path_make_preferred(&path, &ec)` to rewrite
all separators explicitly.

### Error handling

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

Pass `NULL` instead of `&ec` when the error details are intentionally ignored.
As with `std::filesystem::exists(p, ec)`, a path that does not exist normally
returns `FS_FALSE` without making nonexistence itself an error.

### Copying

`fs_copy_options_t` mirrors
[`std::filesystem::copy_options`](https://en.cppreference.com/w/cpp/filesystem/copy_options.html).
Options can be combined with bitwise OR:

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

### Directory iteration

Iterator entries are borrowed paths owned by the iterator. Destroy every
successfully created iterator with its matching macro:

```c
fs_error_code_t ec;
fs_dir_iter_t it;
fs_cpath_t entry;

it = fs_directory_iterator(FS_MAKE_PATH("."), &ec);
if (ec.type != fs_error_type_none) {
        fprintf(stderr, "directory_iterator failed: %s\n", ec.msg);
        return 1;
}

FOR_EACH_ENTRY_IN_DIR(entry, it) {
        char *display = fs_path_get(entry);
        printf("%s\n", display);
        free(display);
}
FS_DESTROY_DIR_ITER(entry, it);
```

Use `FOR_EACH_ENTRY_IN_RDIR` and `FS_DESTROY_RDIR_ITER` for a recursive
iterator. `fs_directory_options_follow_directory_symlink` and
`fs_directory_options_skip_permission_denied` correspond to the similarly
named `std::filesystem::directory_options` values.

## OS requirements

| Windows           | Linux | macOS               |
|:------------------|:------|:--------------------|
| Windows **95+**\* | Any   | macOS (**Darwin**)  |

Older Windows versions are checked by modifying the `_WIN32_WINNT` value.

Linux, macOS, and Windows are tested in CI with CTest (see
`.github/workflows/`). Some specific fixes are implemented for `FreeBSD`, whose
compatibility is not covered by CI.

## Differences with std::filesystem

`std::filesystem` implementation across compilers is *extremely* inconsistent. This
library adopts the most **common** or **logical** way across various implementations,
or a **custom** one.

 - On `Windows`, paths above `MAX_PATH` *(260 chars)* length are supported.
 - Empty paths `""` are **not** transformed in `"."` and `NULL` paths are treated as
   an error in `Debug` (**fs_cfs_error_invalid_argument**) or **undefined behavior**
   in `Release` mode.
 - `fs_file_time_type` is based on the **UNIX** epoch on **all** OSs.
 - `fs_hard_link_count` never includes the file itself as a link, for
   consistency across operating systems.
