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
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/statfs.h>
#include <sys/types.h>

#include "attributes.h"
#include "cgroup-util.h"
#include "def.h"
#include "fd-util.h"
#include "fileio.h"
#include "macro.h"
#include "missing.h"
#include "parse-util.h"
#include "process-util.h"
#include "string-util.h"

#define F_TYPE_EQUAL(a, b) (a == (typeof(a)) b)

int cg_pid_get_path(const char *controller, pid_t pid, char **path) {
        _cleanup_fclose_ FILE *f = NULL;
        char line[LINE_MAX];
        const char *fs;
        size_t cs = 0;
        int unified;

        assert(path);
        assert(pid >= 0);

        unified = cg_unified();
        if (unified < 0)
                return unified;
        if (unified == 0) {
                if (controller) {
                        if (!cg_controller_is_valid(controller))
                                return -EINVAL;
                } else
                        controller = SYSTEMD_CGROUP_CONTROLLER;

                cs = strlen(controller);
        }

        fs = procfs_file_alloca(pid, "cgroup");
        f = fopen(fs, "re");
        if (!f)
                return errno == ENOENT ? -ESRCH : -errno;

        FOREACH_LINE(line, f, return -errno) {
                char *e, *p;

                truncate_nl(line);

                if (unified) {
                        e = startswith(line, "0:");
                        if (!e)
                                continue;

                        e = strchr(e, ':');
                        if (!e)
                                continue;
                } else {
                        char *l;
                        size_t k;
                        const char *word, *state;
                        bool found = false;

                        l = strchr(line, ':');
                        if (!l)
                                continue;

                        l++;
                        e = strchr(l, ':');
                        if (!e)
                                continue;

                        *e = 0;
                        FOREACH_WORD_SEPARATOR(word, k, l, ",", state) {
                                if (k == cs && memcmp(word, controller, cs) == 0) {
                                        found = true;
                                        break;
                                }
                        }

                        if (!found)
                                continue;
                }

                p = strdup(e + 1);
                if (!p)
                        return -ENOMEM;

                *path = p;
                return 0;
        }

        return -ENODATA;
}

#define CONTROLLER_VALID                        \
        DIGITS LETTERS                          \
        "_"

bool cg_controller_is_valid(const char *p) {
        const char *t, *s;

        if (!p)
                return false;

        s = startswith(p, "name=");
        if (s)
                p = s;

        if (*p == 0 || *p == '_')
                return false;

        for (t = p; *t; t++)
                if (!strchr(CONTROLLER_VALID, *t))
                        return false;

        if (t - p > FILENAME_MAX)
                return false;

        return true;
}

static thread_local int unified_cache = -1;

int cg_unified(void) {
        struct statfs fs;

        /* Checks if we support the unified hierarchy. Returns an
         * error when the cgroup hierarchies aren't mounted yet or we
         * have any other trouble determining if the unified hierarchy
         * is supported. */

        if (unified_cache >= 0)
                return unified_cache;

        if (statfs("/sys/fs/cgroup/", &fs) < 0)
                return -errno;

        if (F_TYPE_EQUAL(fs.f_type, CGROUP_SUPER_MAGIC))
                unified_cache = true;
        else if (F_TYPE_EQUAL(fs.f_type, TMPFS_MAGIC))
                unified_cache = false;
        else
                return -ENOMEDIUM;

        return unified_cache;
}

int cg_blkio_weight_parse(const char *s, uint64_t *ret) {
        uint64_t u;
        int r;

        if (isempty(s)) {
                *ret = CGROUP_BLKIO_WEIGHT_INVALID;
                return 0;
        }

        r = safe_atou64(s, &u);
        if (r < 0)
                return r;

        if (u < CGROUP_BLKIO_WEIGHT_MIN || u > CGROUP_BLKIO_WEIGHT_MAX)
                return -ERANGE;

        *ret = u;
        return 0;
}
