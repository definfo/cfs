/*
 * Copy a tree recursively, replacing existing files.
 * NOTE: this example creates directory before copying.
 *
 * Build: cc -Iinclude examples/copy_tree.c -o copy_tree
 */

#define CFS_IMPLEMENTATION
#include <stdlib.h>
#include <stdio.h>
#include <cfs/cfs.h>

int main(void)
{
        fs_error_code_t  ec;
        fs_copy_options_t options;

        options = fs_copy_options_recursive | fs_copy_options_overwrite_existing;
        fs_create_directories(FS_PATH("backup"), &ec);
        fs_copy_opt(FS_PATH("assets"), FS_PATH("backup/assets"), options, &ec);
        if (ec.type != fs_error_type_none) {
                fprintf(stderr, "copy failed: %s\n", ec.msg);
                return 1;
        }
        return 0;
}
