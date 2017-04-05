#pragma once

/***
  This file is part of systemd.

  Copyright 2010 Lennart Poettering

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "attributes.h"

typedef uint64_t usec_t;
typedef uint64_t nsec_t;

#define NSEC_FMT "%" PRIu64
#define USEC_FMT "%" PRIu64

#include "macro.h"

typedef struct dual_timestamp {
        usec_t realtime;
        usec_t monotonic;
} dual_timestamp;

#define USEC_INFINITY ((usec_t) -1)
#define NSEC_INFINITY ((nsec_t) -1)

#define MSEC_PER_SEC  1000ULL
#define USEC_PER_SEC  ((usec_t) 1000000ULL)
#define USEC_PER_MSEC ((usec_t) 1000ULL)
#define NSEC_PER_SEC  ((nsec_t) 1000000000ULL)
#define NSEC_PER_MSEC ((nsec_t) 1000000ULL)
#define NSEC_PER_USEC ((nsec_t) 1000ULL)

#define USEC_PER_MINUTE ((usec_t) (60ULL*USEC_PER_SEC))
#define NSEC_PER_MINUTE ((nsec_t) (60ULL*NSEC_PER_SEC))
#define USEC_PER_HOUR ((usec_t) (60ULL*USEC_PER_MINUTE))
#define NSEC_PER_HOUR ((nsec_t) (60ULL*NSEC_PER_MINUTE))
#define USEC_PER_DAY ((usec_t) (24ULL*USEC_PER_HOUR))
#define NSEC_PER_DAY ((nsec_t) (24ULL*NSEC_PER_HOUR))
#define USEC_PER_WEEK ((usec_t) (7ULL*USEC_PER_DAY))
#define NSEC_PER_WEEK ((nsec_t) (7ULL*NSEC_PER_DAY))
#define USEC_PER_MONTH ((usec_t) (2629800ULL*USEC_PER_SEC))
#define NSEC_PER_MONTH ((nsec_t) (2629800ULL*NSEC_PER_SEC))
#define USEC_PER_YEAR ((usec_t) (31557600ULL*USEC_PER_SEC))
#define NSEC_PER_YEAR ((nsec_t) (31557600ULL*NSEC_PER_SEC))

#define TIME_T_MAX (time_t)((UINTMAX_C(1) << ((sizeof(time_t) << 3) - 1)) - 1)

#define DUAL_TIMESTAMP_NULL ((struct dual_timestamp) { 0ULL, 0ULL })

usec_t now(clockid_t clock);

static inline bool dual_timestamp_is_set(dual_timestamp *ts) {
        return ((ts->realtime > 0 && ts->realtime != USEC_INFINITY) ||
                (ts->monotonic > 0 && ts->monotonic != USEC_INFINITY));
}

usec_t timespec_load(const struct timespec *ts) _pure_;
struct timespec *timespec_store(struct timespec *ts, usec_t u);

clockid_t clock_boottime_or_monotonic(void);

#define xstrftime(buf, fmt, tm) \
        assert_message_se(strftime(buf, ELEMENTSOF(buf), fmt, tm) > 0, \
                          "xstrftime: " #buf "[] must be big enough")

static inline usec_t usec_add(usec_t a, usec_t b) {
        usec_t c;

        /* Adds two time values, and makes sure USEC_INFINITY as input results as USEC_INFINITY in output, and doesn't
         * overflow. */

        c = a + b;
        if (c < a || c < b) /* overflow check */
                return USEC_INFINITY;

        return c;
}

static inline usec_t usec_sub(usec_t timestamp, int64_t delta) {
        if (delta < 0)
                return usec_add(timestamp, (usec_t) (-delta));

        if (timestamp == USEC_INFINITY) /* Make sure infinity doesn't degrade */
                return USEC_INFINITY;

        if (timestamp < (usec_t) delta)
                return 0;

        return timestamp - delta;
}
