#include "timer.h"

#include "log.h"
#include "measure.h"

#include <string.h>

static const long int NANOSECONDS_IN_SECOND = 1000000000;
static const long int NANOSECONDS_IN_MILLISECOND = 1000000;
static const long int MILLISECONDS_IN_SECOND = 1000;

static void valid_set(struct timer *timer);
static void valid_unset(struct timer *timer);
static int valid(const struct timer *timer);

static void initialized_set(struct timer *timer);
static int initialized(const struct timer *timer);

static int64_t timespec_to_ms(const struct timespec *t);
static int timespec_cmp(const struct timespec *a, const struct timespec *b);

// public

void timer_init(struct timer *timer) {
    if (initialized(timer)) {
        LOGW("%s(): timer is already initialized", __func__);
        return;
    }

    timer->status = 0;
    initialized_set(timer);
    pthread_mutex_init(&timer->lock, NULL);
}

void timer_destroy(struct timer *timer) {
    if (!initialized(timer)) {
        LOGW("%s(): timer is already deinitialized", __func__);
        return;
    }

    timer->status = 0;
    pthread_mutex_destroy(&timer->lock);
}

void timer_set(struct timer *timer, int64_t msec) {
    if (msec < 0) {
        valid_unset(timer);
        LOGE("%s(): negative time interval: %lld", __func__, msec);
        return;
    }

    clock_gettime(CLOCK_MONOTONIC, &timer->start);
    memcpy(&timer->deadline, &timer->start, sizeof(struct timespec));
    // memcpy() allows to eliminate a clock_gettime() syscall

    timer->deadline.tv_sec += msec / MILLISECONDS_IN_SECOND;
    timer->deadline.tv_nsec += (msec % MILLISECONDS_IN_SECOND) * NANOSECONDS_IN_MILLISECOND;
    if (timer->deadline.tv_nsec >= NANOSECONDS_IN_SECOND) {
        timer->deadline.tv_nsec -= NANOSECONDS_IN_SECOND;
        timer->deadline.tv_sec += 1;
    }

    valid_set(timer);
}

int64_t timer_remaining(const struct timer *timer) {
    if (!valid(timer)) {
        return -1;
    }

    struct timespec diff;
    clock_gettime(CLOCK_MONOTONIC, &diff);
    if (timespec_cmp(&diff, &timer->deadline) >= 0) { // timer expired
        return 0;
    }

    // It's ok to pass timer to measure_diff, it won't modify it.
    measure_diff(&diff, &timer->deadline, &diff);
    return timespec_to_ms(&diff);
}

int64_t timer_elapsed(const struct timer *timer) {
    if (!valid(timer)) {
        return -1;
    }

    struct timespec diff;
    clock_gettime(CLOCK_MONOTONIC, &diff);
    measure_diff(&timer->start, &diff, &diff);
    return timespec_to_ms(&diff);
}

int timer_expired(const struct timer *timer) {
    if (!valid(timer)) {
        return -1;
    }

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return timespec_cmp(&now, &timer->deadline) >= 0 ? 1 : 0;
}

int timer_valid(const struct timer *timer) {
    return valid(timer) ? 1 : 0;
}

void timer_invalidate(struct timer *timer) {
    valid_unset(timer);
}

// public locked

void timer_set_locked(struct timer *timer, int64_t msec) {
    if (!initialized(timer)) {
        LOGE("%s(): timer isn't initialized! Can't lock!", __func__);
        return;
    }

    pthread_mutex_lock(&timer->lock);
    timer_set(timer, msec);
    pthread_mutex_unlock(&timer->lock);
}

int64_t timer_remaining_locked(struct timer *timer) {
    if (!initialized(timer)) {
        LOGE("%s(): timer isn't initialized! Can't lock!", __func__);
        return -1;
    }

    pthread_mutex_lock(&timer->lock);
    int64_t result = timer_remaining(timer);
    pthread_mutex_unlock(&timer->lock);

    return result;
}

int64_t timer_elapsed_locked(struct timer *timer) {
    if (!initialized(timer)) {
        LOGE("%s(): timer isn't initialized! Can't lock!", __func__);
        return -1;
    }

    pthread_mutex_lock(&timer->lock);
    int64_t result = timer_elapsed(timer);
    pthread_mutex_unlock(&timer->lock);

    return result;
}

int timer_expired_locked(struct timer *timer) {
    if (!initialized(timer)) {
        LOGE("%s(): timer isn't initialized! Can't lock!", __func__);
        return -1;
    }

    pthread_mutex_lock(&timer->lock);
    int result = timer_expired(timer);
    pthread_mutex_unlock(&timer->lock);

    return result;
}

int timer_valid_locked(struct timer *timer) {
    if (!initialized(timer)) {
        LOGE("%s(): timer isn't initialized! Can't lock!", __func__);
        return -1;
    }

    pthread_mutex_lock(&timer->lock);
    int result = timer_valid(timer);
    pthread_mutex_unlock(&timer->lock);

    return result;
}

void timer_invalidate_locked(struct timer *timer) {
    if (!initialized(timer)) {
        LOGE("%s(): timer isn't initialized! Can't lock!", __func__);
        return;
    }

    pthread_mutex_lock(&timer->lock);
    timer_invalidate(timer);
    pthread_mutex_unlock(&timer->lock);
}

// private

enum {
    STATUS_VALID =       1 << 0,
    STATUS_INITIALIZED = 1 << 1
};

static
void valid_set(struct timer *timer) {
    timer->status |= STATUS_VALID;
}

static
void valid_unset(struct timer *timer) {
    timer->status &= ~STATUS_VALID;
}

static
int valid(const struct timer *timer) {
    return timer->status & STATUS_VALID;
}

static
void initialized_set(struct timer *timer) {
    timer->status |= STATUS_INITIALIZED;
}

static
int initialized(const struct timer *timer) {
    return timer->status & STATUS_INITIALIZED;
}

static
int timespec_cmp(const struct timespec *a, const struct timespec *b) {
    if (a->tv_sec == b->tv_sec) {
        return a->tv_nsec == b->tv_nsec ? 0
             : a->tv_nsec > b->tv_nsec  ? 1
             : -1;
    } else if (a->tv_sec > b->tv_sec) {
        return 1;
    } else {
        return -1;
    }
}

static
int64_t timespec_to_ms(const struct timespec *t) {
    int64_t result = t->tv_sec * MILLISECONDS_IN_SECOND;
    result += t->tv_nsec / NANOSECONDS_IN_MILLISECOND;
    return result;
}
