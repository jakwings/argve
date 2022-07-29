#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <inttypes.h>

#define ARGVE_H_WITH_IMPLEMENTATION
#define ARGVE_H_API static
#include <argve.h>

#include <test.h>

#include "sfc.c"

#define ERROR_EXIT(x) \
    do { \
        fflush(stdout); \
        fprintf(stderr, "\n[ERROR] %s#L%d %s\n", __FILE__, __LINE__, #x); \
        exit(EXIT_FAILURE); \
    } while (0)

#define MAX_OPTS 255

#define SETUP_WITH_ARGV(size) \
    int argc = size - 1; \
    char * argv[size]; \
    argve_option options[MAX_OPTS + 1]; \
    argve_state state; \
    argve_type type; \
    int c, i, j, k; char *s; \
    memset(&argv, 0, sizeof(argv)); \
    memset(&options, 0, sizeof(options)); \
    /* -Werror,-Wunused-variable -Werror=unused-value */ \
    (void)argc; (void)state; (void)type; \
    (void)c; (void)i; (void)j; (void)k; (void)s; \
    (void)argve_perror

void set_args(char **argv, int *argc, ...)
{
    va_list args;
    va_start(args, argc);
    *argc = 0;
    while ((*argv = va_arg(args, char *)) != NULL) { *argc += 1; argv += 1; }
    va_end(args);
}

#define SET_OPTION(i, yes, chr, str) \
    do { \
        options[i].need_arg = (yes); \
        options[i].shortopt = (chr); \
        options[i].longopt = (str); \
    } while (0)

#define ASSERT_ARGVE_END \
    do { \
        ASSERT(state.options == options); \
        ASSERT(state.option == NULL); \
        ASSERT(state.optstr == NULL); \
        ASSERT(state.argstr == NULL); \
        ASSERT(state.argc == 0); \
        ASSERT(state.argv == argv + argc); \
        ASSERT(state.type == ARGVE_END); \
        ASSERT(!argve_error(&state)); \
    } while (0)

#define EXPECT_STRING(a, b) ASSERT(strcmp(a, b) == 0)

TEST_CASE("state = NULL", case_init_1) {
    SETUP_WITH_ARGV(2);
    set_args(argv, &argc, "trivial", NULL);
    SET_OPTION(0, 0, 'o', "option");
    ASSERT(!argve_init(NULL, options, argc, argv));
    ASSERT(argve_init(&state, options, argc, argv));
}

TEST_CASE("options = NULL", case_init_2) {
    SETUP_WITH_ARGV(2);
    set_args(argv, &argc, "trivial", NULL);
    SET_OPTION(0, 0, 'o', "option");
    ASSERT(!argve_init(&state, NULL, argc, argv));
    ASSERT(argve_init(&state, options, argc, argv));
}

TEST_CASE("argc < 0", case_init_3) {
    SETUP_WITH_ARGV(2);
    set_args(argv, &argc, "trivial", NULL);
    SET_OPTION(0, 0, 'o', "option");
    ASSERT(!argve_init(&state, options, -1, argv));
    ASSERT(argve_init(&state, options, argc, argv));
}

TEST_CASE("argv = NULL", case_init_4) {
    SETUP_WITH_ARGV(2);
    set_args(argv, &argc, "trivial", NULL);
    SET_OPTION(0, 0, 'o', "option");
    ASSERT(!argve_init(&state, options, argc, NULL));
    ASSERT(argve_init(&state, options, argc, argv));
}

TEST_CASE("argv[i] = NULL", case_init_5) {
    SETUP_WITH_ARGV(2);
    SET_OPTION(0, 0, 'o', "option");
    ASSERT(!argve_init(&state, options, argc, argv));
}

TEST_CASE("invalid flags", case_init_6) {
    char *longopts[] = {"", "=X", "[ ]", "[\xC2\xA1]", NULL};
    char longopt[] = "[X]";
    SETUP_WITH_ARGV(2);
    set_args(argv, &argc, "trivial", NULL);

    SET_OPTION(0, 0, 'o', "option");
    for (i = 0; longopts[i] != NULL; i++) {
        options[0].longopt = longopts[i];
        ASSERT(!argve_init(&state, options, argc, argv));
        options[0].longopt = "option";
        ASSERT(argve_init(&state, options, argc, argv));
    }
    for (i = 1; i <= 0x20; i++) {
        longopt[1] = i;
        options[0].longopt = longopt;
        ASSERT(!argve_init(&state, options, argc, argv));
        options[0].longopt = "option";
        ASSERT(argve_init(&state, options, argc, argv));
    }
    for (i = 0x7F; i <= 0xFF; i++) {
        longopt[1] = i;
        options[0].longopt = longopt;
        ASSERT(!argve_init(&state, options, argc, argv));
        options[0].longopt = "option";
        ASSERT(argve_init(&state, options, argc, argv));
    }

    SET_OPTION(0, 0, 'o', "option");
    /* XXX: [INT_MIN, INT_MAX] would take too much time */
    for (i = -32768; i < 32767; i++) {
        options[0].shortopt = i;
        options[0].longopt = "";
        ASSERT(!argve_init(&state, options, argc, argv));
        options[0].longopt = "option";
        ASSERT(argve_init(&state, options, argc, argv));
    }
}

TEST_CASE("nothing", case_positional_1) {
    SETUP_WITH_ARGV(1);
    ASSERT(0 == argc);
    ASSERT(argve_init(&state, options, argc, argv));
    /* check public properties */
    ASSERT(state.options == options);
    ASSERT(state.option == NULL);
    ASSERT(state.optstr == NULL);
    ASSERT(state.argstr == NULL);
    ASSERT(state.argc == argc);
    ASSERT(state.argv == argv);
    ASSERT(state.type == ARGVE_ERR_UNKNOWN);
    ASSERT(argve_error(&state));
    /* parsing end immediately */
    ASSERT(argve_next(&state) == ARGVE_END);
    ASSERT_ARGVE_END;
}

TEST_CASE("trivial", case_positional_2) {
    SETUP_WITH_ARGV(256);

    for (i = 0; i < argc; i++) argv[i] = "trivial";
    ASSERT(argve_init(&state, options, argc, argv));
    ASSERT(state.type == ARGVE_ERR_UNKNOWN);

    for (i = 0; i < argc; i++) {
        ASSERT(argve_next(&state) == ARGVE_TEXT);
        ASSERT(state.options == options);
        ASSERT(state.option == NULL);
        ASSERT(state.optstr == NULL);
        ASSERT(state.argstr == argv[i]);
        ASSERT(state.argc == argc - i - 1);
        ASSERT(state.argv == argv + i + 1);
        ASSERT(state.type == ARGVE_TEXT);
        EXPECT_STRING("trivial", state.argstr);
    }
    ASSERT(i == argc);
    ASSERT(argve_next(&state) == ARGVE_END);
    ASSERT_ARGVE_END;
}

TEST_CASE("no options defined", case_positional_3) {
    SETUP_WITH_ARGV(256);

    for (i = 0; i < argc; i++) {
        argv[i] = i % 3 == 0 ? "-o" : (i % 5 == 0 ? "--option" : "trivial");
    }
    ASSERT(argve_init(&state, options, argc, argv));
    ASSERT(state.type == ARGVE_ERR_UNKNOWN);

    for (i = 0; i < argc; i++) {
        if (i % 3 == 0) {
            ASSERT(argve_next(&state) == ARGVE_ERR_DEF_SHORT);
            ASSERT(argve_error(&state));
            ASSERT(state.optstr == argv[i] + 1);
            ASSERT(state.argstr == NULL);
            ASSERT(state.type == ARGVE_ERR_DEF_SHORT);
            EXPECT_STRING("-o", state.optstr - 1);
        } else if (i % 5 == 0) {
            ASSERT(argve_next(&state) == ARGVE_ERR_DEF_LONG);
            ASSERT(argve_error(&state));
            ASSERT(state.optstr == argv[i] + 2);
            ASSERT(state.argstr == NULL);
            ASSERT(state.type == ARGVE_ERR_DEF_LONG);
            EXPECT_STRING("--option", state.optstr - 2);
        } else {
            ASSERT(argve_next(&state) == ARGVE_TEXT);
            ASSERT(state.optstr == NULL);
            ASSERT(state.argstr == argv[i]);
            ASSERT(state.type == ARGVE_TEXT);
            EXPECT_STRING("trivial", state.argstr);
        }
        ASSERT(state.options == options);
        ASSERT(state.option == NULL);
        ASSERT(state.argc == argc - i - 1);
        ASSERT(state.argv == argv + i + 1);
    }
    ASSERT(i == argc);
    ASSERT(argve_next(&state) == ARGVE_END);
    ASSERT_ARGVE_END;
}

TEST_CASE("-o (null), --option (null)", case_options_1) {
    SETUP_WITH_ARGV(256);

    for (i = 0; i < argc; i++) {
        argv[i] = i % 3 == 0 ? "-o" : (i % 5 == 0 ? "--option" : "trivial");
    }
    SET_OPTION(0, 0, 'o', NULL);
    SET_OPTION(1, 0, 'o' + 256, "option");
    ASSERT(argve_init(&state, options, argc, argv));

    for (i = 0; i < argc; i++) {
        if (i % 3 == 0) {
            ASSERT(argve_next(&state) == ARGVE_SHORT);
            ASSERT(state.option == &options[0]);
            ASSERT(state.optstr == argv[i] + 1);
            ASSERT(state.argstr == NULL);
            ASSERT(state.type == ARGVE_SHORT);
            ASSERT(state.option->need_arg == 0);
            ASSERT(state.option->shortopt == 'o');
            ASSERT(state.option->longopt == NULL);
            EXPECT_STRING("-o", state.optstr - 1);
        } else if (i % 5 == 0) {
            ASSERT(argve_next(&state) == ARGVE_LONG);
            ASSERT(state.option == &options[1]);
            ASSERT(state.optstr == argv[i] + 2);
            ASSERT(state.argstr == NULL);
            ASSERT(state.type == ARGVE_LONG);
            ASSERT(state.option->need_arg == 0);
            ASSERT(state.option->shortopt == 367);
            EXPECT_STRING("option", state.option->longopt);
            EXPECT_STRING("--option", state.optstr - 2);
        } else {
            ASSERT(argve_next(&state) == ARGVE_TEXT);
            ASSERT(state.option == NULL);
            ASSERT(state.optstr == NULL);
            ASSERT(state.argstr == argv[i]);
            ASSERT(state.type == ARGVE_TEXT);
            EXPECT_STRING("trivial", state.argstr);
        }
        ASSERT(state.options == options);
        ASSERT(state.argc == argc - i - 1);
        ASSERT(state.argv == argv + i + 1);
    }
    ASSERT(i == argc);
    ASSERT(argve_next(&state) == ARGVE_END);
    ASSERT_ARGVE_END;
}

TEST_CASE("-o <value>, --option <value>", case_options_2) {
    SETUP_WITH_ARGV(256);

    for (i = 0; i < argc; i++) {
        argv[i] = i % 3 == 0 ? "-o" : (i % 5 == 0 ? "--option" : "trivial");
    }
    SET_OPTION(0, 1, 'o', NULL);
    SET_OPTION(1, 1, 'o' + 256, "option");
    ASSERT(argve_init(&state, options, argc, argv));

    for (i = 0; i < argc; i++) {
        type = argve_next(&state);
        if (i % 3 == 0) {
            ASSERT(type == ARGVE_SHORT);
            ASSERT(state.option == &options[0]);
            ASSERT(state.optstr == argv[i] + 1);
            ASSERT(state.argstr != NULL);
            ASSERT(state.type == ARGVE_SHORT);
            ASSERT(state.option->need_arg == 1);
            ASSERT(state.option->shortopt == 'o');
            ASSERT(state.option->longopt == NULL);
            EXPECT_STRING("-o", state.optstr - 1);
        } else if (i % 5 == 0) {
            ASSERT(type == ARGVE_LONG);
            ASSERT(state.option == &options[1]);
            ASSERT(state.optstr == argv[i] + 2);
            ASSERT(state.argstr != NULL);
            ASSERT(state.type == ARGVE_LONG);
            ASSERT(state.option->need_arg == 1);
            ASSERT(state.option->shortopt == 367);
            EXPECT_STRING("option", state.option->longopt);
            EXPECT_STRING("--option", state.optstr - 2);
        } else {
            ASSERT(type == ARGVE_TEXT);
            ASSERT(state.option == NULL);
            ASSERT(state.optstr == NULL);
            ASSERT(state.argstr == argv[i]);
            ASSERT(state.type == ARGVE_TEXT);
            EXPECT_STRING("trivial", state.argstr);
        }
        if (type == ARGVE_SHORT || type == ARGVE_LONG) {
            i++;
            if (i % 3 == 0) {
                EXPECT_STRING("-o", state.argstr);
            } else if (i % 5 == 0) {
                EXPECT_STRING("--option", state.argstr);
            } else {
                EXPECT_STRING("trivial", state.argstr);
            }
        }
        ASSERT(state.options == options);
        ASSERT(state.argc == argc - i - 1);
        ASSERT(state.argv == argv + i + 1);
    }
    ASSERT(i == argc);
    ASSERT(argve_next(&state) == ARGVE_END);
    ASSERT_ARGVE_END;
}

TEST_CASE("-o<value>, --option=<value>", case_options_3) {
    SETUP_WITH_ARGV(256);

    for (i = 0; i < argc; i++) {
        argv[i] = i % 3 == 0 ? "-o==" : (i % 5 == 0 ? "--option===" : "==");
    }
    SET_OPTION(0, 1, 'o', NULL);
    SET_OPTION(1, 1, 'o' + 256, "option");
    ASSERT(argve_init(&state, options, argc, argv));

    for (i = 0; i < argc; i++) {
        type = argve_next(&state);
        if (i % 3 == 0) {
            ASSERT(type == ARGVE_SHORT);
            ASSERT(state.option == &options[0]);
            ASSERT(state.optstr == argv[i] + 1);
            ASSERT(state.argstr != NULL);
            ASSERT(state.type == ARGVE_SHORT);
            ASSERT(state.option->need_arg == 1);
            ASSERT(state.option->shortopt == 'o');
            ASSERT(state.option->longopt == NULL);
            EXPECT_STRING("-o==", state.optstr - 1);
            EXPECT_STRING("-o==", state.argstr - 2);
        } else if (i % 5 == 0) {
            ASSERT(type == ARGVE_LONG);
            ASSERT(state.option == &options[1]);
            ASSERT(state.optstr == argv[i] + 2);
            ASSERT(state.argstr != NULL);
            ASSERT(state.type == ARGVE_LONG);
            ASSERT(state.option->need_arg == 1);
            ASSERT(state.option->shortopt == 367);
            EXPECT_STRING("option", state.option->longopt);
            EXPECT_STRING("--option===", state.optstr - 2);
            EXPECT_STRING("--option===", state.argstr - 9);
        } else {
            ASSERT(type == ARGVE_TEXT);
            ASSERT(state.option == NULL);
            ASSERT(state.optstr == NULL);
            ASSERT(state.argstr == argv[i]);
            ASSERT(state.type == ARGVE_TEXT);
            EXPECT_STRING("==", state.argstr);
        }
        ASSERT(state.options == options);
        ASSERT(state.argc == argc - i - 1);
        ASSERT(state.argv == argv + i + 1);
    }
    ASSERT(i == argc);
    ASSERT(argve_next(&state) == ARGVE_END);
    ASSERT_ARGVE_END;
}

TEST_CASE("-abc => -a -b -c", case_options_4) {
    SETUP_WITH_ARGV(32);

    set_args(argv, &argc,
             "-abc", "-acb", "-bac", "d", "-bca", "-cab", "d", "-cba",
             "-ab", "-ba", "-ac", "d", "-ca", "-bc", "d", "-cb",
             "-a", "-b", "-c", "d", NULL);
    SET_OPTION(0, 0, 'a', NULL);
    SET_OPTION(1, 0, 'b', NULL);
    SET_OPTION(2, 0, 'c', NULL);
    ASSERT(argve_init(&state, options, argc, argv));

    k = 0 /* index in cluster */;
    for (i = 0; i < argc; i++) {
        type = argve_next(&state);
        if (type == ARGVE_CLUSTER) {
            if (k++ != 0) {
                /* not processing new argument */
                ASSERT(*state.argv == argv[i]);
                ASSERT(state.argc == argc - i);
                i--;
            }
            c = argv[i][k];
            ASSERT('a' <= c && c <= 'c');
            ASSERT(state.option == &options[c - 'a']);
            ASSERT(state.optstr == argv[i] + k);
            ASSERT(state.argstr == NULL);
            ASSERT(state.type == ARGVE_CLUSTER);
            ASSERT(state.option->need_arg == 0);
            ASSERT(state.option->shortopt == c);
            ASSERT(state.option->longopt == NULL);
            ASSERT(state.optstr[0] == c);
            ASSERT(state.optstr[0 - k] == '-');
            if (state.optstr[1] == '\0' || state.argstr != NULL) {
                k = 0;
            }
        } else if (type == ARGVE_SHORT) {
            c = argv[i][1];
            ASSERT('a' <= c && c <= 'c');
            ASSERT(state.option == &options[c - 'a']);
            ASSERT(state.optstr == argv[i] + 1);
            ASSERT(state.argstr == NULL);
            ASSERT(state.type == ARGVE_SHORT);
            ASSERT(state.option->need_arg == 0);
            ASSERT(state.option->shortopt == c);
            ASSERT(state.option->longopt == NULL);
            ASSERT(state.optstr[0] == c);
            ASSERT(state.optstr[1] == 0);
        } else {
            ASSERT(type == ARGVE_TEXT);
            ASSERT(state.option == NULL);
            ASSERT(state.optstr == NULL);
            ASSERT(state.argstr == argv[i]);
            ASSERT(state.type == ARGVE_TEXT);
            EXPECT_STRING("d", state.argstr);
        }
        ASSERT(state.options == options);
        ASSERT(state.argc == argc - i - 1);
        ASSERT(state.argv == argv + i + 1);
    }
    ASSERT(i == argc);
    ASSERT(argve_next(&state) == ARGVE_END);
    ASSERT_ARGVE_END;
}

TEST_CASE("-abc => -a -b c", case_options_5) {
    SETUP_WITH_ARGV(32);

    set_args(argv, &argc,
             "-abc", "-acb", "-bac", "d", "-bca", "-cab", "d", "-cba",
             "-ab", "-ba", "-ac", "d", "-ca", "-bc", "d", "-cb",
             "-a", "-b", "-c", "d", NULL);
    SET_OPTION(0, 0, 'a', NULL);
    SET_OPTION(1, 1, 'b', NULL);
    SET_OPTION(2, 0, 'c', NULL);
    ASSERT(argve_init(&state, options, argc, argv));

    k = 0 /* index in cluster */;
    for (i = 0; i < argc; i++) {
        type = argve_next(&state);
        if (type == ARGVE_CLUSTER) {
            j = 0;
            if (k++ != 0) {
                j = (*state.argv == argv[i + 1]);
                /* not processing new argument or new argument was consumed */
                ASSERT(*state.argv == argv[i] || j);
                ASSERT(state.argc == argc - i - j);
                i--;
            }
            c = argv[i][k];
            ASSERT('a' <= c && c <= 'c');
            ASSERT(state.option == &options[c - 'a']);
            ASSERT(state.optstr == argv[i] + k);
            ASSERT(state.optstr[0] == c);
            ASSERT(state.optstr[0 - k] == '-');
            if (c != 'b') {
                ASSERT(state.argstr == NULL);
            } else if (state.optstr[1] == '\0') {
                ASSERT(state.argstr == argv[i + 1]);
            } else {
                ASSERT(state.argstr == argv[i] + k + 1);
            }
            ASSERT(state.type == ARGVE_CLUSTER);
            ASSERT(state.option->need_arg == (c == 'b'));
            ASSERT(state.option->shortopt == c);
            ASSERT(state.option->longopt == NULL);
            if (state.optstr[1] == '\0' || state.argstr != NULL) {
                k = 0;
            }
            /* cluster ended as new argument was consumed */
            if (c == 'b' && state.optstr[1] == '\0') {
                i++;
            }
        } else if (type == ARGVE_SHORT) {
            c = argv[i][1];
            ASSERT('a' <= c && c <= 'c');
            ASSERT(state.option == &options[c - 'a']);
            ASSERT(state.optstr == argv[i] + 1);
            ASSERT(state.optstr[0] == c);
            ASSERT(state.optstr[-1] == '-');
            if (c != 'b') {
                ASSERT(state.argstr == NULL);
            } else if (state.optstr[1] == '\0') {
                ASSERT(state.argstr == argv[i + 1]);
            } else {
                ASSERT(state.argstr == argv[i] + 2);
            }
            ASSERT(state.type == ARGVE_SHORT);
            ASSERT(state.option->need_arg == (c == 'b'));
            ASSERT(state.option->shortopt == c);
            ASSERT(state.option->longopt == NULL);
            if (c == 'b' && state.optstr[1] == '\0') {
                i++;
            }
        } else {
            ASSERT(type == ARGVE_TEXT);
            ASSERT(state.option == NULL);
            ASSERT(state.optstr == NULL);
            ASSERT(state.argstr == argv[i]);
            ASSERT(state.type == ARGVE_TEXT);
            EXPECT_STRING("d", state.argstr);
        }
        ASSERT(state.options == options);
        ASSERT(state.argc == argc - i - 1);
        ASSERT(state.argv == argv + i + 1);
    }
    ASSERT(i == argc);
    ASSERT(argve_next(&state) == ARGVE_END);
    ASSERT_ARGVE_END;
}

TEST_CASE("-o- =/= -o --", case_options_6) {
    SETUP_WITH_ARGV(2);

    set_args(argv, &argc, "-o-", NULL);
    SET_OPTION(0, 0, 'o', (s = "option"));
    ASSERT(argve_init(&state, options, argc, argv));
    ASSERT(argve_next(&state) == ARGVE_CLUSTER);
    ASSERT(state.options == options);
    ASSERT(state.option == &options[0]);
    ASSERT(state.option->need_arg == 0);
    ASSERT(state.option->shortopt == 'o');
    ASSERT(state.option->longopt == s);
    ASSERT(state.optstr == argv[0] + 1);
    ASSERT(state.argstr == NULL);
    ASSERT(state.type == ARGVE_CLUSTER);
    ASSERT(state.argc == argc - 1);
    ASSERT(state.argv == argv + 1);
    /* actually '-' is invalidated before having a chance for matching */
    ASSERT(argve_next(&state) == ARGVE_ERR_DEF_SHORT);
    ASSERT(argve_error(&state));
    ASSERT(state.options == options);
    ASSERT(state.option == NULL);
    ASSERT(state.optstr == argv[0] + 2);
    ASSERT(state.argstr == NULL);
    ASSERT(state.type == ARGVE_ERR_DEF_SHORT);
    /* not processing new argument */
    ASSERT(state.argc == argc - 1);
    ASSERT(state.argv == argv + 1);
    ASSERT(argve_next(&state) == ARGVE_END);
    ASSERT_ARGVE_END;

    set_args(argv, &argc, "-o-", NULL);
    SET_OPTION(0, 0, 'o', (s = "option"));
    SET_OPTION(1, 0, '-', s);  /* '-' is a virtual code for arbitrary <s> */
    ASSERT(argve_init(&state, options, argc, argv));
    ASSERT(argve_next(&state) == ARGVE_CLUSTER);
    ASSERT(argve_next(&state) == ARGVE_ERR_DEF_SHORT);
    ASSERT(argve_error(&state));
    ASSERT(state.options == options);
    ASSERT(state.option == NULL);
    ASSERT(state.optstr == argv[0] + 2);  /* '-' is unrecognized */
    ASSERT(state.argstr == NULL);
    ASSERT(state.type == ARGVE_ERR_DEF_SHORT);
    ASSERT(state.argc == argc - 1);
    ASSERT(state.argv == argv + 1);
    ASSERT(argve_next(&state) == ARGVE_END);
    ASSERT_ARGVE_END;

    set_args(argv, &argc, "-o-", NULL);
    SET_OPTION(0, 1, 'o', (s = "option"));
    SET_OPTION(1, 0, '-', s);
    ASSERT(argve_init(&state, options, argc, argv));
    ASSERT(argve_next(&state) == ARGVE_SHORT);
    ASSERT(state.options == options);
    ASSERT(state.option == &options[0]);
    ASSERT(state.option->need_arg == 1);
    ASSERT(state.option->shortopt == 'o');
    ASSERT(state.option->longopt == s);
    ASSERT(state.optstr == argv[0] + 1);
    ASSERT(state.argstr == argv[0] + 2);  /* '-' is consumed */
    ASSERT(state.type == ARGVE_SHORT);
    ASSERT(state.argc == argc - 1);
    ASSERT(state.argv == argv + 1);
    ASSERT(argve_next(&state) == ARGVE_END);
    ASSERT_ARGVE_END;
}

TEST_CASE("missing option value", case_options_7) {
    SETUP_WITH_ARGV(4);

    set_args(argv, &argc, "-o", NULL);
    SET_OPTION(0, 1, 'o', (s = "option"));
    ASSERT(argve_init(&state, options, argc, argv));
    ASSERT(argve_next(&state) == ARGVE_ERR_ARG_SHORT);
    ASSERT(argve_error(&state));
    ASSERT(state.options == options);
    ASSERT(state.option == &options[0]);
    ASSERT(state.option->need_arg == 1);
    ASSERT(state.option->shortopt == 'o');
    ASSERT(state.option->longopt == s);
    ASSERT(state.optstr == argv[0] + 1);
    ASSERT(state.argstr == NULL);
    ASSERT(state.type == ARGVE_ERR_ARG_SHORT);
    ASSERT(state.argc == argc - 1);
    ASSERT(state.argv == argv + 1);
    ASSERT(argve_next(&state) == ARGVE_END);
    ASSERT_ARGVE_END;

    set_args(argv, &argc, "-no", NULL);
    SET_OPTION(0, 0, 'n', (s = "option"));
    SET_OPTION(1, 1, 'o', (s = "option"));
    ASSERT(argve_init(&state, options, argc, argv));
    ASSERT(argve_next(&state) == ARGVE_CLUSTER);
    ASSERT(state.options == options);
    ASSERT(state.option == &options[0]);
    ASSERT(state.option->need_arg == 0);
    ASSERT(state.option->shortopt == 'n');
    ASSERT(state.option->longopt == s);
    ASSERT(state.optstr == argv[0] + 1);
    ASSERT(state.argstr == NULL);
    ASSERT(state.type == ARGVE_CLUSTER);
    ASSERT(state.argc == argc - 1);
    ASSERT(state.argv == argv + 1);
    ASSERT(argve_next(&state) == ARGVE_ERR_ARG_SHORT);
    ASSERT(argve_error(&state));
    ASSERT(state.options == options);
    ASSERT(state.option == &options[1]);
    ASSERT(state.option->need_arg == 1);
    ASSERT(state.option->shortopt == 'o');
    ASSERT(state.option->longopt == s);
    ASSERT(state.optstr == argv[0] + 2);
    ASSERT(state.argstr == NULL);
    ASSERT(state.type == ARGVE_ERR_ARG_SHORT);
    /* not processing new argument */
    ASSERT(state.argc == argc - 1);
    ASSERT(state.argv == argv + 1);
    ASSERT(argve_next(&state) == ARGVE_END);
    ASSERT_ARGVE_END;

    set_args(argv, &argc, "--option=", "--option", NULL);
    SET_OPTION(0, 1, 'o', (s = "option"));
    ASSERT(argve_init(&state, options, argc, argv));
    ASSERT(argve_next(&state) == ARGVE_LONG);
    ASSERT(state.options == options);
    ASSERT(state.option == &options[0]);
    ASSERT(state.option->need_arg == 1);
    ASSERT(state.option->shortopt == 'o');
    ASSERT(state.option->longopt == s);
    ASSERT(state.optstr == argv[0] + 2);
    ASSERT(state.argstr == argv[0] + 9);
    ASSERT(state.type == ARGVE_LONG);
    ASSERT(state.argc == argc - 1);
    ASSERT(state.argv == argv + 1);
    ASSERT(argve_next(&state) == ARGVE_ERR_ARG_LONG);
    ASSERT(argve_error(&state));
    ASSERT(state.options == options);
    ASSERT(state.option == &options[0]);
    ASSERT(state.option->need_arg == 1);
    ASSERT(state.option->shortopt == 'o');
    ASSERT(state.option->longopt == s);
    ASSERT(state.optstr == argv[1] + 2);
    ASSERT(state.argstr == NULL);
    ASSERT(state.type == ARGVE_ERR_ARG_LONG);
    ASSERT(state.argc == argc - 2);
    ASSERT(state.argv == argv + 2);
    ASSERT(argve_next(&state) == ARGVE_END);
    ASSERT_ARGVE_END;
}

TEST_CASE("unneeded option value", case_options_8) {
    SETUP_WITH_ARGV(2);

    set_args(argv, &argc, "--option=", NULL);
    SET_OPTION(0, 0, 'o', (s = "option"));
    ASSERT(argve_init(&state, options, argc, argv));
    ASSERT(argve_next(&state) == ARGVE_ERR_ARG_LONG);
    ASSERT(argve_error(&state));
    ASSERT(state.options == options);
    ASSERT(state.option == &options[0]);
    ASSERT(state.option->need_arg == 0);
    ASSERT(state.option->shortopt == 'o');
    ASSERT(state.option->longopt == s);
    ASSERT(state.optstr == argv[0] + 2);
    ASSERT(state.argstr == argv[0] + 9);
    ASSERT(state.type == ARGVE_ERR_ARG_LONG);
    ASSERT(state.argc == argc - 1);
    ASSERT(state.argv == argv + 1);
    ASSERT(argve_next(&state) == ARGVE_END);
    ASSERT_ARGVE_END;
}

TEST_CASE("-long", case_options_9) {
    SETUP_WITH_ARGV(3);

    set_args(argv, &argc, "-option", "-option=", NULL);
    SET_OPTION(0, 0, 0, "option");
    ASSERT(argve_init(&state, options, argc, argv));

    for (i = 0; i < argc; i++) {
        ASSERT(argve_next(&state) == ARGVE_ERR_DEF_SHORT);
        ASSERT(argve_error(&state));
        ASSERT(state.options == options);
        ASSERT(state.option == NULL);
        ASSERT(state.optstr == argv[i] + 1);
        ASSERT(state.argstr == NULL);
        ASSERT(state.type == ARGVE_ERR_DEF_SHORT);
        ASSERT(state.argc == argc - i - 1);
        ASSERT(state.argv == argv + i + 1);
    }
    ASSERT(i == argc);
    ASSERT(argve_next(&state) == ARGVE_END);
    ASSERT_ARGVE_END;
}

TEST_CASE("single dash", case_dashes_1) {
    SETUP_WITH_ARGV(256);

    for (i = 0; i < argc; i++) {
        argv[i] = i % 2 == 0 ? "-" : "- ";
    }
    SET_OPTION(0, 1, '-', NULL);
    ASSERT(argve_init(&state, options, argc, argv));

    for (i = 0; i < argc; i++) {
        if (i % 2 == 0) {
            ASSERT(argve_next(&state) == ARGVE_TEXT);
            ASSERT(state.option == NULL);
            ASSERT(state.optstr == NULL);
            ASSERT(state.argstr == argv[i]);
            ASSERT(state.type == ARGVE_TEXT);
            EXPECT_STRING("-", state.argstr);
        } else {
            ASSERT(argve_next(&state) == ARGVE_ERR_DEF_SHORT);
            ASSERT(argve_error(&state));
            ASSERT(state.option == NULL);
            ASSERT(state.optstr == argv[i] + 1);
            ASSERT(state.argstr == NULL);
            ASSERT(state.type == ARGVE_ERR_DEF_SHORT);
            EXPECT_STRING("- ", state.optstr - 1);
        }
        ASSERT(state.options == options);
        ASSERT(state.argc == argc - i - 1);
        ASSERT(state.argv == argv + i + 1);
    }
    ASSERT(i == argc);
    ASSERT(argve_next(&state) == ARGVE_END);
    ASSERT_ARGVE_END;
}

TEST_CASE("double dashes", case_dashes_2) {
    SETUP_WITH_ARGV(256);

    for (i = 0; i < argc; i++) {
        argv[i] = i % 2 == 0 ? "--" : "---";
    }
    SET_OPTION(0, 1, '-', "-");
    ASSERT(argve_init(&state, options, argc, argv));

    for (i = 0; i < argc; i++) {
        if (i % 2 == 0) {
            ASSERT(argve_next(&state) == ARGVE_TEXT);
            ASSERT(state.option == NULL);
            ASSERT(state.optstr == NULL);
            ASSERT(state.argstr == argv[i]);
            ASSERT(state.type == ARGVE_TEXT);
            EXPECT_STRING("--", state.argstr);
        } else {
            ASSERT(argve_next(&state) == ARGVE_LONG);
            ASSERT(state.option == &options[0]);
            ASSERT(state.optstr == argv[i] + 2);
            ASSERT(state.argstr == argv[i + 1]);
            ASSERT(state.type == ARGVE_LONG);
            ASSERT(state.option->need_arg == 1);
            ASSERT(state.option->shortopt == '-');
            EXPECT_STRING("-", state.option->longopt);
            EXPECT_STRING("---", state.optstr - 2);
            EXPECT_STRING("--", state.argstr);
            i++;
        }
        ASSERT(state.options == options);
        ASSERT(state.argc == argc - i - 1);
        ASSERT(state.argv == argv + i + 1);
    }
    ASSERT(i == argc);
    ASSERT(argve_next(&state) == ARGVE_END);
    ASSERT_ARGVE_END;
}

/* NOTE: variable memory not fully initialized may lead to miscompilation */
TEST_CASE("fuzzing", case_fuzzing_1) {
    int const max_arg_size = 256;
    int const max_flag_size = 32;
    int rounds = 1000;
    char *memory = NULL;
    sfc64 rng;
    uint64_t seed = *TEST_DATA(uint64_t *);
    SETUP_WITH_ARGV(256);

    sfc64_seed(&rng, seed);
    /* hand-rolled arena allocator */
    c = k = argc * max_arg_size + MAX_OPTS * max_flag_size;
    ASSERT((memory = (char *)malloc(k)) != NULL);

    while (rounds-- > 0) {
        k = c;
        for (i = 0; i < c; i++) memory[i] = sfc64_rand(&rng, 256);

        for (i = 0; i < argc; i++) {
            j = 1 + sfc64_rand(&rng, max_arg_size);
            argv[i] = memory + (k -= j);  /* allocate memory */
            argv[i][j - 1] = '\0';
        }
        for (i = 0; i < MAX_OPTS; i++) {
            j = 1 + sfc64_rand(&rng, max_flag_size);
            s = memory + (k -= j);  /* allocate memory */
            SET_OPTION(i, 1, s[0], s);
            s[j - 1] = '\0';
            options[i].longopt = argve_to_long_opt(s, 0);
        }
        ASSERT_OR_GOTO(argve_init(&state, options, argc, argv), error);

        for (i = 0; i < argc && state.argc > 0; i++) {
            /* no crashing, no segfault ? */
            ASSERT_OR_GOTO(argve_next(&state) != ARGVE_END, error);
        }
        ASSERT_OR_GOTO(argve_next(&state) == ARGVE_END, error);
        ASSERT_OR_GOTO(state.options == options, error);
        ASSERT_OR_GOTO(state.option == NULL, error);
        ASSERT_OR_GOTO(state.optstr == NULL, error);
        ASSERT_OR_GOTO(state.argstr == NULL, error);
        ASSERT_OR_GOTO(state.argc == 0, error);
        ASSERT_OR_GOTO(state.argv == argv + argc, error);
        ASSERT_OR_GOTO(state.type == ARGVE_END, error);
    }
error:
    free(memory);
}

TEST_SUITE("argve_init", suite_init) {
    TEST(case_init_1, NULL);
    TEST(case_init_2, NULL);
    TEST(case_init_3, NULL);
    TEST(case_init_4, NULL);
    TEST(case_init_5, NULL);
    TEST(case_init_6, NULL);
}

TEST_SUITE("positional", suite_positional) {
    TEST(case_positional_1, NULL);
    TEST(case_positional_2, NULL);
    TEST(case_positional_3, NULL);
}

TEST_SUITE("options", suite_options) {
    TEST(case_options_1, NULL);
    TEST(case_options_2, NULL);
    TEST(case_options_3, NULL);
    TEST(case_options_4, NULL);
    TEST(case_options_5, NULL);
    TEST(case_options_6, NULL);
    TEST(case_options_7, NULL);
    TEST(case_options_8, NULL);
    TEST(case_options_9, NULL);
}

TEST_SUITE("dashes", suite_dashes) {
    TEST(case_dashes_1, NULL);
    TEST(case_dashes_2, NULL);
}

TEST_SUITE("fuzzing", suite_fuzzing) {
    TEST(case_fuzzing_1, TEST_DATA(uint64_t *));
}

TEST_MAIN {
    uint64_t seed = 0, n; char *s; struct timeval tv;

    if ((s = getenv("SEED")) != NULL && *s != '\0') {
        while ('0' <= *s && *s <= '9') {
            seed = (seed * 10) + (*s++ - '0');
        }
        if (*s != '\0') {
            ERROR_EXIT(SEED);
        }
    } else {
        if (gettimeofday(&tv, NULL) != 0) {
            ERROR_EXIT(gettimeofday);
        }
        n = sizeof(time_t) - sizeof(suseconds_t);
        seed = tv.tv_sec ^ ((time_t)tv.tv_usec << n * 8);
    }
    fprintf(stdout, "[INFO] SEED = %" PRIu64 "\n", seed);

    RUN(suite_init, NULL);
    RUN(suite_positional, NULL);
    RUN(suite_options, NULL);
    RUN(suite_dashes, NULL);
    RUN(suite_fuzzing, &seed);
}
