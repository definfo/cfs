/*
 * Write an in-memory buffer to a temp file, compare, then commit -- the
 * atomic-update pattern.
 *
 * Writes the new content (an in-memory string buffer) to a temp file beside
 * the target first, so a crash or write error never leaves the target
 * half-written. It then stream-compares the temp file's bytes with the
 * target's: if equal, the temp is discarded (no rewrite); if different or the
 * target is missing, the temp is atomically renamed over the target
 * (fs_rename), so readers see either the old or the new file, never a partial
 * one. A text-mode variant could compare `io_file_gets` results line by
 * line. cio does the byte-level read/write; cfs.h supplies paths,
 * existence, rename, and remove.
 *
 * Build: cc -Iinclude examples/write_tmpfile_compare_then_write.c -o write_tmpfile_compare_then_write
 * Run:   ./write_tmpfile_compare_then_write <target> <content>
 */

#define CFS_IMPLEMENTATION
#define CIO_IMPLEMENTATION
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cfs/cfs.h>
#include <cfs/cio.h>

/* Stream an in-memory buffer into a binary file. */
static fs_bool_t write_buffer(fs_cpath_t path, const char *buf, fs_umax_t len,
                              fs_error_code_t *ec)
{
        io_file_t f;
        fs_umax_t    n, offset = 0;

        f = io_file_open(path, io_file_mode_write, ec);
        if (f == NULL)
                return FS_FALSE;

        while (offset < len) {
                n = io_file_write(f, buf + (size_t)offset, len - offset, ec);
                if ((ec && ec->type != fs_error_type_none) || n == 0) {
                        io_file_close(f);
                        return FS_FALSE;
                }
                offset += n;
        }
        io_file_close(f);
        return FS_TRUE;
}

/* Compare two binary files as streams without loading either whole file. */
static fs_bool_t contents_equal(fs_cpath_t a_path, fs_cpath_t b_path,
                                fs_error_code_t *ec)
{
        io_file_t a, b;
        char         a_buf[4096], b_buf[4096];
        fs_umax_t    a_len, b_len;
        fs_bool_t    equal = FS_FALSE;

        a = io_file_open(a_path, io_file_mode_read, ec);
        if (a == NULL)
                return FS_FALSE;
        b = io_file_open(b_path, io_file_mode_read, ec);
        if (b == NULL) {
                io_file_close(a);
                return FS_FALSE;
        }

        for (;;) {
                a_len = io_file_read(a, a_buf, sizeof a_buf, ec);
                if (ec && ec->type != fs_error_type_none)
                        break;
                b_len = io_file_read(b, b_buf, sizeof b_buf, ec);
                if (ec && ec->type != fs_error_type_none)
                        break;
                if (a_len != b_len)
                        break;
                if (a_len == 0) {
                        equal = FS_TRUE;
                        break;
                }
                if (memcmp(a_buf, b_buf, (size_t)a_len) != 0)
                        break;
        }

        io_file_close(a);
        io_file_close(b);
        return equal;
}

int main(int argc, char *argv[])
{
        fs_error_code_t ec;
        fs_path_t        target, tmp, parent;
        const char      *buf;        /* the in-memory string buffer (new content) */
        fs_umax_t        len;
        fs_bool_t        exists, equal = FS_FALSE;

        if (argc < 3) {
                fprintf(stderr, "usage: %s <target> <content>\n", argv[0]);
                return 1;
        }
        target = fs_make_path(argv[1]);
        if (target == NULL) {
                fprintf(stderr, "fs_make_path failed\n");
                return 1;
        }
        buf = argv[2];
        len = (fs_umax_t)strlen(buf);

        /* ensure parent exists; temp beside the target (same fs -> atomic rename) */
        parent = fs_path_parent_path(target, &ec);
        if (parent && parent[0] != '\0')
                fs_create_directories(parent, &ec);
        free(parent);
        tmp = fs_path_concat(target, FS_PATH(".tmp"), &ec);   /* target.tmp */
        if (tmp == NULL) {
                fprintf(stderr, "tmp path failed\n");
                free(target);
                return 1;
        }

        /* 1. write the buffer to the temp file (target untouched) */
        if (!write_buffer(tmp, buf, len, &ec)) {
                fprintf(stderr, "write(temp) failed: %s\n", ec.msg ? ec.msg : "");
                fs_remove(tmp, NULL);
                free(tmp);
                free(target);
                return 1;
        }

        /* 2. compare the temp and target as binary streams */
        exists = fs_exists(target, &ec);
        if (ec.type != fs_error_type_none) {
                fprintf(stderr, "exists(target) failed: %s\n", ec.msg ? ec.msg : "");
                fs_remove(tmp, NULL);
                free(tmp);
                free(target);
                return 1;
        }
        if (exists) {
                equal = contents_equal(tmp, target, &ec);
                if (ec.type != fs_error_type_none) {
                        fprintf(stderr, "compare failed: %s\n", ec.msg ? ec.msg : "");
                        fs_remove(tmp, NULL);
                        free(tmp);
                        free(target);
                        return 1;
                }
        }

        if (equal) {
                printf("unchanged; discarding temp\n");
                fs_remove(tmp, &ec);
                if (ec.type != fs_error_type_none) {
                        fprintf(stderr, "remove(temp) failed: %s\n", ec.msg ? ec.msg : "");
                        free(tmp);
                        free(target);
                        return 1;
                }
        } else {
                /* 3. atomically commit: rename temp over target */
                fs_rename(tmp, target, &ec);
                if (ec.type != fs_error_type_none) {
                        fprintf(stderr, "rename failed: %s\n", ec.msg ? ec.msg : "");
                        fs_remove(tmp, NULL);
                        free(tmp);
                        free(target);
                        return 1;
                }
                printf("committed %llu bytes\n", (unsigned long long)len);
        }

        free(tmp);
        free(target);
        return 0;
}
