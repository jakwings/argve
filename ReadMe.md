# argve.h -- yet another iterative argument parser in a single header

Features:

* inspired by [optparse](https://github.com/skeeto/optparse)
* written with C89 & C++11 in mind
* no dynamic memory allocation
* no auto permutation of `char *argv[]` (you can Do It Yourself)
* can distinguish `-abc` from `-a -bc`, `--key=` from `--key ""`
* accepts long flags like `--long` but not `-long`
* no optional values like `-o[value]` or `--option[=value]` (DIY)
* no auto skipping of `--` and the arguments following it
* never parses `--` as a flag but takes it as a trivial argument
* special cases like `-o-`, `--=`, `-` and `---` are considered
* parser state never corrupted by parsing error, always advancing
* thoroughly tested by `make test` (see [test2.c](tests/test2.c))

APIs:

* `argve_init(state, options, arguments) -> bool`
* `argve_next(state) -> result_type`
* `argve_error(state) -> bool`
* `argve_perror(state, label, stream) -> int`

Data types:

* `struct argve_option`
* `struct argve_state`
* `enum argve_type`

Please read the documentation in the source code.

Beloew is a program with support for subcommands while abusing argve:

```c
/*\
 / demo.c -- abusing fault tolerance for fun
 /
 / Usage: demo [options] [<command> [options] [<arguments>]]
\*/
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define ARGVE_H_WITH_IMPLEMENTATION
#define ARGVE_H_API static
#include "argve.h"

int main(int argc, char **argv)
{
    /* an empty list of options */
    argve_option options[] = {
        {/*need_arg*/ 0, /*shortopt*/ 0, /*longopt*/ NULL}
    };
    argve_state state;

    /* use <args> to rearrange positional arguments */
    char **args = argv + 1, *cmd = NULL;
    int verbose = 0, dashdash = 0; size_t length = 0;

    /* argv[0] is reserved for the name of this program */
    assert(argve_init(&state, options, argc - 1, argv + 1));

    while (argve_next(&state) != ARGVE_END) {
        /* unrecognized option ? */
        if (state.optstr != NULL) {
            /* state.argv points to the unprocessed arguments */
            length += strlen(state.argv[-1]);
            if (dashdash) {
                *args++ = state.argv[-1];
                goto dashdash;
            } else if (strcmp("verbose", state.optstr) == 0) {
                /* global options: -verbose, --verbose */
                verbose = verbose || (cmd == NULL);
            } else if (strcmp("help", state.optstr) == 0) {
                printf("demo [--verbose] <command> <arguments>\n");
                return 0;
            } else if (state.optstr[0] == 'h') {
                printf("[DEMO] Eh? %s? Use your words, moron.\n",
                        state.argv[-1]);
                return 0;
            } else {
                argve_perror(&state, "[DEMO] ", stderr);
            }
        } else if (state.argstr != NULL) {
            /* positional argument */
            length += strlen(state.argstr);
            if (!dashdash && strcmp("--", state.argstr) == 0) {
                dashdash = 1;
            } else if (cmd == NULL) {
                cmd = state.argstr;
                dashdash = 0;
            } else if (dashdash) {
                *args++ = state.argstr;
dashdash:
                /* copy the remaing args */
                while (state.argc-- > 0) {
                    length += strlen(*state.argv);
                    *args++ = *state.argv++;
                }
            } else {
                *args++ = state.argstr;
            }
        } else {
            /* unexpected error, must be a bug */
            argve_perror(&state, "\n[ERROR] ", stderr);
            assert(argve_error(&state));
            return 1;
        }
    }
    if (cmd == NULL) {
        printf("[DEMO] Eww, NO command?\n");
    } else {
        if (verbose) {
            while (--args != argv) {
                printf("[DEMO] You mere mortals dare to %s %s!?\n",
                        cmd, *args);
            }
            printf("[DEMO] Go %s yourself.\n", cmd);
        }
        printf("[DEMO] You just wasted %lu bytes of text!\n", length);
    }

    return 0;
}
```

More proper use of the APIs shall be found in [test3.c](tests/test3.c).


------------------------------------------------------------------------

# test.h -- a single-header unit testing framework

Features:

* less boilerplate code to write
* less automation but more flexibility
* definitely okay to feed yourself some TODO notes
* compatible with C89 & C++11, no fancy macro magic
* no multi-thread safety yet (counters and printf) but easy to solve

Sample code in [test1.c](tests/test1.c):

    #include "test.h"

    TEST_CASE("...", case_xxx) {
        ASSERT(yes); ...
    }
    TEST_CASE("...", case_yyy) {
        EXPECT("yeah", right); ...
    }
    TEST_CASE("...", case_zzz) {
        Fact *fact = TEST_DATA(Fact *);
        Memory *memory = recall();
        ASSERT_OR_GOTO(memory->good, free);
        EXPECT_OR_GOTO("truth", memory->matches(fact), free);
        RETURN;
    free:
        free(memory); ...
    }
    ...

    TEST_SUITE("...", suite_xyz) {
        Fact *fact = TEST_DATA(Fact *);
        TEST(case_xxx, NULL);
        TEST(case_yyy, NULL);
        TEST(case_zzz, fact);
        ...
    }

    TEST_CASE("more", case_more) {
        ...
        TODO;
    }
    TEST_SUITE("more cases", suite_more) {
        TEST(case_more, NULL);
        TODO;
    }

    TEST_MAIN {
        Fact fact = {0, 1, ...};
        RUN(suite_xyz, &fact);
        RUN(suite_more, NULL);
    }

To see how great it could be, just clone this repo and run `make test`.

The compiled unit testing programs provide some additional goodies:

```
Usage: program [options]

Synopsis:
    program -h
    program [-q] [-c <case>]...
    program [-q] [-s <suite> [-c <case>]...]...
    program [-q] [-c <case>]... [-s <suite> [-c <case>]...]...

Options:
    -h, --help            Show this help information.
    -q, --quiet           Do not output logs of operations.
    -s, --suite <name>    Only run test cases in the specific suite.
    -c, --case <name>     Only run the specific test case in the suite.
```


------------------------------------------------------------------------

EOF
