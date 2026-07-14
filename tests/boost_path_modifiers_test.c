/*
 * Path modifier and comparison scenarios migrated from Boost.Filesystem's
 * test/path_test.cpp (develop branch).
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

typedef struct modifier_case {
        fs_cpath_t path;
        fs_cpath_t arg;
        fs_cpath_t expected;
} modifier_case_t;

typedef struct append_case {
        fs_cpath_t path;
        fs_cpath_t arg;
        fs_cpath_t expected;
        fs_bool_t  expected_preferred;
} append_case_t;

#define EXPECT_NO_EC(ec) EXPECT_EQ((ec).type, fs_error_type_none)

#define EXPECT_PATH_EQ_PREFERRED(actual, expected_literal)                \
do {                                                                    \
        fs_path_t expected__ = fs_path_dupe((expected_literal), NULL);   \
        fs_path_make_preferred(&expected__, NULL);                       \
        EXPECT_EQ_PATH((actual), expected__);                            \
        free(expected__);                                                \
} while (0)

#define EXPECT_PATH_EQ_LITERAL(actual, expected_literal)                  \
do {                                                                    \
        fs_path_t expected__ = fs_path_dupe((expected_literal), NULL);   \
        EXPECT_EQ_PATH((actual), expected__);                            \
        free(expected__);                                                \
} while (0)

TEST(boost_path_modifiers, append)
{
        const append_case_t cases[] = {
                { FS_MAKE_PATH(""), FS_MAKE_PATH(""), FS_MAKE_PATH(""), FS_TRUE },
                { FS_MAKE_PATH(""), FS_MAKE_PATH("/"), FS_MAKE_PATH("/"), FS_TRUE },
                { FS_MAKE_PATH(""), FS_MAKE_PATH("bar"), FS_MAKE_PATH("bar"), FS_TRUE },
                { FS_MAKE_PATH(""), FS_MAKE_PATH("/bar"), FS_MAKE_PATH("/bar"), FS_TRUE },
                { FS_MAKE_PATH("/"), FS_MAKE_PATH(""), FS_MAKE_PATH("/"), FS_TRUE },
                { FS_MAKE_PATH("/"), FS_MAKE_PATH("/"), FS_MAKE_PATH("/"), FS_TRUE },
                { FS_MAKE_PATH("/"), FS_MAKE_PATH("bar"), FS_MAKE_PATH("/bar"), FS_TRUE },
                { FS_MAKE_PATH("/"), FS_MAKE_PATH("/bar"), FS_MAKE_PATH("/bar"), FS_TRUE },
                { FS_MAKE_PATH("foo"), FS_MAKE_PATH("bar"), FS_MAKE_PATH("foo/bar"), FS_TRUE },
                /* Existing separators are preserved; only inserted separators are preferred. */
                { FS_MAKE_PATH("foo/"), FS_MAKE_PATH(""), FS_MAKE_PATH("foo/"), FS_FALSE },
                { FS_MAKE_PATH("foo/"), FS_MAKE_PATH("bar"), FS_MAKE_PATH("foo/bar"), FS_FALSE }
        };

        fs_error_code_t ec;
        unsigned int    i;

        for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
                fs_path_t out;
                fs_path_t in_place;

                out = fs_path_append(cases[i].path, cases[i].arg, &ec);
                EXPECT_NO_EC(ec);
                if (cases[i].expected_preferred)
                        EXPECT_PATH_EQ_PREFERRED(out, cases[i].expected);
                else
                        EXPECT_PATH_EQ_LITERAL(out, cases[i].expected);
                free(out);

                in_place = fs_path_dupe(cases[i].path, NULL);
                fs_path_append_s(&in_place, cases[i].arg, &ec);
                EXPECT_NO_EC(ec);
                if (cases[i].expected_preferred)
                        EXPECT_PATH_EQ_PREFERRED(in_place, cases[i].expected);
                else
                        EXPECT_PATH_EQ_LITERAL(in_place, cases[i].expected);
                free(in_place);
        }
}

TEST(boost_path_modifiers, concat)
{
        const modifier_case_t cases[] = {
                { FS_MAKE_PATH(""), FS_MAKE_PATH(""), FS_MAKE_PATH("") },
                { FS_MAKE_PATH(""), FS_MAKE_PATH("/"), FS_MAKE_PATH("/") },
                { FS_MAKE_PATH(""), FS_MAKE_PATH("bar"), FS_MAKE_PATH("bar") },
                { FS_MAKE_PATH(""), FS_MAKE_PATH("/bar"), FS_MAKE_PATH("/bar") },
                { FS_MAKE_PATH("/"), FS_MAKE_PATH(""), FS_MAKE_PATH("/") },
                { FS_MAKE_PATH("/"), FS_MAKE_PATH("/"), FS_MAKE_PATH("//") },
                { FS_MAKE_PATH("/"), FS_MAKE_PATH("bar"), FS_MAKE_PATH("/bar") },
                { FS_MAKE_PATH("/"), FS_MAKE_PATH("/bar"), FS_MAKE_PATH("//bar") },
                { FS_MAKE_PATH("foo"), FS_MAKE_PATH("/"), FS_MAKE_PATH("foo/") },
                { FS_MAKE_PATH("foo"), FS_MAKE_PATH("bar"), FS_MAKE_PATH("foobar") },
                { FS_MAKE_PATH("foo/"), FS_MAKE_PATH("bar"), FS_MAKE_PATH("foo/bar") }
        };

        fs_error_code_t ec;
        unsigned int    i;

        for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
                fs_path_t out;
                fs_path_t in_place;

                out = fs_path_concat(cases[i].path, cases[i].arg, &ec);
                EXPECT_NO_EC(ec);
                EXPECT_PATH_EQ_LITERAL(out, cases[i].expected);
                free(out);

                in_place = fs_path_dupe(cases[i].path, NULL);
                fs_path_concat_s(&in_place, cases[i].arg, &ec);
                EXPECT_NO_EC(ec);
                EXPECT_PATH_EQ_LITERAL(in_place, cases[i].expected);
                free(in_place);
        }
}

TEST(boost_path_modifiers, replace_filename_and_extension)
{
        fs_error_code_t ec;
        fs_path_t       path;

        path = fs_path_dupe(FS_MAKE_PATH("/foo/bar.txt"), NULL);
        fs_path_replace_filename(&path, FS_MAKE_PATH("baz.txt"), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_PATH_EQ_LITERAL(path, FS_MAKE_PATH("/foo/baz.txt"));
        free(path);

        path = fs_path_dupe(FS_MAKE_PATH("a.txt"), NULL);
        fs_path_replace_extension(&path, FS_MAKE_PATH(""), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_PATH_EQ_LITERAL(path, FS_MAKE_PATH("a"));
        free(path);

        path = fs_path_dupe(FS_MAKE_PATH("a.txt"), NULL);
        fs_path_replace_extension(&path, FS_MAKE_PATH("tex"), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_PATH_EQ_LITERAL(path, FS_MAKE_PATH("a.tex"));
        free(path);

        path = fs_path_dupe(FS_MAKE_PATH("a"), NULL);
        fs_path_replace_extension(&path, FS_MAKE_PATH(".txt"), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_PATH_EQ_LITERAL(path, FS_MAKE_PATH("a.txt"));
        free(path);

        path = fs_path_dupe(FS_MAKE_PATH("foo.txt"), NULL);
        fs_path_replace_extension(&path, FS_MAKE_PATH(".tar.bz2"), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_PATH_EQ_LITERAL(path, FS_MAKE_PATH("foo.tar.bz2"));
        free(path);

        path = fs_path_dupe(FS_MAKE_PATH("a.txt/b"), NULL);
        fs_path_replace_extension(&path, FS_MAKE_PATH(".c"), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_PATH_EQ_LITERAL(path, FS_MAKE_PATH("a.txt/b.c"));
        free(path);
}

TEST(boost_path_modifiers, make_preferred_and_compare)
{
        fs_error_code_t ec;
        fs_path_t       path;
        int             cmp;

        path = fs_path_dupe(FS_MAKE_PATH("//abc\\def/ghi"), NULL);
        fs_path_make_preferred(&path, &ec);
        EXPECT_NO_EC(ec);
#ifdef _WIN32
        EXPECT_PATH_EQ_LITERAL(path, FS_MAKE_PATH("\\\\abc\\def\\ghi"));
#else
        EXPECT_PATH_EQ_LITERAL(path, FS_MAKE_PATH("//abc\\def/ghi"));
#endif
        free(path);

        cmp = fs_path_compare(FS_MAKE_PATH("foo"), FS_MAKE_PATH("foo"), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_EQ(cmp, 0);

        cmp = fs_path_compare(FS_MAKE_PATH("foo"), FS_MAKE_PATH("zoo"), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_TRUE(cmp < 0);

        cmp = fs_path_compare(FS_MAKE_PATH("zoo"), FS_MAKE_PATH("foo"), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_TRUE(cmp > 0);

        cmp = fs_path_compare(FS_MAKE_PATH("/foo"), FS_MAKE_PATH("foo"), &ec);
        EXPECT_NO_EC(ec);
        EXPECT_TRUE(cmp > 0);
}

int main(void)
{
        REGISTER_TEST(boost_path_modifiers, append);
        REGISTER_TEST(boost_path_modifiers, concat);
        REGISTER_TEST(boost_path_modifiers, replace_filename_and_extension);
        REGISTER_TEST(boost_path_modifiers, make_preferred_and_compare);
        return RUN_ALL_TESTS();
}
