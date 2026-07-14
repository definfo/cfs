/*
 * Long-path scenarios migrated from Boost.Filesystem's test/long_path_test.cpp
 * (develop branch).
 *
 * The original test is Copyright Beman Dawes 2011 and distributed under the
 * Boost Software License, Version 1.0. See
 * third_party/boost/LICENSE_1_0.txt.
 *
 * This is a C11 adaptation for the CFS API, not a copy of the C++ test.
 *
 * Boost builds two paths that exceed Windows MAX_PATH (260) -- one without
 * and one with the "\\?\" escape -- then create_directory/exists both. CFS
 * transparently adds "\\?\" to paths longer than MAX_PATH on Windows (see
 * README "Differences with std::filesystem"), so the user never supplies the
 * "\\?\" prefix; CFS owns that. We exercise the two input shapes CFS supports:
 * a long RELATIVE path (CFS absolutizes, then escapes) and a long ABSOLUTE
 * path (CFS escapes directly), asserting create_directories/exists/status/
 * canonical -- all through CFS, mirroring Boost's create_directory+exists
 * scope. CFS has no file-content API, so -- like Boost -- no regular file is
 * created, only the directory tree.
 */

#define CFS_IMPLEMENTATION
#include "cfs/cfs.h"

#define TESTS_IMPLEMENTATION
#include "tests.h"

#include <stdlib.h>

#define EXPECT_NO_EC(ec) EXPECT_EQ((ec).type, fs_error_type_none)

/* A single path component kept under NAME_MAX (255) on all platforms; two of
 * them push the TOTAL path length past MAX_PATH (260). Per the Boost test's
 * "260 - prefix - 100" sizing. */
#define LONG_COMPONENT_LEN 140

static fs_path_t make_long_component(char c)
{
        char      *buf;
        fs_path_t  p;
        size_t     i;

        buf = malloc(LONG_COMPONENT_LEN + 1);
        if (!buf)
                return NULL;
        for (i = 0; i < LONG_COMPONENT_LEN; i++)
                buf[i] = c;
        buf[LONG_COMPONENT_LEN] = '\0';
        p = fs_make_path(buf);
        free(buf);
        return p;
}

static fs_path_t join3(fs_cpath_t a, fs_cpath_t b, fs_cpath_t c)
{
        fs_error_code_t ec;
        fs_path_t       p, q;

        p = fs_path_append(a, b, &ec);
        if (ec.type != fs_error_type_none) {
                free(p);
                return NULL;
        }
        q = fs_path_append(p, c, &ec);
        free(p);
        if (ec.type != fs_error_type_none) {
                free(q);
                return NULL;
        }
        return q;
}

TEST(boost_long_path, no_escape)
{
        fs_error_code_t  ec = {0};
        fs_path_t        root, comp1, comp2, path, canon;
        fs_file_status_t st;

        root  = fs_path_append(FS_MAKE_PATH("cfs-boost-longpath"),
                               FS_MAKE_PATH("no-escape"), &ec);
        comp1 = make_long_component('x');
        comp2 = make_long_component('y');
        path  = join3(root, comp1, comp2);
        free(root);
        free(comp1);
        free(comp2);
        EXPECT_TRUE(path != NULL);

        fs_create_directories(path, &ec); /* whole tree, incl. >MAX_PATH depth */
        EXPECT_NO_EC(ec);
        EXPECT_TRUE(fs_exists(path, &ec) == FS_TRUE);

        st = fs_status(path, &ec);
        EXPECT_EQ(st.type, fs_file_type_directory);
        EXPECT_TRUE(fs_is_directory(path, NULL));

        canon = fs_canonical(path, &ec); /* canonicalize a >MAX_PATH path */
        EXPECT_TRUE(canon != NULL);
        free(canon);

        free(path);
}

TEST(boost_long_path, absolute)
{
        fs_error_code_t  ec = {0};
        fs_path_t        cwd, root, comp1, comp2, path, canon;
        fs_file_status_t st;

        cwd = fs_current_path(&ec); /* anchor an absolute base */
        EXPECT_TRUE(cwd != NULL);
        root  = fs_path_append(cwd, FS_MAKE_PATH("cfs-boost-longpath-absolute"), &ec);
        free(cwd);
        comp1 = make_long_component('x');
        comp2 = make_long_component('y');
        path  = join3(root, comp1, comp2);
        free(root);
        free(comp1);
        free(comp2);
        EXPECT_TRUE(path != NULL);

        fs_create_directories(path, &ec); /* long ABSOLUTE path -> CFS escapes directly */
        EXPECT_NO_EC(ec);
        EXPECT_TRUE(fs_exists(path, &ec) == FS_TRUE);

        st = fs_status(path, &ec);
        EXPECT_EQ(st.type, fs_file_type_directory);
        EXPECT_TRUE(fs_is_directory(path, NULL));

        canon = fs_canonical(path, &ec);
        EXPECT_TRUE(canon != NULL);
        free(canon);

        free(path);
}

int main(void)
{
        int       result;
        fs_path_t rel_root, cwd, abs_root;

        REGISTER_TEST(boost_long_path, no_escape);
        REGISTER_TEST(boost_long_path, absolute);
        result = RUN_ALL_TESTS();

        rel_root = fs_path_append(FS_MAKE_PATH("cfs-boost-longpath"),
                                  FS_MAKE_PATH("no-escape"), NULL);
        if (rel_root && fs_exists(rel_root, NULL))
                fs_remove_all(rel_root, NULL);
        free(rel_root);
        if (fs_exists(FS_MAKE_PATH("cfs-boost-longpath"), NULL))
                fs_remove_all(FS_MAKE_PATH("cfs-boost-longpath"), NULL);

        cwd = fs_current_path(NULL);
        abs_root = fs_path_append(cwd, FS_MAKE_PATH("cfs-boost-longpath-absolute"), NULL);
        free(cwd);
        if (abs_root && fs_exists(abs_root, NULL))
                fs_remove_all(abs_root, NULL);
        free(abs_root);
        return result;
}
