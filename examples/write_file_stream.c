/*
 * Stream a file to another file in chunks -- the write counterpart of
 * read_file_stream.c. For large or unbounded content, copy in fixed chunks
 * rather than buffering everything in memory.
 *
 * cio's io_file_open/read/write/close do the byte-level I/O cfs.h omits.
 */

#define CFS_IMPLEMENTATION
#define CIO_IMPLEMENTATION
#include <cfs/cfs.h>
#include <cfs/cio.h>
#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char *argv[])
{
        fs_error_code_t ec;
        fs_path_t src, dest;
        io_file_t in, out;
        char chunk[4096];
        fs_umax_t n;

        if (argc < 3)
                {
                        fprintf (stderr, "usage: %s <src> <dest>\n", argv[0]);
                        return 1;
                }
        src = fs_make_path (argv[1]);
        dest = fs_make_path (argv[2]);
        if (src == NULL || dest == NULL)
                {
                        fprintf (stderr, "fs_make_path failed\n");
                        free (src);
                        free (dest);
                        return 1;
                }
        in = io_file_open (src, io_file_mode_read, &ec);
        free (src);
        if (in == NULL)
                {
                        fprintf (stderr, "open(src) failed: %s\n",
                                 ec.msg ? ec.msg : "");
                        free (dest);
                        return 1;
                }
        out = io_file_open (dest, io_file_mode_write, &ec);
        free (dest);
        if (out == NULL)
                {
                        fprintf (stderr, "open(dest) failed: %s\n",
                                 ec.msg ? ec.msg : "");
                        io_file_close (in);
                        return 1;
                }
        while ((n = io_file_read (in, chunk, sizeof chunk, &ec)) > 0)
                {
                        io_file_write (out, chunk, n, &ec);
                }
        io_file_close (in);
        io_file_close (out);
        if (ec.type != fs_error_type_none)
                {
                        fprintf (stderr, "copy failed: %s\n",
                                 ec.msg ? ec.msg : "");
                        return 1;
                }
        return 0;
}
