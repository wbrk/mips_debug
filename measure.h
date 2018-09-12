#ifndef MEASURE_H_INCLUDED
#define MEASURE_H_INCLUDED

#include <time.h>

/*
    Small set of functions that allow to measure execution time.

    FUNCTIONS FROM THIS MODULE ARE NOT THREAD-SAFE IF IT IS NOT STATED EXPLICITLY.
    I wanted to make everything thread-safe, but it seems that our compiler
    doesn't support thread-local storage. One can use TLS that is provided by
    pthread library, buy I have heard that this implementation is rather slow,
    and I anyway don't have enough time to check it out.

    clock_gettime(CLOCK_MONOTONIC) is used to measure time intervals. It might
    be better to use CLOCK_MONOTONIC_RAW, but it's not available on this system.

    Three functions -- measure_start(), measure_print(), measure_get() -- allow
    you to measure code execution time. These functions use stack of
    'struct timespec' variables to allow you to nest measurements. Successive
    calls to measure_start() store to different memory regions and calls to
    measure_print() and measure_get() give you time passed from their
    corresponding measure_start() call. This is useful when you want to measure
    time in some function, its callees and callees of its callees.

    Usecases:

    This will print how much time was spent between measure_start() and
    measure_print():
    <code>
        measure_start();
        some_routine();
        measure_print("some_routine()");
    </code>

    If you measure some complicated process, it might be handy to write
    something like this:
    <code>
        measure_start();
        some_routine();                        // takes 90 ms
        measure_print("some_routine()");       // prints "some_routine() took 0.09 seconds"
        some_other_routine();                  // takes 100 ms
        measure_print("some_other_routine()"); // prints "some_other_routine() took 0.19 seconds"
    </code>
    In this fashion after each measure_print() you will obtain time passed from
    the single starting point to specific measure_print(). This will only work
    if internal stack is empty when you call measure_start().

    If you need to measure time in some function and its callees, you can write
    something like this:
    <code>
        void foo() {
            measure_start();

            measure_start();
            bar();
            measure_print(); // prints 'took 0.95'

            measure_start();
            baz();
            measure_print(); // prints 'took 0.4'

            measure_print(); // prints 'took 1.35'
        }

        void bar() {
            measure_start();
            baz();
            qux();
            measure_print(); // prints 'took 0.7'
            usleep(250000);
        }

        void baz() {
            measure_start();
            qux();
            measure_print(); // prints 'took 0.3';
            usleep(100000);
        }

        void qux() {
            measure_start();
            usleep(300000);
            measure_print(); // prints 'took 0.3'
        }
    </code>
*/

/*
    Calculate difference between two time values. All arguments must not be NULL.
    Value pointed to by 'start' must be less or equal than value, pointed to
    by 'end'. Result will be stored to the struct pointed to by 'result'. 'end'
    and 'result' can point to the same location (hence pointer to 'end' is not
    const). Returns 'result'. Thread-safe since operates only on arguments
    passed.
*/
struct timespec *measure_diff(const struct timespec *start,
  struct timespec *end, struct timespec *result);

/*
    Store current time to internal stack. Note that stack used by this module is
    limited to 16 values, i.e. you can store only 16 starting points using
    measure_start(). If stack is already full, does nothing.
*/
void measure_start();

/*
    Print difference between time stored at the top of internal stack and current
    moment. Pops value from stack. If stack is empty, previous top of the stack
    is used to calculate difference. If log is  enabled and log sink is
    specified, message will be printed to log with DEBUG level. Otherwise message
    will be printed to stdout. 'comment' must not be NULL. 'comment' is used as
    a part of message to be printed. Resulting message is something like this:
    $comment took 1.092398 seconds
*/
void measure_print(const char *comment);

/*
    Get difference between time stored at the top of internal stack and current
    moment. Pops value from stack. If stack is empty, previous top of the stack
    is used. 'diff' must not be NULL. Result will be stored to the struct
    pointed to by 'diff'. Returns 'diff'.
*/
struct timespec *measure_get(struct timespec *diff);

#endif // MEASURE_H_INCLUDED
