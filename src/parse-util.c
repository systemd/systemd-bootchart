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

#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include "macro.h"
#include "parse-util.h"
#include "string-util.h"

int parse_boolean(const char *v) {
        assert(v);

        if (streq(v, "1") || strcaseeq(v, "yes") || strcaseeq(v, "y") || strcaseeq(v, "true") || strcaseeq(v, "t") || strcaseeq(v, "on"))
                return 1;
        else if (streq(v, "0") || strcaseeq(v, "no") || strcaseeq(v, "n") || strcaseeq(v, "false") || strcaseeq(v, "f") || strcaseeq(v, "off"))
                return 0;

        return -EINVAL;
}

int safe_atou(const char *s, unsigned *ret_u) {
        char *x = NULL;
        unsigned long l;

        assert(s);
        assert(ret_u);

        /* strtoul() is happy to parse negative values, and silently
         * converts them to unsigned values without generating an
         * error. We want a clean error, hence let's look for the "-"
         * prefix on our own, and generate an error. But let's do so
         * only after strtoul() validated that the string is clean
         * otherwise, so that we return EINVAL preferably over
         * ERANGE. */

        s += strspn(s, WHITESPACE);

        errno = 0;
        l = strtoul(s, &x, 0);
        if (errno > 0)
                return -errno;
        if (!x || x == s || *x)
                return -EINVAL;
        if (s[0] == '-')
                return -ERANGE;
        if ((unsigned long) (unsigned) l != l)
                return -ERANGE;

        *ret_u = (unsigned) l;
        return 0;
}

int safe_atoi(const char *s, int *ret_i) {
        char *x = NULL;
        long l;

        assert(s);
        assert(ret_i);

        errno = 0;
        l = strtol(s, &x, 0);
        if (errno > 0)
                return -errno;
        if (!x || x == s || *x)
                return -EINVAL;
        if ((long) (int) l != l)
                return -ERANGE;

        *ret_i = (int) l;
        return 0;
}

int safe_atollu(const char *s, long long unsigned *ret_llu) {
        char *x = NULL;
        unsigned long long l;

        assert(s);
        assert(ret_llu);

        s += strspn(s, WHITESPACE);

        errno = 0;
        l = strtoull(s, &x, 0);
        if (errno > 0)
                return -errno;
        if (!x || x == s || *x)
                return -EINVAL;
        if (*s == '-')
                return -ERANGE;

        *ret_llu = l;
        return 0;
}

int safe_atolli(const char *s, long long int *ret_lli) {
        char *x = NULL;
        long long l;

        assert(s);
        assert(ret_lli);

        errno = 0;
        l = strtoll(s, &x, 0);
        if (errno > 0)
                return -errno;
        if (!x || x == s || *x)
                return -EINVAL;

        *ret_lli = l;
        return 0;
}

int safe_atou8(const char *s, uint8_t *ret) {
        char *x = NULL;
        unsigned long l;

        assert(s);
        assert(ret);

        s += strspn(s, WHITESPACE);

        errno = 0;
        l = strtoul(s, &x, 0);
        if (errno > 0)
                return -errno;
        if (!x || x == s || *x)
                return -EINVAL;
        if (s[0] == '-')
                return -ERANGE;
        if ((unsigned long) (uint8_t) l != l)
                return -ERANGE;

        *ret = (uint8_t) l;
        return 0;
}

int safe_atou16(const char *s, uint16_t *ret) {
        char *x = NULL;
        unsigned long l;

        assert(s);
        assert(ret);

        s += strspn(s, WHITESPACE);

        errno = 0;
        l = strtoul(s, &x, 0);
        if (errno > 0)
                return -errno;
        if (!x || x == s || *x)
                return -EINVAL;
        if (s[0] == '-')
                return -ERANGE;
        if ((unsigned long) (uint16_t) l != l)
                return -ERANGE;

        *ret = (uint16_t) l;
        return 0;
}

int safe_atoi16(const char *s, int16_t *ret) {
        char *x = NULL;
        long l;

        assert(s);
        assert(ret);

        errno = 0;
        l = strtol(s, &x, 0);
        if (errno > 0)
                return -errno;
        if (!x || x == s || *x)
                return -EINVAL;
        if ((long) (int16_t) l != l)
                return -ERANGE;

        *ret = (int16_t) l;
        return 0;
}

int safe_atod(const char *s, double *ret_d) {
        char *x = NULL;
        double d = 0;
        locale_t loc;

        assert(s);
        assert(ret_d);

        loc = newlocale(LC_NUMERIC_MASK, "C", (locale_t) 0);
        if (loc == (locale_t) 0)
                return -errno;

        errno = 0;
        d = strtod_l(s, &x, loc);
        if (errno > 0) {
                freelocale(loc);
                return -errno;
        }
        if (!x || x == s || *x) {
                freelocale(loc);
                return -EINVAL;
        }

        freelocale(loc);
        *ret_d = (double) d;
        return 0;
}
