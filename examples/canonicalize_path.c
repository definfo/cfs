/*
 * Convert a user-supplied char * path and canonicalize it.
 *
 * Self-contained example (see build_and_print_path.c for the build/TU note).
 * fs_make_path widens a narrow char* on Windows (mbstowcs) and returns an
 * owned path; fs_canonical borrows its argument and returns a new owned path,
 * so free the pre-canonical copy separately.
 *
 * Build: cc -Iinclude examples/canonicalize_path.c -o canonicalize_path
 * Run:   ./canonicalize_path some/relative/path
 */

#define CFS_IMPLEMENTATION
#include <stdlib.h>
#include <stdio.h>
#include <cfs/cfs.h>

int main(int argc, char *argv[])
{
        fs_error_code_t ec;
        fs_path_t       raw, p;
        char           *narrow;

        if (argc < 2) {
                fprintf(stderr, "usage: %s <path>\n", argv[0]);
                return 1;
        }

        raw = fs_make_path(argv[1]);
        p = fs_canonical(raw, &ec);      /* resolve symlinks and . / ..; raw is borrowed */
        free(raw);                       /* free the pre-canonical copy */

        if (ec.type != fs_error_type_none) {
                fprintf(stderr, "canonical failed: %s\n", ec.msg);
                free(p);
                return 1;
        }

        narrow = fs_path_get(p);         /* native path -> narrow char* for output */
        printf("%s\n", narrow);

        free(narrow);
        free(p);
        return 0;
}
