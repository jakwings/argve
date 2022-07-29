#ifdef TEST_H_DEBUGGING
#line 3 "test.c"
#endif

#include <test.h>

TEST_CASE("addition", case_add) {
    ASSERT(0 + 1 == 1);
    ASSERT(1 + 0 == 1);
}
TEST_CASE("subtraction", case_sub) {
    ASSERT(1 - 0 == 1);
    ASSERT(0 - 1 == -1);
}
TEST_CASE("multiplication", case_mul) {
    ASSERT(0 * 1 == 0);
    ASSERT(1 * 0 == 0);
}
TEST_CASE("division", case_div) {
    ASSERT(2 / 1 == 2.0);
    ASSERT(1 / 2 == 0.5);
}
TEST_SUITE("basic", suite_basic) {
    TEST(case_add, NULL);
    TEST(case_sub, NULL);
    TEST(case_div, NULL);
    TEST(case_mul, NULL);
}

TEST_CASE("answer", case_answer) {
    EXPECT("The answer to life, the universe, and everything is 42?",
            *TEST_DATA(int *) == 42);
}
TEST_CASE("fizzbuzz", case_fizzbuzz) {
    int number = *TEST_DATA(int *);
    ASSERT(number % 3 == 0);
    ASSERT_OR_GOTO(number % 5 == 0, ignore);
    RETURN;
ignore:
    EXPECT_OR_GOTO("stop this silly game", number % 5 == 2, ignore);
}
TEST_SUITE("advanced", suite_advanced) {
    int answer = *TEST_DATA(int *);
    TEST(case_answer, &answer);
    TEST(case_fizzbuzz, &answer);
}

TEST_CASE("more", case_more) {
    TODO;
}
TEST_SUITE("more", suite_more) {
    TEST(case_more, NULL);
    TODO;
}

TEST_MAIN {
    int answer = 42;
    RUN(suite_basic, NULL);
    RUN(suite_advanced, &answer);
    RUN(suite_more, NULL);
}
