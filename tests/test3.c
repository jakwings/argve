#include <argve.h>

#define TEST_MAIN
#include <test.h>

#undef  TEST_MAIN
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

int main(int argc, char **argv)
{
    test_h_globaldata_t data;
    test_h_filter_t *filter;
    int c = 0, dashdash = 0;
    const char *suite = NULL;
    struct timeval time_start, time_end, time_used;

    argve_option options[] = {
        {0, 'h', "help"},
        {0, 'q', "quiet"},
        {1, 's', "suite"},
        {1, 'c', "case"},
        {0, 0, NULL}
    };
    argve_state state;

    memset(&data, 0, sizeof(data));
    filter = &data.filter;

    if (!argve_init(&state, options, argc - 1, argv + 1)) {
        TEST_H_ERROR_EXIT(argve_init);
    }
    while (argve_next(&state) != ARGVE_END) {
        if (argve_error(&state)) {
            test_h_help(stderr);
            argve_perror(&state, "\n[ERROR] ", stderr);
            exit(EXIT_FAILURE);
        }
        if (state.type == ARGVE_TEXT) {
            if (dashdash || !(dashdash = test_h_str_eq("--", state.argstr))) {
                fprintf(stderr, "\n[ERROR] unexpected argument#%d: %s\n",
                        argc - 1 - state.argc, state.argstr);
                exit(EXIT_FAILURE);
            }
            c = 0;
        } else {
            /* NOTE: add_filter(suite) when --suite <name> <not --case> */
            switch (state.option->shortopt) {
            case 'h':
                test_h_help(stdout);
                return EXIT_SUCCESS;
            case 'q':
                data.silent = 1;
                break;
            case 's':
                if (c != 'c' /* not after -s <suite> [-c <case>]... */) {
                    filter = test_h_add_filter(filter, suite, NULL);
                }
                suite = state.argstr;
                break;
            case 'c':
                filter = test_h_add_filter(filter, suite, state.argstr);
                break;
            default:
                TEST_H_ERROR_EXIT(argve_next);
            }
            c = state.option->shortopt;
        }
        if (c == 's' && state.argc == 0) {
            filter = test_h_add_filter(filter, suite, NULL);
            suite = NULL;
        } else if (c != 's' && c != 'c') {
            suite = NULL;
        }
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

#include "test1.c"
