#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

#include <pthread.h>
#include <time.h>
#include <stdint.h>

/*
    timer - abstraction that allows to mark a deadline in future and verify
    whether the deadline has expired.

    clock_gettime(CLOCK_MONOTONIC) is used to measure time intervals. It might
    be better to use CLOCK_MONOTONIC_RAW, but it's not available on this system.

    There are two sets of methods - thread-safe and thread-unsafe. They have
    almost the same behavior and differ mostly in that methods with _locked suffix
    protect access to a 'struct timer' with a mutex.

    Usage example:
    <code>
        static struct timer timer = TIMER_INITIALIZER;

        // executes on thread A:
        void onEventA()
        {
            ...
            timer_set_locked(&timer, 5000); // set timer to expire after 5 seconds
            ...
        }

        // executes on thread B:
        void onEventB()
        {
            ...
            timer_invalidate_locked(&timer);
            ...
        }

        // executes on thread C:
        void eventLoop()
        {
            ...
            int expired = timer_expired_locked(&timer);
            if (expired == -1) // was invalidated
            {
                ...
            }
            else if (expired) // actually expired
            {
                ...
            }
            ...
        }
    </code>
*/

/*
    You should not rely on implementation details of this structure. Think of it
    as an opaque data type.
*/
struct timer {
    volatile int status; // fixme: can it be that this variable must be atomic?
    pthread_mutex_t lock;
    struct timespec start;
    struct timespec deadline;
};

/*
    Similarly to PTHREAD_MUTEX_INITIALIZER you can use this macro to initialize
    struct by assignment. Initialization sets timer to invalid state. Example
    usage of initializer:

    <code>
        struct timer timer = TIMER_INITIALIZER;
    </code>

    It is safe to initialize static variables in that way. It is probably unsafe
    to use the macro to initialize local variables. Older versions of POSIX
    standard state that PTHREAD_MUTEX_INITIALIZER can only be used with static
    variables. More recent version doesn't contain that constraint, but since
    our environment is a bit old, use it at your own risk.
*/
#define TIMER_INITIALIZER { 1 << 1, PTHREAD_MUTEX_INITIALIZER, {0, 0}, {0, 0} };

/*
    Initialize a timer and set it to invalid state.  If timer is already
    initialized, this will print a message to log and return immediately.
    Alternatively you can initialize a timer using initializer macro. You can
    omit initialization entirely if you don't use locked methods. Usage of
    uninitialized timer with locked methods results in undefined behavior.

    'timer' must be a valid pointer to timer.
*/
void timer_init(struct timer *timer);

/*
    Destroy a timer and set it to invalid state. If timer is not initialized
    (wasn't initialized at all or was destroyed already), this will print a
    message to log and return immediately.

    'timer' must be a valid pointer to timer.
*/
void timer_destroy(struct timer *timer);

/*
    Set the remaining time for this timer and set timer to valid state. Timer will
    expire after 'msec' milliseconds from current moment will pass.

    'msec' is remaining time in milliseconds. Should not be negative. If 'msec' is
    negative, this will print a message to log, set timer to invalid state and
    return immediately.

    'timer' must be a valid pointer to timer.
*/
void timer_set(struct timer *timer, int64_t msec);

/*
    Get the remaining time for this timer.

    'timer' must be a valid pointer to timer.

    Returns -1 if timer is in invalid state, 0 if timer expired, and remaining
    time in milliseconds otherwise.
*/
int64_t timer_remaining(const struct timer *timer);

/*
    Get the elapsed time for this timer since last timer_set() call.

    'timer' must be a valid pointer to timer.

    Returns -1 if timer is in invalid state, and elapsed time in milliseconds
    otherwise.
*/
int64_t timer_elapsed(const struct timer *timer);

/*
    Check if timer is expired.

    'timer' must be a valid pointer to timer.

    Returns -1 if timer is in invalid state, 0 if timer is not expired and 1 if
    timer is expired.
*/
int timer_expired(const struct timer *timer);

/*
    Check if timer is in valid state.

    'timer' must be a valid pointer to timer.

    Returns 0 if timer is not in valid state and 1 otherwise.
*/
int timer_valid(const struct timer *timer);

/*
    Invalidate timer i.e. set it to invalid state.

    'timer' must be a valid pointer to timer.
*/
void timer_invalidate(struct timer *timer);


/*
    All methods behave as their unlocked counterparts with a little exception: if
    timer is not initialized all methods print a message to log and methods that
    return something also return -1 in this situation.
*/

void timer_set_locked(struct timer *timer, int64_t msec);
int64_t timer_remaining_locked(struct timer *timer);
int64_t timer_elapsed_locked(struct timer *timer);
int timer_expired_locked(struct timer *timer);
int timer_valid_locked(struct timer *timer);
void timer_invalidate_locked(struct timer *timer);

#endif // TIMER_H_INCLUDED
