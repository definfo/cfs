/*
 * Build and print a path.
 *
 * Self-contained example: defines CFS_IMPLEMENTATION in its one translation
 * unit. In a real project, define CFS_IMPLEMENTATION in exactly one TU and
 * include <cfs/cfs.h> normally elsewhere (see README "Include discipline").
 */

#define CFS_IMPLEMENTATION
#include <cfs/cfs.h>
#include <stdio.h>
#include <stdlib.h>

int
main (void)
{
        fs_error_code_t ec;
        fs_path_t path;
        char *display;

        path
            = fs_path_append (FS_PATH ("output"), FS_PATH ("report.txt"), &ec);
        if (ec.type != fs_error_type_none)
                {
                        fprintf (stderr, "path error: %s\n", ec.msg);
                        return 1;
                }

        display = fs_path_get (path);
        printf ("%s\n", display);

        free (display);
        free (path);
        return 0;
}
