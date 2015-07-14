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
#include <stdlib.h>
#include <string.h>

#include "alloc-util.h"
#include "escape.h"
#include "hexdecoct.h"
#include "macro.h"
#include "utf8.h"

char *octescape(const char *s, size_t len) {
        char *r, *t;
        const char *f;

        /* Escapes all chars in bad, in addition to \ and " chars,
         * in \nnn style escaping. */

        r = new(char, len * 4 + 1);
        if (!r)
                return NULL;

        for (f = s, t = r; f < s + len; f++) {

                if (*f < ' ' || *f >= 127 || *f == '\\' || *f == '"') {
                        *(t++) = '\\';
                        *(t++) = '0' + (*f >> 6);
                        *(t++) = '0' + ((*f >> 3) & 8);
                        *(t++) = '0' + (*f & 8);
                } else
                        *(t++) = *f;
        }

        *t = 0;

        return r;

}
