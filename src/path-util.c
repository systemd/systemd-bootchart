/***
  This file is part of systemd.

  Copyright 2010-2012 Lennart Poettering

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
#include <stdlib.h>
#include <string.h>

#include "alloc-util.h"
#include "macro.h"
#include "path-util.h"
#include "string-util.h"
#include "strv.h"

bool path_is_absolute(const char *p) {
        return p[0] == '/';
}

char **path_strv_resolve(char **l, const char *prefix) {
        char **s;
        unsigned k = 0;
        bool enomem = false;

        if (strv_isempty(l))
                return l;

        /* Goes through every item in the string list and canonicalize
         * the path. This works in place and won't rollback any
         * changes on failure. */

        STRV_FOREACH(s, l) {
                char *t, *u;
                _cleanup_free_ char *orig = NULL;

                if (!path_is_absolute(*s)) {
                        free(*s);
                        continue;
                }

                if (prefix) {
                        orig = *s;
                        t = strappend(prefix, orig);
                        if (!t) {
                                enomem = true;
                                continue;
                        }
                } else
                        t = *s;

                errno = 0;
                u = canonicalize_file_name(t);
                if (!u) {
                        if (errno == ENOENT) {
                                if (prefix) {
                                        u = orig;
                                        orig = NULL;
                                        free(t);
                                } else
                                        u = t;
                        } else {
                                free(t);
                                if (errno == ENOMEM || errno == 0)
                                        enomem = true;

                                continue;
                        }
                } else if (prefix) {
                        char *x;

                        free(t);
                        x = path_startswith(u, prefix);
                        if (x) {
                                /* restore the slash if it was lost */
                                if (!startswith(x, "/"))
                                        *(--x) = '/';

                                t = strdup(x);
                                free(u);
                                if (!t) {
                                        enomem = true;
                                        continue;
                                }
                                u = t;
                        } else {
                                /* canonicalized path goes outside of
                                 * prefix, keep the original path instead */
                                free(u);
                                u = orig;
                                orig = NULL;
                        }
                } else
                        free(t);

                l[k++] = u;
        }

        l[k] = NULL;

        if (enomem)
                return NULL;

        return l;
}

char **path_strv_resolve_uniq(char **l, const char *prefix) {

        if (strv_isempty(l))
                return l;

        if (!path_strv_resolve(l, prefix))
                return NULL;

        return strv_uniq(l);
}

char *path_kill_slashes(char *path) {
        char *f, *t;
        bool slash = false;

        /* Removes redundant inner and trailing slashes. Modifies the
         * passed string in-place.
         *
         * ///foo///bar/ becomes /foo/bar
         */

        for (f = path, t = path; *f; f++) {

                if (*f == '/') {
                        slash = true;
                        continue;
                }

                if (slash) {
                        slash = false;
                        *(t++) = '/';
                }

                *(t++) = *f;
        }

        /* Special rule, if we are talking of the root directory, a
        trailing slash is good */

        if (t == path && slash)
                *(t++) = '/';

        *t = 0;
        return path;
}

char* path_startswith(const char *path, const char *prefix) {
        assert(path);
        assert(prefix);

        if ((path[0] == '/') != (prefix[0] == '/'))
                return NULL;

        for (;;) {
                size_t a, b;

                path += strspn(path, "/");
                prefix += strspn(prefix, "/");

                if (*prefix == 0)
                        return (char*) path;

                if (*path == 0)
                        return NULL;

                a = strcspn(path, "/");
                b = strcspn(prefix, "/");

                if (a != b)
                        return NULL;

                if (memcmp(path, prefix, a) != 0)
                        return NULL;

                path += a;
                prefix += b;
        }
}

int path_compare(const char *a, const char *b) {
        int d;

        assert(a);
        assert(b);

        /* A relative path and an abolute path must not compare as equal.
         * Which one is sorted before the other does not really matter.
         * Here a relative path is ordered before an absolute path. */
        d = (a[0] == '/') - (b[0] == '/');
        if (d != 0)
                return d;

        for (;;) {
                size_t j, k;

                a += strspn(a, "/");
                b += strspn(b, "/");

                if (*a == 0 && *b == 0)
                        return 0;

                /* Order prefixes first: "/foo" before "/foo/bar" */
                if (*a == 0)
                        return -1;
                if (*b == 0)
                        return 1;

                j = strcspn(a, "/");
                k = strcspn(b, "/");

                /* Alphabetical sort: "/foo/aaa" before "/foo/b" */
                d = memcmp(a, b, MIN(j, k));
                if (d != 0)
                        return (d > 0) - (d < 0); /* sign of d */

                /* Sort "/foo/a" before "/foo/aaa" */
                d = (j > k) - (j < k);  /* sign of (j - k) */
                if (d != 0)
                        return d;

                a += j;
                b += k;
        }
}

bool path_equal(const char *a, const char *b) {
        return path_compare(a, b) == 0;
}

char *file_in_same_dir(const char *path, const char *filename) {
        char *e, *ret;
        size_t k;

        assert(path);
        assert(filename);

        /* This removes the last component of path and appends
         * filename, unless the latter is absolute anyway or the
         * former isn't */

        if (path_is_absolute(filename))
                return strdup(filename);

        e = strrchr(path, '/');
        if (!e)
                return strdup(filename);

        k = strlen(filename);
        ret = new(char, (e + 1 - path) + k + 1);
        if (!ret)
                return NULL;

        memcpy(mempcpy(ret, path, e + 1 - path), filename, k + 1);
        return ret;
}

bool hidden_file_allow_backup(const char *filename) {
        assert(filename);

        return
                filename[0] == '.' ||
                streq(filename, "lost+found") ||
                streq(filename, "aquota.user") ||
                streq(filename, "aquota.group") ||
                endswith(filename, ".rpmnew") ||
                endswith(filename, ".rpmsave") ||
                endswith(filename, ".rpmorig") ||
                endswith(filename, ".dpkg-old") ||
                endswith(filename, ".dpkg-new") ||
                endswith(filename, ".dpkg-tmp") ||
                endswith(filename, ".dpkg-dist") ||
                endswith(filename, ".dpkg-bak") ||
                endswith(filename, ".dpkg-backup") ||
                endswith(filename, ".dpkg-remove") ||
                endswith(filename, ".swp");
}

bool hidden_file(const char *filename) {
        assert(filename);

        if (endswith(filename, "~"))
                return true;

        return hidden_file_allow_backup(filename);
}
