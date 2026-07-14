/*
 * Lexical path scenarios migrated from Boost.Filesystem's test/path_test.cpp
 * and test/relative_test.cpp (develop branch).
 *
 * The original tests are Copyright Beman Dawes 2002-2015 and distributed
 * under the Boost Software License, Version 1.0. See
 * third_party/boost/LICENSE_1_0.txt.
 *
 * This is a C89 adaptation for the CFS API, not a copy of the C++ tests.
 */

#define CFS_IMPLEMENTATION
#include "cfs/cfs.h"

#define TESTS_IMPLEMENTATION
#include "tests.h"

typedef struct path_case {
        fs_cpath_t in;
        fs_cpath_t expected;
} path_case_t;

typedef struct relative_case {
        fs_cpath_t path;
        fs_cpath_t base;
        fs_cpath_t expected;
} relative_case_t;

#define EXPECT_NO_EC(ec) EXPECT_EQ((ec).type, fs_error_type_none)

#define EXPECT_PATH_EQ(actual, expected_literal)                         \
do {                                                                    \
        fs_path_t expected__ = fs_path_dupe((expected_literal), NULL);   \
        fs_path_make_preferred(&expected__, NULL);                       \
        EXPECT_EQ_PATH((actual), expected__);                            \
        free(expected__);                                                \
} while (0)

TEST(boost_path, query_and_decomposition_examples)
{
        fs_error_code_t ec;
        fs_path_t       out;

        out = fs_path_parent_path(FS_MAKE_PATH("/foo/bar.txt"), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_PATH_EQ(out, FS_MAKE_PATH("/foo"));
        free(out);

        out = fs_path_parent_path(FS_MAKE_PATH("/foo/bar/"), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_PATH_EQ(out, FS_MAKE_PATH("/foo/bar"));
        free(out);

        out = fs_path_parent_path(FS_MAKE_PATH("/"), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_PATH_EQ(out, FS_MAKE_PATH(""));
        free(out);

        out = fs_path_filename(FS_MAKE_PATH("/foo/bar.txt"), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_PATH_EQ(out, FS_MAKE_PATH("bar.txt"));
        free(out);

        out = fs_path_filename(FS_MAKE_PATH("/foo/bar/"), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_PATH_EQ(out, FS_MAKE_PATH(""));
        free(out);

        out = fs_path_stem(FS_MAKE_PATH("a.b.c."), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_PATH_EQ(out, FS_MAKE_PATH("a.b.c"));
        free(out);

        out = fs_path_extension(FS_MAKE_PATH("a.b.c."), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_PATH_EQ(out, FS_MAKE_PATH("."));
        free(out);
}

TEST(boost_path, lexically_normal)
{
        const path_case_t cases[] = {
                { FS_MAKE_PATH(""), FS_MAKE_PATH("") },
                { FS_MAKE_PATH("/"), FS_MAKE_PATH("/") },
                { FS_MAKE_PATH("///"), FS_MAKE_PATH("/") },
                { FS_MAKE_PATH("foo"), FS_MAKE_PATH("foo") },
                { FS_MAKE_PATH("/./foo"), FS_MAKE_PATH("/foo") },
                { FS_MAKE_PATH("foo/bar"), FS_MAKE_PATH("foo/bar") },
                { FS_MAKE_PATH(".."), FS_MAKE_PATH("..") },
                { FS_MAKE_PATH("../.."), FS_MAKE_PATH("../..") },
                { FS_MAKE_PATH("../foo"), FS_MAKE_PATH("../foo") },
                { FS_MAKE_PATH("foo/.."), FS_MAKE_PATH(".") },
                { FS_MAKE_PATH("foo/../bar"), FS_MAKE_PATH("bar") },
                { FS_MAKE_PATH("foo/bar/.."), FS_MAKE_PATH("foo") },
                { FS_MAKE_PATH("foo/./bar/.."), FS_MAKE_PATH("foo") },
                { FS_MAKE_PATH("foo/bar/../blah"), FS_MAKE_PATH("foo/blah") },
                { FS_MAKE_PATH("f/../b"), FS_MAKE_PATH("b") },
                { FS_MAKE_PATH("foo/bar/blah/../.."), FS_MAKE_PATH("foo") },
                { FS_MAKE_PATH("foo/bar/blah/../../bletch"), FS_MAKE_PATH("foo/bletch") }
        };

        fs_error_code_t ec;
        unsigned int    i;

        for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
                fs_path_t out = fs_path_lexically_normal(cases[i].in, &ec);
                EXPECT_NO_EC(ec);
                EXPECT_PATH_EQ(out, cases[i].expected);
                free(out);
        }
}

TEST(boost_path, lexically_relative)
{
        const relative_case_t cases[] = {
                { FS_MAKE_PATH("/foo"), FS_MAKE_PATH("/foo"), FS_MAKE_PATH(".") },
                { FS_MAKE_PATH("foo"), FS_MAKE_PATH("foo"), FS_MAKE_PATH(".") },
                { FS_MAKE_PATH("a/b/c"), FS_MAKE_PATH("a"), FS_MAKE_PATH("b/c") },
                { FS_MAKE_PATH("a//b//c"), FS_MAKE_PATH("a"), FS_MAKE_PATH("b/c") },
                { FS_MAKE_PATH("a/b/c"), FS_MAKE_PATH("a/b"), FS_MAKE_PATH("c") },
                { FS_MAKE_PATH("a///b//c"), FS_MAKE_PATH("a//b"), FS_MAKE_PATH("c") },
                { FS_MAKE_PATH("a/b/c"), FS_MAKE_PATH("a/b/c"), FS_MAKE_PATH(".") },
                { FS_MAKE_PATH("a/b/c"), FS_MAKE_PATH("a/b/c/x"), FS_MAKE_PATH("..") },
                { FS_MAKE_PATH("a/b/c"), FS_MAKE_PATH("a/b/c/x/y"), FS_MAKE_PATH("../..") },
                { FS_MAKE_PATH("a/b/c"), FS_MAKE_PATH("a/x"), FS_MAKE_PATH("../b/c") },
                { FS_MAKE_PATH("a/b/c"), FS_MAKE_PATH("a/b/x"), FS_MAKE_PATH("../c") },
                { FS_MAKE_PATH("a/b/c"), FS_MAKE_PATH("a/x/y"), FS_MAKE_PATH("../../b/c") },
                { FS_MAKE_PATH("a/b/c"), FS_MAKE_PATH("a/b/x/y"), FS_MAKE_PATH("../../c") },
                { FS_MAKE_PATH("a/b/c"), FS_MAKE_PATH("a/"), FS_MAKE_PATH("b/c") },
                { FS_MAKE_PATH("a/b/c"), FS_MAKE_PATH("a/."), FS_MAKE_PATH("b/c") },
                { FS_MAKE_PATH("/a/b/c"), FS_MAKE_PATH("/x"), FS_MAKE_PATH("../a/b/c") },
                { FS_MAKE_PATH("/a/d"), FS_MAKE_PATH("/a/b/c"), FS_MAKE_PATH("../../d") },
                { FS_MAKE_PATH("/foo/new"), FS_MAKE_PATH("/foo/bar"), FS_MAKE_PATH("../new") },
                { FS_MAKE_PATH("a/b/c"), FS_MAKE_PATH("x"), FS_MAKE_PATH("") },
                { FS_MAKE_PATH("a/b/c"), FS_MAKE_PATH("/a/b/c"), FS_MAKE_PATH("") }
        };

        fs_error_code_t ec;
        unsigned int    i;

        for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
                fs_path_t out = fs_path_lexically_relative(cases[i].path, cases[i].base, &ec);
                EXPECT_NO_EC(ec);
                EXPECT_PATH_EQ(out, cases[i].expected);
                free(out);
        }
}

TEST(boost_path, lexically_proximate)
{
        fs_error_code_t ec;
        fs_path_t       out;

        out = fs_path_lexically_proximate(FS_MAKE_PATH("a/b/c"), FS_MAKE_PATH("x"), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_PATH_EQ(out, FS_MAKE_PATH("a/b/c"));
        free(out);
}

int main(void)
{
        REGISTER_TEST(boost_path, query_and_decomposition_examples);
        REGISTER_TEST(boost_path, lexically_normal);
        REGISTER_TEST(boost_path, lexically_relative);
        REGISTER_TEST(boost_path, lexically_proximate);
        return RUN_ALL_TESTS();
}
