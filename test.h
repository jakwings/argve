/*==========================================================================*\
) Copyright (c) 2022 by J.W https://github.com/jakwings/test.h               (
)                                                                            (
)   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION          (
)                                                                            (
)  0. You just DO WHAT THE FUCK YOU WANT TO.                                 (
\*==========================================================================*/

#ifndef TEST_H
#define TEST_H

/*\
 / A single-header unit testing library for writing less boilerplate code.
 /
 / * A test suite includes zero or more test cases.
 / * A test case includes zero or more assertions.
 / * A wrong assertion aborts the current test case immediately.
 / * A failed test case does not prevent checking other test cases.
 / * A test suite never prevents any other test suites from running.
 / * Test suites and test cases are not automatically registered.
 / * Test cases must be defined before test suites, which precedes TEST_MAIN.
 / * No thread safety yet.
 /
 / Sample code:
 /
 /     #include "test.h"
 /
 /     TEST_CASE("addition", case_add) {
 /         ASSERT(0 + 1 == 1);
 /         ASSERT(1 + 0 == 1);
 /     }
 /     TEST_CASE("subtraction", case_sub) {
 /         ASSERT(1 - 0 == 1);
 /         ASSERT(0 - 1 == -1);
 /     }
 /     TEST_CASE("multiplication", case_mul) {
 /         ASSERT(0 * 1 == 0);
 /         ASSERT(1 * 0 == 0);
 /     }
 /     TEST_CASE("division", case_div) {
 /         ASSERT(2 / 1 == 2.0);
 /         ASSERT(1 / 2 == 0.5);
 /     }
 /     TEST_SUITE("basic", suite_basic) {
 /         TEST(case_add, NULL);
 /         TEST(case_sub, NULL);
 /         TEST(case_div, NULL);
 /         TEST(case_mul, NULL);
 /     }
 /
 /     TEST_CASE("answer", case_answer) {
 /         EXPECT("The answer to life, the universe, and everything is 42?",
 /                 *TEST_DATA(int *) == 42);
 /     }
 /     TEST_CASE("fizzbuzz", case_fizzbuzz) {
 /         int number = *TEST_DATA(int *);
 /         ASSERT(number % 3 == 0);
 /         ASSERT_OR_GOTO(number % 5 == 0, ignore);
 /         RETURN;
 /     ignore:
 /         EXPECT_OR_GOTO("stop this silly game", number % 5 == 2, ignore);
 /     }
 /     TEST_SUITE("advanced", suite_advanced) {
 /         int answer = *TEST_DATA(int *);
 /         TEST(case_answer, &answer);
 /         TEST(case_fizzbuzz, &answer);
 /     }
 /
 /     TEST_CASE("more", case_more) {
 /         TODO;
 /     }
 /     TEST_SUITE("more", suite_more) {
 /         TEST(case_more, NULL);
 /         TODO;
 /     }
 /
 /     TEST_MAIN {
 /         int answer = 42;
 /         RUN(suite_basic, NULL);
 /         RUN(suite_advanced, &answer);
 /         RUN(suite_more, NULL);
 /     }
\*/

#define TEST_H_HELP "Usage: program [options]\n" \
"\n""Synopsis:\n" \
"    program -h\n" \
"    program [-q] [-c <case>]...\n" \
"    program [-q] [-s <suite> [-c <case>]...]...\n" \
"    program [-q] [-c <case>]... [-s <suite> [-c <case>]...]...\n" \
"\n""Options:\n" \
"    -h, --help            Show this help information.\n" \
"    -q, --quiet           Do not output logs of operations.\n" \
"    -s, --suite <name>    Only run test cases in the specific suite.\n" \
"    -c, --case <name>     Only run the specific test case in the suite.\n" \
/* ... */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TEST_H_NONE         0
#define TEST_H_FAILED       1
#define TEST_H_FILTERED     2
#define TEST_H_TODO         4

typedef struct test_h_filter {
    const char *suite;
    const char *test;
    struct test_h_filter *next;
} test_h_filter_t;

typedef struct test_h_globaldata {
    int total;
    int passed;
    int failed;
    int silent;
    test_h_filter_t filter;
} test_h_globaldata_t;

typedef struct test_h_suitedata {
    const char *name;
    const char *func;
    int flags;
    void *userdata;
    test_h_globaldata_t *global;
} test_h_suitedata_t;

typedef struct test_h_testdata {
    const char *name;
    const char *func;
    int flags;
    int lineno;
    const char *message;
    void *userdata;
} test_h_testdata_t;

#define TEST_H_ERROR_EXIT(x) \
    do { \
        fflush(stdout); \
        fprintf(stderr, "\n[ERROR] %s#L%d %s\n", __FILE__, __LINE__, #x); \
        exit(EXIT_FAILURE); \
    } while (0)

static int
test_h_str_eq(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

static int
test_h_filter_testcase(test_h_filter_t *filter,
                       const char *suite1, const char *suite2,
                       const char *test1, const char *test2)
{
    int no_filter = filter->next == NULL;

    if ((suite1 == NULL || suite2 == NULL)
            || (test1 == NULL && test2 != NULL)
            || (test1 != NULL && test2 == NULL)) {
        TEST_H_ERROR_EXIT(test_h_filter_testcase);
    }
    while ((filter = filter->next) != NULL) {
        if (filter->suite == NULL && filter->test == NULL) {
            TEST_H_ERROR_EXIT(test_h_filter_testcase);
        }
        if (filter->suite != NULL) {
            if (test_h_str_eq(filter->suite, suite1)
                    || test_h_str_eq(filter->suite, suite2)) {
                if (filter->test == NULL
                        || test1 == NULL
                        || test_h_str_eq(filter->test, test1)
                        || test2 == NULL
                        || test_h_str_eq(filter->test, test2)) {
                    return 1;
                }
            }
        } else if (filter->test != NULL) {
            if (test1 == NULL
                    || test_h_str_eq(filter->test, test1)
                    || test2 == NULL
                    || test_h_str_eq(filter->test, test2)) {
                return 1;
            }
        }
    }
    return no_filter;
}

#define TEST_SUITE(suitename, fn) \
    static void test_h_suite_ ## fn(test_h_suitedata_t *test_h_context); \
    static void fn(test_h_globaldata_t *globaldata, void *userdata) \
    { \
        test_h_suitedata_t suitedata; \
        if (!test_h_filter_testcase(&globaldata->filter, \
                                    #fn, suitename, \
                                    NULL, NULL)) { \
            return; \
        } \
        suitedata.name = suitename; \
        suitedata.func = #fn; \
        suitedata.flags = TEST_H_NONE; \
        suitedata.userdata = userdata; \
        suitedata.global = globaldata; \
        test_h_suite_ ## fn(&suitedata); \
        if (suitedata.flags & TEST_H_TODO) { \
            printf("[TEST] %s -- %s ... TODO\n", \
                    suitedata.func, suitedata.name); \
            fflush(stdout); \
        } \
    } \
    static void test_h_suite_ ## fn(test_h_suitedata_t *test_h_context) \
    /* { body: TEST(fn_test, userdata); } */

#define TEST_CASE(testname, fn) \
    static void test_h_test_ ## fn(test_h_testdata_t *test_h_context); \
    static void fn(test_h_suitedata_t *suitedata, void *userdata) \
    { \
        test_h_testdata_t testdata; \
        if (!test_h_filter_testcase(&suitedata->global->filter, \
                                    suitedata->func, suitedata->name, \
                                    #fn, testname)) { \
            suitedata->flags |= TEST_H_FILTERED; \
            return; \
        } \
        testdata.name = testname; \
        testdata.func = #fn; \
        testdata.flags = TEST_H_NONE; \
        testdata.lineno = 0; \
        testdata.message = NULL; \
        testdata.userdata = userdata; \
        if (!suitedata->global->silent) { \
            printf("[TEST] %s : %s ...", suitedata->name, testdata.name); \
            fflush(stdout); \
        } \
        test_h_test_ ## fn(&testdata); \
        suitedata->flags |= (testdata.flags & (TEST_H_FAILED|TEST_H_TODO)); \
        if (!suitedata->global->silent) { \
            if (testdata.flags & TEST_H_FAILED) { \
                printf(" FAILED at %s#L%d %s.%s: %s\n", \
                        __FILE__, testdata.lineno, \
                        suitedata->func, testdata.func, testdata.message); \
            } else if (testdata.flags & TEST_H_TODO) { \
                printf(" TODO\n"); \
            } else { \
                printf(" PASSED\n"); \
            } \
            fflush(stdout); \
        } \
    } \
    static void test_h_test_ ## fn(test_h_testdata_t *test_h_context) \
    /* { body: ASSERT(yes); } */

#define TODO \
    do { \
        test_h_context->flags |= TEST_H_TODO; \
        return; \
    } while (0)

#define RETURN return

#define ASSERT(yes) EXPECT(#yes, yes)

#define EXPECT(yeah, right) \
    /* test_h_testdata_t *test_h_context */ \
    do { \
        if (!(right)) { \
            test_h_context->flags |= TEST_H_FAILED; \
            test_h_context->lineno = __LINE__; \
            test_h_context->message = (const char *)(yeah); \
            RETURN; \
        } \
        test_h_context->flags = TEST_H_NONE; \
        test_h_context->lineno = 0; \
        test_h_context->message = NULL; \
    } while (0)

#define ASSERT_OR_GOTO(yes, label) EXPECT_OR_GOTO(#yes, yes, label)

#define EXPECT_OR_GOTO(yeah, right, label) \
    /* test_h_testdata_t *test_h_context */ \
    do { \
        if (!(right)) { \
            test_h_context->lineno = __LINE__; \
            test_h_context->message = (const char *)(yeah); \
            test_h_context->flags |= TEST_H_FAILED; \
            goto label; \
        } \
        test_h_context->flags = TEST_H_NONE; \
        test_h_context->lineno = 0; \
        test_h_context->message = NULL; \
    } while (0)

#define TEST(fn_test, userdata) \
    /* test_h_suitedata_t *test_h_context */ \
    do { \
        fn_test(test_h_context, userdata); \
        /* flags passed from test_h_testdata */ \
        if (!(test_h_context->flags & (TEST_H_FILTERED|TEST_H_TODO))) { \
            test_h_context->global->total += 1; \
            if (test_h_context->flags & TEST_H_FAILED) { \
                test_h_context->global->failed += 1; \
            } else { \
                test_h_context->global->passed += 1; \
            } \
        } \
        test_h_context->flags = TEST_H_NONE; \
    } while (0)

#define RUN(fn_suit, userdata) fn_suit(test_h_globaldata, userdata)

#define TEST_DATA(type) ((type)(test_h_context->userdata))

#ifndef TEST_MAIN
#define TEST_MAIN \
    static void test_h_main(test_h_globaldata_t *test_h_globaldata) \
    /* { body: RUN(fn_suit, userdata) } */

static void test_h_main(test_h_globaldata_t *test_h_globaldata);

#include <sys/time.h>

#ifndef timersub
#define timersub(a, b, t) \
    do { \
        (t)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
        (t)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
        if ((t)->tv_usec < 0) { \
            (t)->tv_sec--; \
            (t)->tv_usec += 1000000; \
        } \
    } while (0)
#endif /* timersub */

static void
test_h_help(FILE *stream)
{
    fprintf(stream, "%s", TEST_H_HELP);
}

static void
test_h_need_argument(const char *opt, const char *arg)
{
    if (arg == NULL) {
        test_h_help(stderr);
        fprintf(stderr, "\n[ERROR] missing argument for %s\n", opt);
        exit(EXIT_FAILURE);
    }
}

static test_h_filter_t *
test_h_add_filter(test_h_filter_t *filter,
                  const char *suite,
                  const char *test)
{
    if (suite != NULL || test != NULL) {
        filter->next = (test_h_filter_t *)malloc(sizeof(test_h_filter_t));
        filter = filter->next;
        if (filter != NULL) {
            filter->suite = suite;
            filter->test = test;
            filter->next = NULL;
        } else {
            fprintf(stderr, "\n[ERROR] failed to allocate memory\n");
            exit(EXIT_FAILURE);
        }
    }
    return filter;
}

int main(int argc, const char **argv)
{
    test_h_globaldata_t data;
    test_h_filter_t *filter;
    int i, dashdash = 0;
    const char *suite = NULL;
    struct timeval time_start, time_end, time_used;

    memset(&data, 0, sizeof(data));
    filter = &data.filter;

    for (i = 1; i < argc; i += 1) {
        if (dashdash || argv[i][0] != '-' || argv[i][1] == '\0') {
            test_h_help(stderr);
            fprintf(stderr, "\n[ERROR] unexpected argument#%d: %s\n",
                    i, argv[i]);
            return EXIT_FAILURE;
        }
        /* NOTE: add_filter(suite) when --suite <name> <not --case> */
        if (test_h_str_eq("--", argv[i])) {
            dashdash = 1;
        } else if (test_h_str_eq("-h", argv[i])
                || test_h_str_eq("--help", argv[i])) {
            test_h_help(stdout);
            return EXIT_SUCCESS;
        } else if (test_h_str_eq("-q", argv[i])
                || test_h_str_eq("--quiet", argv[i])) {
            data.silent = 1;
        } else if (test_h_str_eq("-s", argv[i])
                || test_h_str_eq("--suite", argv[i])) {
            test_h_need_argument(argv[i], argv[i + 1]);
            if (i > 1 && argv[i - 1] == suite) {
                filter = test_h_add_filter(filter, suite, NULL);
            }
            suite = argv[i + 1];
            if (i + 2 == argc) {
                filter = test_h_add_filter(filter, suite, NULL);
            }
            i += 1;
            continue;
        } else if (test_h_str_eq("-c", argv[i])
                || test_h_str_eq("--case", argv[i])) {
            test_h_need_argument(argv[i], argv[i + 1]);
            filter = test_h_add_filter(filter, suite, argv[i + 1]);
            i += 1;
            continue;
        } else {
            /* -o<value> and --option=<value> unsupported */
            test_h_help(stderr);
            fprintf(stderr, "\n[ERROR] unknown option: %s\n", argv[i]);
            return EXIT_FAILURE;
        }
        if (i > 1 && argv[i - 1] == suite) {
            filter = test_h_add_filter(filter, suite, NULL);
        }
        suite = NULL;
    }

    if (!data.silent) {
        printf("[INFO] Tests starting...\n");
        if (gettimeofday(&time_start, NULL) != 0) {
            TEST_H_ERROR_EXIT(gettimeofday);
        }
    }

    test_h_main(&data);

    if (!data.silent) {
        if (gettimeofday(&time_end, NULL) != 0) {
            TEST_H_ERROR_EXIT(gettimeofday);
        }
        timersub(&time_end, &time_start, &time_used);
        printf("[INFO] Total / Passed / Failed: %d / %d / %d\n",
                data.total,
                data.passed,
                data.failed);
        printf("[INFO] Time used: %ld minutes %ld.%06ld seconds\n",
                time_used.tv_sec / 60,
                time_used.tv_sec % 60,
                (time_t)time_used.tv_usec);
    }

    return data.failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif /* TEST_MAIN */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TEST_H */
