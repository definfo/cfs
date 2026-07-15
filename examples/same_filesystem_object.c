/*
 * Tell whether two paths name the same filesystem object.
 *
 * fs_equivalent replaces the realpath(p, NULL) + strcmp idiom: two paths are
 * equivalent when they resolve to the same filesystem object (same inode and
 * device, matching std::filesystem::equivalent). fs_status follows a symlink
 * (reports the target's status); fs_symlink_status does not (reports the link
 * itself). Content comparison is out of scope: fs_equivalent reports same
 * object, not equal bytes -- short-circuit on unequal fs_file_size, then
 * compare contents with io_read_file (cio) and memcmp; see
 * compare_then_write.c and write_tmpfile_compare_then_write.c.
 *
 * Build: cc -Iinclude examples/same_filesystem_object.c -o same_filesystem_object
 * Run:   ./same_filesystem_object <path-a> <path-b> [link]
 */

#define CFS_IMPLEMENTATION
#include <stdlib.h>
#include <stdio.h>
#include <cfs/cfs.h>

int main(int argc, char *argv[])
{
        fs_error_code_t ec;
        fs_path_t        a, b, link;
        fs_file_type_t   target, link_t;

        if (argc < 3) {
                fprintf(stderr, "usage: %s <path-a> <path-b> [link]\n", argv[0]);
                return 1;
        }
        a = fs_make_path(argv[1]);
        b = fs_make_path(argv[2]);
        link = fs_make_path((argc >= 4) ? argv[3] : argv[1]);
        if (a == NULL || b == NULL || link == NULL) {
                fprintf(stderr, "fs_make_path failed\n");
                free(a);
                free(b);
                free(link);
                return 1;
        }

        /* same filesystem object (inode/device), like realpath(a)+realpath(b)+strcmp */
        if (fs_equivalent(a, b, &ec))
                printf("same object\n");

        /* fs_status follows a symlink to its target; fs_symlink_status reports the
         * link itself. Both return an fs_file_status_t for type/perms queries. */
        target = fs_status(link, &ec).type;
        link_t = fs_symlink_status(link, &ec).type;
        printf("target type: %d, link type: %d\n", (int)target, (int)link_t);
        free(a);
        free(b);
        free(link);
        return 0;
}
