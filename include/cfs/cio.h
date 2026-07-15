#ifndef CIO_H
#define CIO_H

/*
 * cio.h -- file-content I/O for CFS.
 *
 * This is a SEPARATE extension header, deliberately kept out of cfs.h: cfs.h
 * mirrors C++17 std::filesystem (path + object-level operations only -- see
 * [fs.path] and [fs.op.funcs]). std::filesystem does NOT read or write file
 * contents; that is left to <fstream>/<cstdio>. cio.h provides the
 * byte-level read/write layer cfs.h intentionally omits, so a program can do
 * all of its file/path I/O through CFS.
 *
 * File contents are BYTES (char), independent of the path encoding: fs_char_t
 * is wchar_t on Windows, but a file's bytes are narrow char regardless.
 *
 * Single-header, stb-style: the declarations are always visible; the function
 * bodies are emitted in exactly one translation unit that defines
 * CIO_IMPLEMENTATION before including this header. cio's implementation
 * depends only on cfs.h's PUBLIC API (fs_path_get and the error types) plus
 * <stdio.h>; it does not use cfs.h's internal _FS_* macros.
 *
 *     // one TU:
 *     #define CIO_IMPLEMENTATION
 *     #include <cfs/cio.h>
 *
 * cio's bodies call fs_path_get, whose body lives in cfs.h behind
 * CFS_IMPLEMENTATION -- so either define CFS_IMPLEMENTATION in the same TU
 * (as the examples do) or link a TU that does.
 */

#include <cfs/cfs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque streaming handle (pointer to an incomplete struct). */
typedef struct io_file *io_file_t;

// THINK: bit operation? (1 << 0), (1 << 1), etc.
typedef enum io_file_mode {
        io_file_mode_none   = 0x0,
        io_file_mode_read   = 0x1,  /* open for reading  ("rb") */
        io_file_mode_write  = 0x2,  /* create/truncate  ("wb") */
        io_file_mode_append = 0x4,  /* append            ("ab") */
        io_file_mode_text   = 0x8   /* text mode (newline translation); default binary */
} io_file_mode_t;

/* Whole-file: read into a malloc'd, NUL-terminated buffer (free it; *len is
 * the byte count excluding the NUL); write/append a buffer. */
extern char      *io_read_file(fs_cpath_t p, fs_umax_t *len, fs_error_code_t *ec);
extern fs_bool_t  io_write_file(fs_cpath_t p, const char *buf, fs_umax_t len, fs_error_code_t *ec);
extern fs_bool_t  io_append_file(fs_cpath_t p, const char *buf, fs_umax_t len, fs_error_code_t *ec);

/* Streaming handle. io_file_read returns bytes read (0 at EOF or on
 * error; check ec to tell them apart). io_file_getc returns a byte 0..255
 * or -1 (EOF/error; check ec). io_file_gets returns buf or NULL at EOF. */
extern io_file_t  io_file_open(fs_cpath_t p, io_file_mode_t mode, fs_error_code_t *ec);
extern fs_umax_t     io_file_read(io_file_t f, char *buf, fs_umax_t n, fs_error_code_t *ec);
extern fs_umax_t     io_file_write(io_file_t f, const char *buf, fs_umax_t n, fs_error_code_t *ec);
extern int           io_file_getc(io_file_t f, fs_error_code_t *ec);
extern char         *io_file_gets(io_file_t f, char *buf, fs_umax_t n, fs_error_code_t *ec);
extern void          io_file_close(io_file_t f);

#ifdef __cplusplus
}
#endif

#endif /* CIO_H */

#ifdef CIO_IMPLEMENTATION
#ifndef CIO_IMPLEMENTATION_ONCE
#define CIO_IMPLEMENTATION_ONCE

#include <stdio.h>   /* fopen, fread, fwrite, fgetc, fgets, fclose, fseek, ftell, rewind, clearerr, ferror */
#include <stdlib.h>  /* malloc, free */
#include <errno.h>   /* errno */

#ifdef __cplusplus
extern "C" {
#endif

struct io_file {
        FILE *fp;
};

static const char *io_mode_str(io_file_mode_t mode)
{
        if (mode & io_file_mode_read)
                return (mode & io_file_mode_text) ? "r" : "rb";
        if (mode & io_file_mode_write)
                return (mode & io_file_mode_text) ? "w" : "wb";
        if (mode & io_file_mode_append)
                return (mode & io_file_mode_text) ? "a" : "ab";
        return NULL;
}

/* ec helpers -- depend only on cfs.h's public error types. */
static void io_clear(fs_error_code_t *ec)
{
        if (ec) {
                ec->type = fs_error_type_none;
                ec->code = 0;
                ec->msg = NULL;
        }
}

static void io_err_cfs(fs_error_code_t *ec, fs_cfs_error_t code, const char *msg)
{
        if (ec) {
                ec->type = fs_error_type_cfs;
                ec->code = (int)code;
                ec->msg = msg;
        }
}

static void io_err_sys(fs_error_code_t *ec, const char *msg)
{
        if (ec) {
                ec->type = fs_error_type_system;
                ec->code = errno;
                ec->msg = msg;
        }
}

extern io_file_t io_file_open(const fs_cpath_t p, io_file_mode_t mode,
                                    fs_error_code_t *ec)
{
        const char  *m;
        char        *narrow;
        FILE        *fp;
        io_file_t f;

        io_clear(ec);
        if (p == NULL || p[0] == '\0') {
                io_err_cfs(ec, fs_cfs_error_invalid_argument, "io_file_open: invalid path");
                return NULL;
        }
        m = io_mode_str(mode);
        if (m == NULL) {
                io_err_cfs(ec, fs_cfs_error_invalid_argument, "io_file_open: invalid mode");
                return NULL;
        }
        narrow = fs_path_get(p);          /* cfs: native path -> narrow char* */
        if (narrow == NULL) {
                io_err_cfs(ec, fs_cfs_error_invalid_argument, "io_file_open: path conversion failed");
                return NULL;
        }
        fp = fopen(narrow, m);
        free(narrow);
        if (fp == NULL) {
                io_err_sys(ec, "io_file_open: cannot open");
                return NULL;
        }
        f = malloc(sizeof(struct io_file));
        f->fp = fp;          /* malloc failure is treated as invariant (see cfs README) */
        return f;
}

extern fs_umax_t io_file_read(io_file_t f, char *buf, fs_umax_t n,
                                 fs_error_code_t *ec)
{
        size_t r;

        io_clear(ec);
        if (f == NULL || buf == NULL) {
                io_err_cfs(ec, fs_cfs_error_invalid_argument, "io_file_read: invalid argument");
                return 0;
        }
        clearerr(f->fp);
        r = fread(buf, 1, (size_t)n, f->fp);
        if (r == 0 && ferror(f->fp))
                io_err_sys(ec, "io_file_read: read failed");
        return (fs_umax_t)r;
}

extern fs_umax_t io_file_write(io_file_t f, const char *buf, fs_umax_t n,
                                  fs_error_code_t *ec)
{
        size_t w;

        io_clear(ec);
        if (f == NULL || buf == NULL) {
                io_err_cfs(ec, fs_cfs_error_invalid_argument, "io_file_write: invalid argument");
                return 0;
        }
        clearerr(f->fp);
        w = fwrite(buf, 1, (size_t)n, f->fp);
        if (w != (size_t)n)
                io_err_sys(ec, "io_file_write: write failed");
        return (fs_umax_t)w;
}

extern int io_file_getc(io_file_t f, fs_error_code_t *ec)
{
        int c;

        io_clear(ec);
        if (f == NULL) {
                io_err_cfs(ec, fs_cfs_error_invalid_argument, "io_file_getc: invalid argument");
                return -1;
        }
        clearerr(f->fp);
        c = fgetc(f->fp);
        if (c == EOF && ferror(f->fp))
                io_err_sys(ec, "io_file_getc: read failed");
        return c;          /* byte 0..255, or -1 at EOF */
}

extern char *io_file_gets(io_file_t f, char *buf, fs_umax_t n,
                             fs_error_code_t *ec)
{
        char *r;

        io_clear(ec);
        if (f == NULL || buf == NULL || n == 0) {
                io_err_cfs(ec, fs_cfs_error_invalid_argument, "io_file_gets: invalid argument");
                return NULL;
        }
        clearerr(f->fp);
        r = fgets(buf, (int)n, f->fp);
        if (r == NULL && ferror(f->fp))
                io_err_sys(ec, "io_file_gets: read failed");
        return r;          /* buf, or NULL at EOF */
}

extern void io_file_close(io_file_t f)
{
        if (f != NULL) {
                fclose(f->fp);
                free(f);
        }
}

extern char *io_read_file(const fs_cpath_t p, fs_umax_t *len, fs_error_code_t *ec)
{
        io_file_t f;
        long         size;
        char        *buf;
        size_t       r;

        io_clear(ec);
        if (p == NULL || len == NULL || p[0] == '\0') {
                io_err_cfs(ec, fs_cfs_error_invalid_argument, "io_read_file: invalid argument");
                return NULL;
        }
        f = io_file_open(p, io_file_mode_read, ec);
        if (f == NULL)
                return NULL;          /* fopen failed (e.g. EISDIR for a directory) */
        if (fseek(f->fp, 0, SEEK_END) != 0) {
                io_err_sys(ec, "io_read_file: seek failed");
                io_file_close(f);
                return NULL;
        }
        size = ftell(f->fp);
        if (size < 0) {
                io_err_sys(ec, "io_read_file: tell failed");
                io_file_close(f);
                return NULL;
        }
        rewind(f->fp);
        buf = malloc((size_t)size + 1);
        r = fread(buf, 1, (size_t)size, f->fp);
        io_file_close(f);
        if (r != (size_t)size) {
                io_err_sys(ec, "io_read_file: short read");
                free(buf);
                return NULL;
        }
        buf[r] = '\0';
        *len = (fs_umax_t)r;
        return buf;
}

static fs_bool_t io_put(const fs_cpath_t p, const char *buf, fs_umax_t len,
                          io_file_mode_t mode, fs_error_code_t *ec)
{
        io_file_t f;
        fs_umax_t    w;

        io_clear(ec);
        if (p == NULL || p[0] == '\0' || (buf == NULL && len != 0)) {
                io_err_cfs(ec, fs_cfs_error_invalid_argument, "fs_io: invalid argument");
                return FS_FALSE;
        }
        f = io_file_open(p, mode, ec);
        if (f == NULL)
                return FS_FALSE;
        w = io_file_write(f, buf, len, ec);
        io_file_close(f);
        if (w != len) {
                if (ec && ec->type == fs_error_type_none)
                        io_err_cfs(ec, fs_cfs_error_function_not_supported, "fs_io: short write");
                return FS_FALSE;
        }
        return FS_TRUE;
}

extern fs_bool_t io_write_file(const fs_cpath_t p, const char *buf, fs_umax_t len,
                                  fs_error_code_t *ec)
{
        return io_put(p, buf, len, io_file_mode_write, ec);
}

extern fs_bool_t io_append_file(const fs_cpath_t p, const char *buf, fs_umax_t len,
                                   fs_error_code_t *ec)
{
        return io_put(p, buf, len, io_file_mode_append, ec);
}

#ifdef __cplusplus
}
#endif

#endif /* CIO_IMPLEMENTATION_ONCE */
#endif /* CIO_IMPLEMENTATION */
