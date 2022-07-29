#!/bin/sh

# Copyright (c) 2022 by J.W https://github.com/jakwings/test.h
#
#   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
#
#  0. You just DO WHAT THE FUCK YOU WANT TO.

set -euf; unset -v IFS; export LC_ALL=C

BIN="${1-"./bin/test"}"

echo() {
  printf '%s\n' "$*"
}
test() {
  echo
  "${BIN}" ${1+"$@"} 2>&1 \
    | sed -e '/^\[INFO\] Tests starting/ d' \
          -e '/^\[INFO\] Time used/ d'
}

expected='
[TEST] basic : addition ... PASSED
[TEST] basic : subtraction ... PASSED
[TEST] basic : division ... FAILED at test.c#L21 suite_basic.case_div: 1 / 2 == 0.5
[TEST] basic : multiplication ... PASSED
[TEST] advanced : answer ... PASSED
[TEST] advanced : fizzbuzz ... PASSED
[TEST] more : more ... TODO
[TEST] suite_more -- more ... TODO
[INFO] Total / Passed / Failed: 6 / 5 / 1
'
[ "${expected%?}" = "$(test)" ]

expected='
[TEST] basic : addition ... PASSED
[TEST] basic : subtraction ... PASSED
[TEST] basic : division ... FAILED at test.c#L21 suite_basic.case_div: 1 / 2 == 0.5
[TEST] basic : multiplication ... PASSED
[TEST] advanced : answer ... PASSED
[TEST] advanced : fizzbuzz ... PASSED
[INFO] Total / Passed / Failed: 6 / 5 / 1
'
[ "${expected%?}" = "$(test -s basic -s advanced)" ]
[ "${expected%?}" = "$(test -s suite_basic -s suite_advanced)" ]

expected='
[TEST] basic : addition ... PASSED
[TEST] basic : subtraction ... PASSED
[TEST] basic : division ... FAILED at test.c#L21 suite_basic.case_div: 1 / 2 == 0.5
[TEST] basic : multiplication ... PASSED
[INFO] Total / Passed / Failed: 4 / 3 / 1
'
[ "${expected%?}" = "$(test -s basic)" ]
[ "${expected%?}" = "$(test -s suite_basic)" ]

expected='
[TEST] advanced : answer ... PASSED
[TEST] advanced : fizzbuzz ... PASSED
[INFO] Total / Passed / Failed: 2 / 2 / 0
'
[ "${expected%?}" = "$(test -s advanced)" ]
[ "${expected%?}" = "$(test -s suite_advanced)" ]

expected='
[TEST] advanced : answer ... PASSED
[TEST] suite_more -- more ... TODO
[INFO] Total / Passed / Failed: 1 / 1 / 0
'
[ "${expected%?}" = "$(test -c answer)" ]
[ "${expected%?}" = "$(test -c case_answer)" ]

expected='
[TEST] basic : division ... FAILED at test.c#L21 suite_basic.case_div: 1 / 2 == 0.5
[TEST] advanced : answer ... PASSED
[TEST] advanced : fizzbuzz ... PASSED
[INFO] Total / Passed / Failed: 3 / 2 / 1
'
[ "${expected%?}" = "$(test -s basic -c division -s advanced)" ]
[ "${expected%?}" = "$(test -s suite_basic -c case_div -s suite_advanced)" ]

expected='
[TEST] basic : division ... FAILED at test.c#L21 suite_basic.case_div: 1 / 2 == 0.5
[TEST] advanced : answer ... PASSED
[TEST] advanced : fizzbuzz ... PASSED
[TEST] suite_more -- more ... TODO
[INFO] Total / Passed / Failed: 3 / 2 / 1
'
[ "${expected%?}" = "$(test -c division -s advanced)" ]
[ "${expected%?}" = "$(test -c case_div -s suite_advanced)" ]

expected='
[TEST] basic : division ... FAILED at test.c#L21 suite_basic.case_div: 1 / 2 == 0.5
[TEST] advanced : answer ... PASSED
[TEST] advanced : fizzbuzz ... PASSED
[TEST] suite_more -- more ... TODO
[INFO] Total / Passed / Failed: 3 / 2 / 1
'
[ "${expected%?}" = "$(test -c division -c answer -c fizzbuzz)" ]
[ "${expected%?}" = "$(test -c case_div -c case_answer -c case_fizzbuzz)" ]

expected='
[TEST] basic : division ... FAILED at test.c#L21 suite_basic.case_div: 1 / 2 == 0.5
[INFO] Total / Passed / Failed: 1 / 0 / 1
'
[ "${expected%?}" = "$(test -s basic -c division -c answer --)" ]
[ "${expected%?}" = "$(test -s basic -c case_div -c case_answer --)" ]
