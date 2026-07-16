/*
 * Filesystem operation scenarios migrated from Boost.Filesystem's
 * test/operations_test.cpp (develop branch).
 *
 * The original tests are Copyright Beman Dawes 2002-2015 and distributed
 * under the Boost Software License, Version 1.0. See
 * third_party/boost/LICENSE_1_0.txt.
 *
 * This is a C11 adaptation for the CFS API, not a copy of the C++ tests.
 * Where CFS deliberately differs from std::filesystem, the assertion follows
 * the CFS contract documented in README.md.
 */

#define CFS_IMPLEMENTATION
#include "cfs/cfs.h"

#define TESTS_IMPLEMENTATION
#include "tests.h"

#define OPS_ROOT FS_PATH ("cfs-boost-operations-root")

#define EXPECT_NO_EC(ec) EXPECT_EQ ((ec).type, fs_error_type_none)
#define EXPECT_EC_SET(ec) EXPECT_NE ((ec).type, fs_error_type_none)

static void
ops_create_file (const fs_cpath_t path, const char *contents)
{
        char *native_path;
        FILE *file;

        native_path = fs_path_get (path);
        file = fopen (native_path, "w");
        if (file)
                {
                        if (contents)
                                fputs (contents, file);
                        fclose (file);
                }
        free (native_path);
}

static fs_path_t
ops_join2 (const fs_cpath_t a, const fs_cpath_t b)
{
        return fs_path_append (a, b, NULL);
}

static fs_path_t
ops_join3 (const fs_cpath_t a, const fs_cpath_t b, const fs_cpath_t c)
{
        fs_path_t out;

        out = fs_path_append (a, b, NULL);
        fs_path_append_s (&out, c, NULL);
        return out;
}

static void
ops_prepare (void)
{
        if (fs_exists (OPS_ROOT, NULL))
                fs_remove_all (OPS_ROOT, NULL);
        fs_create_directory (OPS_ROOT, NULL);
}

/*
 * resize_file_tests -- grow/shrink and the missing-file error path.
 */
TEST (boost_operations, resize_file)
{
        fs_error_code_t ec;
        fs_path_t file;

        file = ops_join2 (OPS_ROOT, FS_PATH ("resize.txt"));
        ops_create_file (file, "1234567890");
        EXPECT_TRUE (fs_exists (file, NULL));
        EXPECT_EQ (fs_file_size (file, NULL), (fs_umax_t)10);

        fs_resize_file (file, 5, &ec);
        EXPECT_NO_EC (ec);
        EXPECT_EQ (fs_file_size (file, NULL), (fs_umax_t)5);

        fs_resize_file (file, 15, &ec);
        EXPECT_NO_EC (ec);
        EXPECT_EQ (fs_file_size (file, NULL), (fs_umax_t)15);

        free (file);

        fs_resize_file (ops_join2 (OPS_ROOT, FS_PATH ("no-such-file")), 15,
                        &ec);
        EXPECT_EC_SET (ec);

        fs_remove_all (OPS_ROOT FS_PATH ("/no-such-file"), NULL);
}

/*
 * status_of_nonexistent_tests -- a nonexistent path reports file_not_found
 * without making nonexistence itself an error (std::filesystem::status
 * contract).
 */
TEST (boost_operations, status_of_nonexistent)
{
        fs_error_code_t ec;
        fs_file_status_t s;
        fs_path_t missing;

        missing = ops_join2 (OPS_ROOT, FS_PATH ("nosuch"));

        EXPECT_FALSE (fs_exists (missing, NULL));
        EXPECT_FALSE (fs_is_regular_file (missing, NULL));
        EXPECT_FALSE (fs_is_directory (missing, NULL));

        s = fs_status (missing, &ec);
        EXPECT_NO_EC (ec);
        EXPECT_EQ (s.type, fs_file_type_not_found);
        EXPECT_FALSE (fs_exists_s (s));
        EXPECT_FALSE (fs_is_regular_file_s (s));
        EXPECT_FALSE (fs_is_directory_s (s));

        free (missing);
}

/*
 * remove_tests -- remove a file, remove an empty directory, and that removing
 * a nonexistent path returns false without error.
 */
TEST (boost_operations, remove)
{
        fs_error_code_t ec;
        fs_path_t file;
        fs_path_t dir;

        file = ops_join2 (OPS_ROOT, FS_PATH ("shortlife"));
        ops_create_file (file, "");
        EXPECT_TRUE (fs_exists (file, NULL));
        EXPECT_TRUE (fs_remove (file, &ec));
        EXPECT_NO_EC (ec);
        EXPECT_FALSE (fs_exists (file, NULL));
        free (file);

        EXPECT_FALSE (
            fs_remove (ops_join2 (OPS_ROOT, FS_PATH ("no-such-file")), &ec));
        EXPECT_NO_EC (ec);
        fs_remove_all (OPS_ROOT FS_PATH ("/no-such-file"), NULL);

        dir = ops_join2 (OPS_ROOT, FS_PATH ("shortlife_dir"));
        fs_create_directory (dir, NULL);
        EXPECT_TRUE (fs_is_directory (dir, NULL));
        EXPECT_TRUE (fs_remove (dir, &ec));
        EXPECT_NO_EC (ec);
        EXPECT_FALSE (fs_exists (dir, NULL));
        free (dir);
}

/*
 * remove_all_tests -- remove_all counts removed entries and returns 0 for a
 * path that does not exist.
 */
TEST (boost_operations, remove_all)
{
        fs_error_code_t ec;
        fs_path_t file;
        fs_path_t d1;
        fs_path_t d2;
        fs_path_t nested_file;

        file = ops_join2 (OPS_ROOT, FS_PATH ("shortlife"));
        ops_create_file (file, "");
        EXPECT_EQ (fs_remove_all (file, &ec), (fs_umax_t)1);
        EXPECT_NO_EC (ec);
        EXPECT_FALSE (fs_exists (file, NULL));
        free (file);

        EXPECT_EQ (fs_remove_all (
                       ops_join2 (OPS_ROOT, FS_PATH ("no-such-file")), &ec),
                   (fs_umax_t)0);
        EXPECT_NO_EC (ec);
        fs_remove_all (OPS_ROOT FS_PATH ("/no-such-file"), NULL);

        /* a directory tree: dir/nested_dir, dir/file */
        d1 = ops_join2 (OPS_ROOT, FS_PATH ("shortlife_dir"));
        fs_create_directory (d1, NULL);
        d2 = ops_join3 (OPS_ROOT, FS_PATH ("shortlife_dir"),
                        FS_PATH ("nested_dir"));
        fs_create_directory (d2, NULL);
        nested_file = ops_join3 (OPS_ROOT, FS_PATH ("shortlife_dir"),
                                 FS_PATH ("file"));
        ops_create_file (nested_file, "");

        EXPECT_EQ (fs_remove_all (d1, &ec), (fs_umax_t)3);
        EXPECT_NO_EC (ec);
        EXPECT_FALSE (fs_exists (d1, NULL));

        free (d1);
        free (d2);
        free (nested_file);
}

/*
 * canonical_basic_tests -- canonical of an existent path resolves to
 * the absolute path, and canonical of a nonexistent path errors. CFS treats
 * empty paths as an error rather than the current path (see README.md).
 */
TEST (boost_operations, canonical)
{
        fs_error_code_t ec;
        fs_path_t target;
        fs_path_t cur;
        fs_path_t result;
        fs_path_t expected;

        target = ops_join2 (OPS_ROOT, FS_PATH ("target"));
        ops_create_file (target, "");

        /* canonical(".") is the current path */
        cur = fs_current_path (NULL);
        result = fs_canonical (FS_PATH ("."), &ec);
        EXPECT_NO_EC (ec);
        EXPECT_EQ_PATH (result, cur);
        free (result);
        free (cur);

        /* canonical of an existent directory is its absolute path */
        result = fs_canonical (OPS_ROOT, &ec);
        EXPECT_NO_EC (ec);
        EXPECT_TRUE (fs_path_is_absolute (result, NULL));
        expected = fs_absolute (OPS_ROOT, NULL);
        EXPECT_EQ_PATH (result, expected);
        free (result);
        free (expected);

        /* canonical of an existent file is its absolute path */
        result = fs_canonical (target, &ec);
        EXPECT_NO_EC (ec);
        expected = fs_absolute (target, NULL);
        EXPECT_EQ_PATH (result, expected);
        free (result);
        free (expected);

        /* canonical of a nonexistent path is an error */
        result = fs_canonical (ops_join2 (OPS_ROOT, FS_PATH ("no-such-file")),
                               &ec);
        EXPECT_EQ (result, NULL);
        EXPECT_EQ (ec.code, fs_cfs_error_no_such_file_or_directory);
        free (result);
        fs_remove_all (OPS_ROOT FS_PATH ("/no-such-file"), NULL);

        free (target);
}

int
main (void)
{
        int result;

        ops_prepare ();

        REGISTER_TEST (boost_operations, resize_file);
        REGISTER_TEST (boost_operations, status_of_nonexistent);
        REGISTER_TEST (boost_operations, remove);
        REGISTER_TEST (boost_operations, remove_all);
        REGISTER_TEST (boost_operations, canonical);

        result = RUN_ALL_TESTS ();

        if (fs_exists (OPS_ROOT, NULL))
                fs_remove_all (OPS_ROOT, NULL);
        return result;
}
