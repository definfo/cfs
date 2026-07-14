/*
 * Copy operation scenarios migrated from Boost.Filesystem's test/copy_test.cpp
 * (develop branch).
 *
 * The original test is Copyright Andrey Semashev 2020 and distributed under
 * the Boost Software License, Version 1.0. See
 * third_party/boost/LICENSE_1_0.txt.
 *
 * This is a C89 adaptation for the CFS API, not a copy of the C++ test.
 */

#define CFS_IMPLEMENTATION
#include "cfs/cfs.h"

#define TESTS_IMPLEMENTATION
#include "tests.h"

#define COPY_ROOT   FS_MAKE_PATH("cfs-boost-copy-root")
#define COPY_TARGET FS_MAKE_PATH("cfs-boost-copy-target")

#define EXPECT_NO_EC(ec) EXPECT_EQ((ec).type, fs_error_type_none)

static void copy_write_file(const fs_cpath_t path, const char *contents)
{
        char *native_path;
        FILE *file;

        native_path = fs_path_get(path);
        file = fopen(native_path, "w");
        if (file) {
                fputs(contents, file);
                fclose(file);
        }
        free(native_path);
}

static void copy_expect_file_contents(const fs_cpath_t path, const char *expected, int *ret)
{
        char  buf[64];
        char *native_path;
        FILE *file;

        memset(buf, 0, sizeof(buf));
        native_path = fs_path_get(path);
        file = fopen(native_path, "r");
        if (!file) {
                printf("%s:%d: Failure opening file\n", __FILE__, __LINE__);
                *ret = 2;
                free(native_path);
                return;
        }

        if (!fgets(buf, sizeof(buf), file))
                buf[0] = '\0';
        fclose(file);
        free(native_path);

        if (strcmp(buf, expected) != 0) {
                printf("%s:%d: Failure\nValue of: %s\nExpected: %s\n", __FILE__, __LINE__, buf, expected);
                *ret = 2;
        }
}

static fs_path_t copy_join2(const fs_cpath_t a, const fs_cpath_t b)
{
        return fs_path_append(a, b, NULL);
}

static fs_path_t copy_join3(const fs_cpath_t a, const fs_cpath_t b, const fs_cpath_t c)
{
        fs_path_t out;

        out = fs_path_append(a, b, NULL);
        fs_path_append_s(&out, c, NULL);
        return out;
}

static void copy_prepare_tree(void)
{
        fs_error_code_t ec;
        fs_path_t       path;

        if (fs_exists(COPY_ROOT, NULL))
                fs_remove_all(COPY_ROOT, NULL);
        if (fs_exists(COPY_TARGET, NULL))
                fs_remove_all(COPY_TARGET, NULL);

        fs_create_directory(COPY_ROOT, &ec);

        path = copy_join2(COPY_ROOT, FS_MAKE_PATH("f1"));
        copy_write_file(path, "f1");
        free(path);

        path = copy_join2(COPY_ROOT, FS_MAKE_PATH("f2"));
        copy_write_file(path, "f2");
        free(path);

        path = copy_join2(COPY_ROOT, FS_MAKE_PATH("d1"));
        fs_create_directory(path, NULL);
        free(path);

        path = copy_join3(COPY_ROOT, FS_MAKE_PATH("d1"), FS_MAKE_PATH("f1"));
        copy_write_file(path, "d1f1");
        free(path);

        path = copy_join3(COPY_ROOT, FS_MAKE_PATH("d1"), FS_MAKE_PATH("d1"));
        fs_create_directory(path, NULL);
        free(path);

        path = copy_join3(COPY_ROOT, FS_MAKE_PATH("d1/d1"), FS_MAKE_PATH("f1"));
        copy_write_file(path, "d1d1f1");
        free(path);

        path = copy_join3(COPY_ROOT, FS_MAKE_PATH("d1"), FS_MAKE_PATH("d2"));
        fs_create_directory(path, NULL);
        free(path);

        path = copy_join2(COPY_ROOT, FS_MAKE_PATH("d2"));
        fs_create_directory(path, NULL);
        free(path);

        path = copy_join3(COPY_ROOT, FS_MAKE_PATH("d2"), FS_MAKE_PATH("f1"));
        copy_write_file(path, "d2f1");
        free(path);
}

static void copy_reset_target(void)
{
        if (fs_exists(COPY_TARGET, NULL))
                fs_remove_all(COPY_TARGET, NULL);
}

TEST(boost_copy, file_default)
{
        fs_error_code_t ec;
        fs_path_t       src;
        fs_path_t       dst;

        copy_reset_target();
        fs_create_directory(COPY_TARGET, &ec);
        EXPECT_NO_EC(ec);

        src = copy_join2(COPY_ROOT, FS_MAKE_PATH("f1"));
        fs_copy(src, COPY_TARGET, &ec);
        EXPECT_NO_EC(ec);
        free(src);

        src = copy_join2(COPY_ROOT, FS_MAKE_PATH("f2"));
        dst = copy_join2(COPY_TARGET, FS_MAKE_PATH("f3"));
        fs_copy(src, dst, &ec);
        EXPECT_NO_EC(ec);
        free(src);
        free(dst);

        dst = copy_join2(COPY_TARGET, FS_MAKE_PATH("f1"));
        EXPECT_TRUE(fs_is_regular_file(dst, NULL));
        copy_expect_file_contents(dst, "f1", __ret);
        free(dst);

        dst = copy_join2(COPY_TARGET, FS_MAKE_PATH("f3"));
        EXPECT_TRUE(fs_is_regular_file(dst, NULL));
        copy_expect_file_contents(dst, "f2", __ret);
        free(dst);
}

TEST(boost_copy, directory_default)
{
        fs_error_code_t ec;
        fs_path_t       path;

        copy_reset_target();
        fs_copy(COPY_ROOT, COPY_TARGET, &ec);
        EXPECT_NO_EC(ec);

        path = copy_join2(COPY_TARGET, FS_MAKE_PATH("f1"));
        EXPECT_TRUE(fs_is_regular_file(path, NULL));
        copy_expect_file_contents(path, "f1", __ret);
        free(path);

        path = copy_join2(COPY_TARGET, FS_MAKE_PATH("f2"));
        EXPECT_TRUE(fs_is_regular_file(path, NULL));
        copy_expect_file_contents(path, "f2", __ret);
        free(path);

        path = copy_join2(COPY_TARGET, FS_MAKE_PATH("d1"));
        EXPECT_TRUE(fs_is_directory(path, NULL));
        free(path);

        path = copy_join3(COPY_TARGET, FS_MAKE_PATH("d1"), FS_MAKE_PATH("f1"));
        EXPECT_FALSE(fs_exists(path, NULL));
        free(path);
}

TEST(boost_copy, directory_recursive)
{
        fs_error_code_t ec;
        fs_path_t       path;

        copy_reset_target();
        fs_copy_opt(COPY_ROOT, COPY_TARGET, fs_copy_options_recursive, &ec);
        EXPECT_NO_EC(ec);

        path = copy_join3(COPY_TARGET, FS_MAKE_PATH("d1"), FS_MAKE_PATH("f1"));
        EXPECT_TRUE(fs_is_regular_file(path, NULL));
        copy_expect_file_contents(path, "d1f1", __ret);
        free(path);

        path = copy_join3(COPY_TARGET, FS_MAKE_PATH("d1/d1"), FS_MAKE_PATH("f1"));
        EXPECT_TRUE(fs_is_regular_file(path, NULL));
        copy_expect_file_contents(path, "d1d1f1", __ret);
        free(path);

        path = copy_join3(COPY_TARGET, FS_MAKE_PATH("d2"), FS_MAKE_PATH("f1"));
        EXPECT_TRUE(fs_is_regular_file(path, NULL));
        copy_expect_file_contents(path, "d2f1", __ret);
        free(path);
}

TEST(boost_copy, directory_recursive_directories_only)
{
        const fs_copy_options_t opts = fs_copy_options_recursive | fs_copy_options_directories_only;

        fs_error_code_t ec;
        fs_path_t       path;

        copy_reset_target();
        fs_copy_opt(COPY_ROOT, COPY_TARGET, opts, &ec);
        EXPECT_NO_EC(ec);

        path = copy_join2(COPY_TARGET, FS_MAKE_PATH("d1"));
        EXPECT_TRUE(fs_is_directory(path, NULL));
        free(path);

        path = copy_join3(COPY_TARGET, FS_MAKE_PATH("d1"), FS_MAKE_PATH("d1"));
        EXPECT_TRUE(fs_is_directory(path, NULL));
        free(path);

        path = copy_join3(COPY_TARGET, FS_MAKE_PATH("d1"), FS_MAKE_PATH("f1"));
        EXPECT_FALSE(fs_exists(path, NULL));
        free(path);

        path = copy_join2(COPY_TARGET, FS_MAKE_PATH("f1"));
        EXPECT_FALSE(fs_exists(path, NULL));
        free(path);
}

TEST(boost_copy, errors)
{
        fs_error_code_t ec;
        fs_path_t       src;
        fs_path_t       dst;

        copy_reset_target();
        fs_create_directory(COPY_TARGET, &ec);
        EXPECT_NO_EC(ec);

        src = copy_join2(COPY_ROOT, FS_MAKE_PATH("non-existing"));
        fs_copy(src, COPY_TARGET, &ec);
        EXPECT_EQ(ec.code, fs_cfs_error_no_such_file_or_directory);
        free(src);

        dst = copy_join2(COPY_TARGET, FS_MAKE_PATH("f1"));
        copy_write_file(dst, "existing");

        src = copy_join2(COPY_ROOT, FS_MAKE_PATH("f1"));
        fs_copy(src, COPY_TARGET, &ec);
        EXPECT_NE(ec.type, fs_error_type_none);
        fs_copy(src, dst, &ec);
        EXPECT_NE(ec.type, fs_error_type_none);
        free(src);
        free(dst);

        fs_copy(COPY_TARGET, COPY_TARGET, &ec);
        EXPECT_NE(ec.type, fs_error_type_none);
}

int main(void)
{
        int result;

        copy_prepare_tree();

        REGISTER_TEST(boost_copy, file_default);
        REGISTER_TEST(boost_copy, directory_default);
        REGISTER_TEST(boost_copy, directory_recursive);
        REGISTER_TEST(boost_copy, directory_recursive_directories_only);
        REGISTER_TEST(boost_copy, errors);
        result = RUN_ALL_TESTS();

        if (fs_exists(COPY_TARGET, NULL))
                fs_remove_all(COPY_TARGET, NULL);
        if (fs_exists(COPY_ROOT, NULL))
                fs_remove_all(COPY_ROOT, NULL);
        return result;
}
