/*
 * List directory entries.
 *
 * Self-contained example (see build_and_print_path.c for the build/TU note).
 * `it.elems` is a NULL-terminated array of borrowed entry paths; the NULL
 * terminator is the loop's stop condition, so plain indexing is enough.
 * Entries are owned by the iterator: free each, then the array itself.
 *
 * Build: cc -Iinclude examples/list_directory_entries.c -o list_directory_entries
 */

#define CFS_IMPLEMENTATION
#include <stdlib.h>
#include <stdio.h>
#include <cfs/cfs.h>

int main(void)
{
        fs_error_code_t ec;
        fs_dir_iter_t   it;
        ptrdiff_t       i;

        it = fs_directory_iterator(FS_PATH("."), &ec);
        if (ec.type != fs_error_type_none) {
                fprintf(stderr, "directory_iterator failed: %s\n", ec.msg);
                return 1;
        }

        for (i = 0; it.elems[i]; i++) {
                char *display = fs_path_get(it.elems[i]);
                printf("%s\n", display);
                free(display);
        }

        /* entries are owned by the iterator: free each, then the array itself */
        for (i = 0; it.elems[i]; i++)
                free((void *)it.elems[i]);
        free((void *)it.elems);
        return 0;
}
