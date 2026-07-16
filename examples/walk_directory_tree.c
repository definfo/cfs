/*
 * Walk a directory tree recursively.
 *
 * The recursive iterator has the same fs_dir_iter_t shape and direct-indexing
 * loop as the flat one; FOR_EACH_ENTRY_IN_RDIR and FS_DESTROY_RDIR_ITER are
 * the recursive forms of FOR_EACH_ENTRY_IN_DIR and FS_DESTROY_DIR_ITER, and
 * both take the loop's entry variable as their first argument. (You can also
 * index it.elems[i] directly, like list_directory_entries.c.) Unlike C++
 * recursive_directory_iterator, CFS materializes the whole tree eagerly.
 */

#define CFS_IMPLEMENTATION
#include <cfs/cfs.h>
#include <stdio.h>
#include <stdlib.h>

int
main (void)
{
        fs_error_code_t ec;
        fs_recursive_dir_iter_t it;
        fs_cpath_t name;

        it = fs_recursive_directory_iterator_opt (
            FS_PATH ("strategies"),
            fs_directory_options_follow_directory_symlink, &ec);
        if (ec.type != fs_error_type_none)
                {
                        fprintf (stderr, "recursive iterator failed: %s\n",
                                 ec.msg);
                        return 1;
                }

        FOR_EACH_ENTRY_IN_RDIR (name, it)
        {
                char *display = fs_path_get (name);
                printf ("%s\n", display);
                free (display);
        }

        FS_DESTROY_RDIR_ITER (name, it);
        return 0;
}
