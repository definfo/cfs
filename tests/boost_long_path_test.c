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
 * Boost builds two paths that straddle/exceed Windows MAX_PATH (260):
 *   - x_p : a normal path (no "\\?\" escape), and
 *   - y_p : a "\\?\"-prefixed path whose total length exceeds 260,
 * then create_directory/exists both. CFS transparently adds "\\?\" to paths
 * longer than MAX_PATH on Windows (see README "Differences with std::filesystem"),
 * so the no-escape case is the cross-platform regression (it also exercises
 * CFS's auto-escape on Windows); the with-escape case mirrors Boost's y_p and
 * is Windows-only.
 */

#define CFS_IMPLEMENTATION
#include "cfs/cfs.h"

#define TESTS_IMPLEMENTATION
#include "tests.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
        fs_error_code_t  ec;
        fs_path_t        root, comp1, comp2, path, file, canon;
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

        canon = fs_canonical(path, &ec); /* canonicalize a >MAX_PATH path */
        EXPECT_TRUE(canon != NULL);
        free(canon);

        file = fs_path_append(path, FS_MAKE_PATH("inside.txt"), &ec);
        EXPECT_TRUE(file != NULL);
        {
                char *n = fs_path_get(file);
                FILE *f = n ? fopen(n, "w") : NULL;
                if (f) {
                        fputc('a', f);
                        fclose(f);
                }
                free(n);
        }
        EXPECT_TRUE(fs_exists(file, &ec) == FS_TRUE);
        EXPECT_TRUE(fs_file_size(file, &ec) > 0);

        free(file);
        free(path);
}

#ifdef _WIN32
/* "\\?\" requires an absolute path; anchor on the current working directory.
 * The escaped base is built from a narrow string because fs_path_append would
 * drop an absolute right-hand side under std::filesystem semantics. */
static fs_path_t make_escaped_base(void)
{
        fs_error_code_t ec;
        fs_path_t       cwd, base;
        char           *n_cwd, *escaped;
        size_t          len;

        cwd = fs_current_path(&ec);
        if (cwd == NULL)
                return NULL;
        n_cwd = fs_path_get(cwd);
        free(cwd);
        if (n_cwd == NULL)
                return NULL;

        len = 4 + strlen(n_cwd) + strlen("\\cfs-boost-longpath-escape") + 1;
        escaped = malloc(len);
        if (!escaped) {
                free(n_cwd);
                return NULL;
        }
        strcpy(escaped, "\\\\?\\");
        strcat(escaped, n_cwd);
        strcat(escaped, "\\cfs-boost-longpath-escape");
        free(n_cwd);

        base = fs_make_path(escaped);
        free(escaped);
        return base;
}

TEST(boost_long_path, with_escape)
{
        fs_error_code_t ec;
        fs_path_t       base, comp1, comp2, path;

        base  = make_escaped_base();
        comp1 = make_long_component('x');
        comp2 = make_long_component('y');
        path  = join3(base, comp1, comp2);
        free(base);
        free(comp1);
        free(comp2);
        EXPECT_TRUE(path != NULL);

        fs_create_directories(path, &ec); /* "\\?\"-prefixed >MAX_PATH path */
        EXPECT_NO_EC(ec);
        EXPECT_TRUE(fs_exists(path, &ec) == FS_TRUE);

        free(path);
}
#endif

int main(void)
{
        int       result;
        fs_path_t root;

        REGISTER_TEST(boost_long_path, no_escape);
#ifdef _WIN32
        REGISTER_TEST(boost_long_path, with_escape);
#endif
        result = RUN_ALL_TESTS();

        root = fs_path_append(FS_MAKE_PATH("cfs-boost-longpath"),
                              FS_MAKE_PATH("no-escape"), NULL);
        if (root && fs_exists(root, NULL))
                fs_remove_all(root, NULL);
        free(root);
#ifdef _WIN32
        {
                fs_path_t base = make_escaped_base();
                if (base && fs_exists(base, NULL))
                        fs_remove_all(base, NULL);
                free(base);
        }
#endif
        return result;
}
