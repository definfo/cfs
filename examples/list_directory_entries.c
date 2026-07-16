/*
 * List directory entries.
 *
 * `it.elems` is a NULL-terminated array of borrowed entry paths; the NULL
 * terminator is the loop's stop condition, so plain indexing is enough.
 * Entries are owned by the iterator: free each, then the array itself.
 */

#define CFS_IMPLEMENTATION
#include <cfs/cfs.h>
#include <stdio.h>
#include <stdlib.h>

int
main (void)
{
        fs_error_code_t ec;
        fs_dir_iter_t it;
        ptrdiff_t i;

        it = fs_directory_iterator (FS_PATH ("."), &ec);
        if (ec.type != fs_error_type_none)
                {
                        fprintf (stderr, "directory_iterator failed: %s\n",
                                 ec.msg);
                        return 1;
                }

        for (i = 0; it.elems[i]; i++)
                {
                        char *display = fs_path_get (it.elems[i]);
                        printf ("%s\n", display);
                        free (display);
                }

        /* entries are owned by the iterator: free each, then the array itself
         */
        for (i = 0; it.elems[i]; i++)
                free ((void *)it.elems[i]);
        free ((void *)it.elems);
        return 0;
}
