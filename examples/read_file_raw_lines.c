/*
 * Read raw lines, encoding-agnostic.
 *
 * One of the read strategies (see read_file_size_based.c). io_file_gets
 * reads up to a newline as a NUL-terminated char * without interpreting the
 * bytes as any character set -- safe for text where you only need line
 * boundaries.
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
        fs_path_t path;
        io_file_t f;
        char line[1024];

        if (argc < 2)
                {
                        fprintf (stderr, "usage: %s <file>\n", argv[0]);
                        return 1;
                }
        path = fs_make_path (argv[1]);
        if (path == NULL)
                {
                        fprintf (stderr, "fs_make_path failed\n");
                        return 1;
                }
        f = io_file_open (path, io_file_mode_read, &ec);
        free (path);
        if (f == NULL)
                {
                        fprintf (stderr, "open failed: %s\n",
                                 ec.msg ? ec.msg : "");
                        return 1;
                }
        while (io_file_gets (f, line, sizeof line, &ec) != NULL)
                printf ("%s", line);
        io_file_close (f);
        if (ec.type != fs_error_type_none)
                {
                        fprintf (stderr, "read error: %s\n",
                                 ec.msg ? ec.msg : "");
                        return 1;
                }
        return 0;
}
