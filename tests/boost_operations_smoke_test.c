/*
 * Smoke-test scenarios migrated from Boost.Filesystem's
 * test/operations_unit_test.cpp (develop branch).
 *
 * The original test is Copyright Beman Dawes 2008, 2009, 2015 and is
 * distributed under the Boost Software License, Version 1.0. See
 * third_party/boost/LICENSE_1_0.txt.
 *
 * This is a C11 adaptation for the CFS API, not a copy of the C++ test.
 */

#define CFS_IMPLEMENTATION
#include "cfs/cfs.h"

#define TESTS_IMPLEMENTATION
#include "tests.h"

#define BOOST_SMOKE_ROOT FS_MAKE_PATH("cfs-boost-operations-smoke")

#define EXPECT_NO_EC(ec) EXPECT_EQ((ec).type, fs_error_type_none)

static void create_file(const fs_cpath_t path)
{
        char *native_path;
        FILE *file;

        native_path = fs_path_get(path);
        file = fopen(native_path, "w");
        if (file)
                fclose(file);
        free(native_path);
}

TEST(boost_operations, status_and_queries)
{
        fs_error_code_t  ec;
        fs_file_status_t status;
        fs_umax_t        size;

        status = fs_status(FS_MAKE_PATH("."), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_TRUE(fs_is_directory_s(status));
        EXPECT_TRUE(fs_exists_s(status));

        status = fs_symlink_status(FS_MAKE_PATH("."), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_TRUE(fs_is_directory_s(status));

        size = fs_file_size(FS_MAKE_PATH("no-such-file"), &ec);
        EXPECT_EQ(size, (fs_umax_t)-1);
        EXPECT_EQ(ec.type, fs_error_type_cfs);
        EXPECT_EQ(ec.code, fs_cfs_error_no_such_file_or_directory);

        EXPECT_TRUE(fs_exists(FS_MAKE_PATH("."), &ec));
        EXPECT_NO_EC(ec);
        EXPECT_FALSE(fs_exists(FS_MAKE_PATH("no-such-file"), &ec));
        EXPECT_NO_EC(ec);
}

TEST(boost_operations, directory_iterators)
{
        fs_error_code_t ec;
        fs_dir_iter_t   it;
        ptrdiff_t       i;
        int             entries;

        it = fs_directory_iterator(FS_MAKE_PATH("."), &ec);
        EXPECT_NO_EC(ec);

        entries = 0;
        for (i = 0; it.elems[i]; i++) {
                EXPECT_TRUE(fs_path_has_filename(it.elems[i], NULL));
                ++entries;
        }
        EXPECT_EQ(entries, 2);
        for (i = 0; it.elems[i]; i++)
                free((void *)it.elems[i]);
        free((void *)it.elems);

        it = fs_recursive_directory_iterator(FS_MAKE_PATH("."), &ec);
        EXPECT_NO_EC(ec);

        entries = 0;
        for (i = 0; it.elems[i]; i++) {
                EXPECT_TRUE(fs_path_has_filename(it.elems[i], NULL));
                ++entries;
        }
        EXPECT_EQ(entries, 3);
        for (i = 0; it.elems[i]; i++)
                free((void *)it.elems[i]);
        free((void *)it.elems);
}

TEST(boost_operations, space_and_equivalent)
{
        fs_error_code_t ec;
        fs_space_info_t info;

        info = fs_space(FS_MAKE_PATH("."), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_TRUE(info.capacity >= info.free);
        EXPECT_TRUE(info.free >= info.available);

        EXPECT_TRUE(fs_equivalent(FS_MAKE_PATH("."), FS_MAKE_PATH("."), &ec));
        EXPECT_NO_EC(ec);
        EXPECT_FALSE(fs_equivalent(FS_MAKE_PATH("."), FS_MAKE_PATH("file"), &ec));
        EXPECT_NO_EC(ec);
}

int main(void)
{
        fs_error_code_t ec;
        int             result;

        if (fs_exists(BOOST_SMOKE_ROOT, NULL))
                fs_remove_all(BOOST_SMOKE_ROOT, NULL);

        if (!fs_create_directory(BOOST_SMOKE_ROOT, &ec))
                return EXIT_FAILURE;
        fs_set_current_path(BOOST_SMOKE_ROOT, &ec);
        if (ec.type != fs_error_type_none)
                return EXIT_FAILURE;

        create_file(FS_MAKE_PATH("file"));
        fs_create_directory(FS_MAKE_PATH("directory"), &ec);
        if (ec.type != fs_error_type_none)
                return EXIT_FAILURE;
        create_file(FS_MAKE_PATH("directory/nested-file"));

        REGISTER_TEST(boost_operations, status_and_queries);
        REGISTER_TEST(boost_operations, directory_iterators);
        REGISTER_TEST(boost_operations, space_and_equivalent);
        result = RUN_ALL_TESTS();

        fs_set_current_path(FS_MAKE_PATH(".."), NULL);
        fs_remove_all(BOOST_SMOKE_ROOT, NULL);
        return result;
}
