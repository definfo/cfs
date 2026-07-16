/*
 * Compare an in-memory string buffer against a file on disk, then write.
 *
 * Avoids rewriting a file whose content is unchanged: compare the buffer (the
 * new content, in memory) against the target file's current bytes, streamed in
 * fixed-size chunks, and overwrite the target only if they differ (or the
 * target is missing). A text-mode variant could compare `io_file_gets`
 * results line by line. cio does the byte-level read/write cfs.h omits.
 */

#define CFS_IMPLEMENTATION
#define CIO_IMPLEMENTATION
#include <cfs/cfs.h>
#include <cfs/cio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Compare an in-memory buffer with a binary file without loading the file. */
static fs_bool_t
contents_equal (const char *buf, fs_umax_t len, fs_cpath_t path,
                fs_error_code_t *ec)
{
        io_file_t f;
        char chunk[4096];
        fs_umax_t n, offset = 0;
        fs_bool_t equal = FS_FALSE;

        f = io_file_open (path, io_file_mode_read, ec);
        if (f == NULL)
                return FS_FALSE;

        for (;;)
                {
                        n = io_file_read (f, chunk, sizeof chunk, ec);
                        if (ec && ec->type != fs_error_type_none)
                                break;
                        if (n == 0)
                                {
                                        equal = offset == len;
                                        break;
                                }
                        if (offset > len || n > len - offset
                            || memcmp (buf + (size_t)offset, chunk, (size_t)n)
                                   != 0)
                                break;
                        offset += n;
                }

        io_file_close (f);
        return equal;
}

int
main (int argc, char *argv[])
{
        fs_error_code_t ec;
        fs_path_t target;
        const char *buf; /* the in-memory string buffer (new content) */
        fs_umax_t len;
        fs_bool_t exists, equal;

        if (argc < 3)
                {
                        fprintf (stderr, "usage: %s <target> <content>\n",
                                 argv[0]);
                        return 1;
                }
        target = fs_make_path (argv[1]);
        if (target == NULL)
                {
                        fprintf (stderr, "fs_make_path failed\n");
                        return 1;
                }
        buf = argv[2];
        len = (fs_umax_t)strlen (buf);

        exists = fs_exists (target, &ec);
        if (ec.type != fs_error_type_none)
                {
                        fprintf (stderr, "exists(target) failed: %s\n",
                                 ec.msg ? ec.msg : "");
                        free (target);
                        return 1;
                }
        if (exists)
                {
                        equal = contents_equal (buf, len, target, &ec);
                        if (ec.type != fs_error_type_none)
                                {
                                        fprintf (
                                            stderr,
                                            "compare(target) failed: %s\n",
                                            ec.msg ? ec.msg : "");
                                        free (target);
                                        return 1;
                                }
                        if (equal)
                                {
                                        printf ("unchanged; not rewriting\n");
                                        free (target);
                                        return 0;
                                }
                }

        if (!io_write_file (target, buf, len, &ec))
                {
                        fprintf (stderr, "write failed: %s\n",
                                 ec.msg ? ec.msg : "");
                        free (target);
                        return 1;
                }
        printf ("wrote %llu bytes\n", (unsigned long long)len);
        free (target);
        return 0;
}
