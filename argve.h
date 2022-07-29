/*==========================================================================*\
) Copyright (c) 2022 by J.W https://github.com/jakwings/argve                (
)                                                                            (
)   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION          (
)                                                                            (
)  0. You just DO WHAT THE FUCK YOU WANT TO.                                 (
\*==========================================================================*/

#ifndef ARGVE_H
#define ARGVE_H

/*\
 / A flexible iterative argument parser implemented in a single header.
 /
 / public functions:
 / * argve_init(state, options, arguments) -> bool
 / * argve_next(state) -> result_type
 / * argve_error(state) -> bool
 / * argve_perror(state, label, stream) -> int
 / helper functions:
 / * argve_advance(state)
 / * argve_option_end(option) -> bool
 / * argve_to_shortopt(char) -> int
 / * argve_to_longopt(string, search) -> string
 / * argve_match_short(options, char) -> option
 / * argve_match_long(options, argument) -> option
 / data types:
 / * argve_option
 / * argve_state
 / * argve_type
\*/

#ifndef ARGVE_H_API
#define ARGVE_H_API  /* static, __attribute__, __declspec, [[...]], etc. */
#endif

#ifndef ARGVE_H_FREESTANDING
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ARGVE_END = 0,        /* no more argument */
    ARGVE_TEXT,           /* positional argument; includes "--" and "-" */
    ARGVE_SHORT,          /* short flag excluding -long */
    ARGVE_LONG,           /* long flag excluding -long and "--=[<value>]" */
    ARGVE_CLUSTER,        /* cluster of short flags */
    ARGVE_ERR_DEF_SHORT,  /* unrecognized short flag */
    ARGVE_ERR_DEF_LONG,   /* unrecognized long flag */
    ARGVE_ERR_ARG_SHORT,  /* missing argument for short flag */
    ARGVE_ERR_ARG_LONG,   /* missing/unneeded argument for long flag */
    ARGVE_ERR_UNKNOWN     /* unknown error */
} argve_type;

typedef struct {
    int need_arg;   /* boolean */
    int shortopt;   /* 0 for dummy; number outside 0x21-7E is virtual code */
    char *longopt;  /* NULL for dummy */
} argve_option;

typedef struct {
    argve_option *options;
    argve_option *option;
    char *optstr;  /* -<a>bc (-ab c; -a bc; -a -b -c) --<option>[=value] */
    char *argstr;  /* -w<width> --width=<width> --width <width> */
    char **argv;   /* unprocessed arguments */
    int argc;      /* number of unprocessed arguments in argv */
    argve_type type;
} argve_state;

/*\
 / Return 1 if an error happened, otherwise return 0.
\*/
ARGVE_H_API
int argve_error(argve_state *state)
#ifdef ARGVE_H_WITH_IMPLEMENTATION
{
    return state->type >= ARGVE_ERR_DEF_SHORT;
}
#else
;
#endif /* ARGVE_H_WITH_IMPLEMENTATION */

#ifndef ARGVE_H_FREESTANDING
/*\
 / Output a diagnostic message according to the parser state.
 /
 / Return the return value from fprintf(3).
\*/
ARGVE_H_API
int argve_perror(argve_state *state, const char *label, FILE *stream)
#ifdef ARGVE_H_WITH_IMPLEMENTATION
{
    char c; const char *p, *q;

    switch (state->type) {
    case ARGVE_ERR_ARG_SHORT:
        c = state->optstr[0];
        q = state->argv[-1];
        return fprintf(stream, "%s" "missing argument for -%c : %s\n",
                       label, c, q);
    case ARGVE_ERR_ARG_LONG:
        p = state->option->longopt;
        if (!state->option->need_arg) {
            return fprintf(stream, "%s" "missing argument for --%s\n",
                           label, p);
        } else {
            q = state->argv[-1];
            return fprintf(stream, "%s" "unneeded argument for --%s : %s\n",
                           label, p, q);
        }
    case ARGVE_ERR_DEF_SHORT:
        c = state->optstr[0];
        q = state->argv[-1];
        return fprintf(stream, "%s" "unrecognized option -%c : %s\n",
                       label, c, q);
    case ARGVE_ERR_DEF_LONG:
        q = state->argv[-1];
        p = q;
        while (*p && *p != '=') p += 1;
        return fprintf(stream, "%s" "unrecognized option %.*s : %s\n",
                       label, (int)(p - q), q, q);
    case ARGVE_ERR_UNKNOWN:
        goto error;
    default:
        return fprintf(stream, "%s%s\n", label, "no error");
    }
error:
    return fprintf(stream, "%s%s\n", label, "unknown error");
}
#else
;
#endif /* ARGVE_H_WITH_IMPLEMENTATION */
#endif /* !ARGVE_H_FREESTANDING */

#ifdef ARGVE_H_WITH_IMPLEMENTATION
/*\
 / Advance to the next argument.
\*/
static
void argve_advance(argve_state *state)
{
    state->argc -= 1;
    state->argv += 1;
}

/*\
 / Return 1 if there is the end of options, otherwise return 0.
\*/
static
int argve_option_end(const argve_option *opts)
{
    return opts->shortopt == 0 && opts->longopt == NULL;
}

/*\
 / Return <c> if it is a visible ASCII char except '-', otherwise return 0.
\*/
static
int argve_to_short_opt(char c)
{
    return (c != '-' && c > 0x20 && c < 0x7F) ? (unsigned char)c : 0;
}

/*\
 / Find the short option with character/virtual code <c>.
 /
 / <c> is the scalar value of a visible ASCII character except for '-',
 / otherwise it is taken as a virtual code, except that 0 is always used as
 / a dummy value for long flags without a corresponding short flag.
 /
 / "-" and "--" are never treated as short flags as such.  An argument like
 / "-o-" is not a proper cluster of short flags when -o is a valid flag but
 / does not require an argument, i.e. "-" in this case.
\*/
static
argve_option * argve_match_short(const argve_option *opts, int c)
{
    if (opts == NULL || c == 0) {
        return NULL;
    }
    while (!argve_option_end(opts)) {
        if (opts->shortopt == c) {
            return (argve_option *)opts;
        }
        opts += 1;
    }
    return NULL;
}

/*\
 / Get the start address of the name of a valid long flag in <s>.
 /
 / If <search> is 0, <s> is taken as the name of a long flag.
 / Otherwise, <s> is taken as a long flag in the form "--<name>[=<value>]".
 /
 / "-" and "--" are never treated as long flags.  Long flags have a non-empty
 / name consisted of only visible ASCII characters except for the equal sign.
 /
 / Return the address of the valid name if it is found, otherwise return NULL.
\*/
static
char * argve_to_long_opt(const char *s, int search)
{
    char c; const char *p;

    if (s != NULL && *s != '\0') {
        if (search) {
            /* --<name>[=<value>] */
            if (s[0] != '-' || s[1] != '-' || s[2] == '\0' || s[2] == '=') {
                return NULL;
            }
            s += 2;
        }
        p = s;
        c = *p;
        while (c != '=' && c > 0x20 && c < 0x7F) {
            c = *(p += 1);
        }
        if (search) {
            return (c == '\0' || c == '=') ? (char *)s : NULL;
        }
        return c == '\0' ? (char *)s : NULL;
    }
    return NULL;
}

/*\
 / Find the long option with name in string <s> "--<name>[=<value>]".
 /
 / NULL is a dummy value for short flags without a corresponding long flag,
 / and thus never matching any option.
\*/
static
argve_option * argve_match_long(const argve_option *opts, const char *s)
{
    const char *p, *q;

    if (opts == NULL || (s = argve_to_long_opt(s, 1)) == NULL) {
        return NULL;
    }
    while (!argve_option_end(opts)) {
        if ((p = opts->longopt) != NULL && *p) {
            q = s;
            while (*p && *q && *p == *q) {
                p += 1; q += 1;
            }
            if (*p == '\0' && (*q == '\0' || *q == '=')) {
                return (argve_option *)opts;
            }
        }
        opts += 1;
    }
    return NULL;
}
#endif /* ARGVE_H_WITH_IMPLEMENTATION */

/* Parse the next argument.
 /
 / If a cluster of short flags is found in an argument string "-<chars>",
 /     state->optstr is the start of each single-character option name,
 /     state->argstr is NULL, the remaining <chars> or the next argument;
 /
 / If a short flag is found in an argument string "-o[<value>]",
 /     state->optstr is the start of the single-character option name,
 /     state->argstr is either NULL, <value> or the next argument string;
 /
 / If a long flag is found in an argument string "--name[=<value>]",
 /     state->optstr is the start of the option name,
 /     state->argstr is either NULL, <value> or the next argument string;
 /
 / If an error happens,
 /     state->optstr points to undefined option name,
 /     state->argstr is <value> for --name=<value> where <value> is unneeded,
 /     stop processing the current argument,
 /     report the error and prepare to handle the next argument as normal;
 /
 / Otherwise, the next argument is a positional argument.
\*/
ARGVE_H_API
argve_type argve_next(argve_state *state)
#ifdef ARGVE_H_WITH_IMPLEMENTATION
{
    int c; char *arg;

    /* parse cluster of short flags */
    if (state->type == ARGVE_CLUSTER
            && state->argstr == NULL && state->optstr[1] != '\0') {
        /* process the next flag; -abc => -bc */
        state->optstr += 1;
        state->argstr = NULL;
        c = argve_to_short_opt(state->optstr[0]);
        state->option = argve_match_short(state->options, c);
        if (state->option != NULL) {
            /*state->type = ARGVE_CLUSTER;*/
            if (state->option->need_arg) {
                if (state->optstr[1] != '\0') {
                    /* -o<value> */
                    state->argstr = state->optstr + 1;
                } else if (state->argc > 0) {
                    /* -o <value> */
                    state->argstr = state->argv[0];
                    argve_advance(state);
                } else {
                    state->type = ARGVE_ERR_ARG_SHORT;
                }
            }
        } else {
            state->type = ARGVE_ERR_DEF_SHORT;
        }
        return state->type;
    }

    /* parse new argument */
    if (state->argc > 0) {
        state->type = ARGVE_ERR_UNKNOWN;
        state->option = NULL;
        state->optstr = NULL;
        state->argstr = NULL;
        arg = state->argv[0];
        if (arg[0] == '-' && arg[1] != '-' && arg[1] != '\0') {
            /* short flags; visible ASCII chars only; -a -b -ab<value> */
            c = argve_to_short_opt(arg[1]);
            state->option = argve_match_short(state->options, c);
            if (state->option != NULL) {
                state->type = ARGVE_SHORT;
                state->optstr = arg + 1;
                argve_advance(state);
                if (state->option->need_arg) {
                    if (state->optstr[1] != '\0') {
                        /* -o<value> */
                        state->argstr = state->optstr + 1;
                    } else if (state->argc > 0) {
                        /* -o <value> */
                        state->argstr = state->argv[0];
                        argve_advance(state);
                    } else {
                        state->type = ARGVE_ERR_ARG_SHORT;
                    }
                } else if (state->optstr[1] != '\0') {
                    state->type = ARGVE_CLUSTER;
                }
            } else {
                state->type = ARGVE_ERR_DEF_SHORT;
                state->optstr = arg + 1;
                argve_advance(state);
            }
        } else if (arg[0] == '-' && arg[1] == '-' && arg[2] != '\0') {
            /* long flags; visible ASCII chars only; --name --name=<value> */
            state->option = argve_match_long(state->options, arg);
            arg += 2;
            if (state->option != NULL) {
                state->type = ARGVE_LONG;
                state->optstr = arg;
                argve_advance(state);
                while (*arg && *arg != '=') {
                    arg += 1;
                }
                if (state->option->need_arg) {
                    if (*arg) {
                        /* --name=<value> */
                        state->argstr = arg + 1;
                    } else if (state->argc > 0) {
                        /* --name <value> */
                        state->argstr = state->argv[0];
                        argve_advance(state);
                    } else {
                        state->type = ARGVE_ERR_ARG_LONG;
                    }
                } else if (*arg) {
                    state->type = ARGVE_ERR_ARG_LONG;
                    state->argstr = arg + 1;
                }
            } else {
                state->type = ARGVE_ERR_DEF_LONG;
                state->optstr = arg;
                argve_advance(state);
            }
        } else {
            /* positional arguments including "--" and "-" */
            state->type = ARGVE_TEXT;
            state->argstr = arg;
            argve_advance(state);
        }
    } else {
        state->type = ARGVE_END;
        state->option = NULL;
        state->optstr = NULL;
        state->argstr = NULL;
    }
    return state->type;
}
#else
;
#endif /* ARGVE_H_WITH_IMPLEMENTATION */

/*\
 / Initialize the parser state for iteration of command-line arguments.
 /
 / Must be called before the first use of argve_next with new parameters.
 /
 / Return 1 if initialization succeeds, otherwise return 0.
\*/
ARGVE_H_API
int argve_init(argve_state *state, argve_option *opts, int argc, char **argv)
#ifdef ARGVE_H_WITH_IMPLEMENTATION
{
    argve_option *p = opts;
    int i;

    if (state == NULL || opts == NULL || argc < 0 || argv == NULL) {
        return 0;
    }
    while (!argve_option_end(p)) {
        /* names of long flags must contain only visible ASCII chars */
        if (p->longopt != NULL && argve_to_long_opt(p->longopt, 0) == NULL) {
            return 0;
        }
        p += 1;
    }
    /* argv needs not be NULL-terminated, but must not contain any NULL */
    for (i = 0; i < argc; i += 1) {
        if (argv[i] == NULL) {
            return 0;
        }
    }
    state->options = opts;
    state->option = NULL;
    state->optstr = NULL;
    state->argstr = NULL;
    state->argv = argv;
    state->argc = argc;
    state->type = ARGVE_ERR_UNKNOWN;
    return 1;
}
#else
;
#endif /* ARGVE_H_WITH_IMPLEMENTATION */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ARGVE_H */
