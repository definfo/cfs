> ⚠️ Warning<br>
> The library is currently being tested.

# CFS: cross-platform filesystem API in C89

A single header implementation of `std::filesystem`/`Boost.Filesystem` in `C89`.

### Usage:

`cfs.h` is a single-header library in the [stb](https://github.com/nothings/stb)
style: the same file provides both the declarations and the function bodies.
The API is guarded by `#ifndef CFS_H`, and the implementation lives behind a
separate `#ifdef CFS_IMPLEMENTATION` block with its own one-shot guard, so the
header is safe to `#include` as many times and in any order you like.

To pull in the implementation, define **CFS_IMPLEMENTATION** in **exactly one**
translation unit before including the header. Every other file just includes it
normally to get the declarations:

```c
// cfs_impl.c  --  the ONLY file that defines CFS_IMPLEMENTATION.
#define CFS_IMPLEMENTATION
#include <cfs/cfs.h>
```

```c
// any_other_file.c / your_header.h  --  declarations only, no macro.
#include <cfs/cfs.h>
```

If you would rather not spell out the macro at all, include the companion
header `cfs_impl.h` in that one implementation file instead. It simply defines
`CFS_IMPLEMENTATION` for you and includes `cfs.h`:

```c
// cfs_impl.c -- the ONLY file that includes cfs_impl.h.
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

### OS requirements

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
 - `fs_hard_link_count` always does **not** include the file itself as a link for
   consistency across operating systems.
