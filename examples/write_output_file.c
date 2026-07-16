/*
 * Write an output file, backing up or overwriting an existing one.
 *
 * cfs.h handles the object-level decisions around a write -- ensure the
 * parent with fs_create_directories, snapshot a collision with
 * fs_copy_file_opt, and remove for an overwrite; cio's io_write_file /
 * io_append_file do the byte-level write cfs.h omits.
 */

#define CFS_IMPLEMENTATION
#define CIO_IMPLEMENTATION
#include <cfs/cfs.h>
#include <cfs/cio.h>

int
main (void)
{
        fs_error_code_t ec;
        fs_path_t out, backup_path;
        fs_bool_t append = FS_FALSE;
        const char *content = "...\n";
        fs_umax_t clen;

        fs_create_directories (FS_PATH ("output"), &ec);
        out = fs_path_append (FS_PATH ("output"), FS_PATH ("report.txt"), &ec);
        backup_path
            = fs_path_append (FS_PATH ("output"), FS_PATH ("report.bak"), &ec);

        if (fs_exists (out, &ec) && !append) /* snapshot before overwrite */
                fs_copy_file_opt (out, backup_path,
                                  fs_copy_options_overwrite_existing, &ec);

        if (fs_exists (out, &ec)
            && !append) /* overwrite == remove, then write */
                fs_remove (out, &ec);

        /* byte write via cio (cfs.h handles only object-level) */
        clen = (fs_umax_t)strlen (content);
        if (append)
                io_append_file (out, content, clen, &ec);
        else
                io_write_file (out, content, clen, &ec);
        if (ec.type != fs_error_type_none)
                {
                        fprintf (stderr, "write failed: %s\n",
                                 ec.msg ? ec.msg : "");
                        free (out);
                        free (backup_path);
                        return 1;
                }

        free (out);
        free (backup_path);
        return 0;
}
