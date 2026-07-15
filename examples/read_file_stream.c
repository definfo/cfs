/*
 * Stream a file to stdout in chunks -- for non-seekable / unbounded input.
 *
 * One of the read strategies (see read_file_size_based.c). No io_read_file
 * here: pipes, fifos, sockets report size 0 and are not seekable, so stream
 * until EOF. cio's io_file_open/read/close do the byte-level I/O cfs.h
 * omits.
 *
 * Build: cc -Iinclude examples/read_file_stream.c -o read_file_stream
 */

#define CFS_IMPLEMENTATION
#define CIO_IMPLEMENTATION
#include <stdlib.h>
#include <stdio.h>
#include <cfs/cfs.h>
#include <cfs/cio.h>

static void put_bytes(const char *buf, fs_umax_t n)
{
        for (fs_umax_t i = 0; i < n; i++)
                putchar((unsigned char)buf[i]);
}

int main(int argc, char *argv[])
{
        fs_error_code_t ec;
        fs_path_t        path;
        io_file_t    f;
        char            chunk[4096];
        fs_umax_t       n;

        if (argc < 2) {
                fprintf(stderr, "usage: %s <file>\n", argv[0]);
                return 1;
        }
        path = fs_make_path(argv[1]);
        if (path == NULL) {
                fprintf(stderr, "fs_make_path failed\n");
                return 1;
        }
        f = io_file_open(path, io_file_mode_read, &ec);
        free(path);
        if (f == NULL) {
                fprintf(stderr, "open failed: %s\n", ec.msg ? ec.msg : "");
                return 1;
        }
        while ((n = io_file_read(f, chunk, sizeof chunk, &ec)) > 0)
                put_bytes(chunk, n);
        io_file_close(f);
        if (ec.type != fs_error_type_none) {
                fprintf(stderr, "read error: %s\n", ec.msg ? ec.msg : "");
                return 1;
        }
        return 0;
}
