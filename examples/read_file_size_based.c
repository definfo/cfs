/*
 * Read a whole file's bytes (whole-file read).
 *
 * cio provides the byte-level read cfs.h omits: cfs.h mirrors C++
 * std::filesystem (path + object-level ops only; it does not read file
 * contents). io_read_file opens, sizes, and reads the file into one
 * malloc'd, NUL-terminated buffer, and fails cleanly on a non-openable path
 * (e.g. a directory -> EISDIR). The bytes are written to stdout here.
 */

#define CFS_IMPLEMENTATION
#define CIO_IMPLEMENTATION
#include <cfs/cfs.h>
#include <cfs/cio.h>
#include <stdio.h>
#include <stdlib.h>

static void
put_bytes (const char *buf, fs_umax_t n)
{
        for (fs_umax_t i = 0; i < n; i++)
                putchar ((unsigned char)buf[i]);
}

int
main (int argc, char *argv[])
{
        fs_error_code_t ec;
        fs_path_t p;
        fs_umax_t len = 0;
        char *buf;

        if (argc < 2)
                {
                        fprintf (stderr, "usage: %s <file>\n", argv[0]);
                        return 1;
                }
        p = fs_make_path (argv[1]);
        if (p == NULL)
                {
                        fprintf (stderr, "fs_make_path failed\n");
                        return 1;
                }

        buf = io_read_file (
            p, &len, &ec); /* whole-file read (malloc'd, NUL-terminated) */
        free (p);
        if (buf == NULL)
                {
                        fprintf (stderr, "io_read_file failed: %s\n",
                                 ec.msg ? ec.msg : "");
                        return 1;
                }
        put_bytes (buf, len);
        free (buf);
        return 0;
}
