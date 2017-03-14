/***
  This file is part of systemd.

  Copyright 2013 Kay Sievers

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

/*
 * Concatenates/copies strings. In any case, terminates in all cases
 * with '\0' * and moves the @dest pointer forward to the added '\0'.
 * Returns the * remaining size, and 0 if the string was truncated.
 */

#include <string.h>

#include "strxcpyx.h"

size_t strpcpy(char **dest, size_t size, const char *src) {
        size_t len;

        len = strlen(src);
        if (len >= size) {
                if (size > 1)
                        *dest = mempcpy(*dest, src, size-1);
                size = 0;
        } else {
                if (len > 0) {
                        *dest = mempcpy(*dest, src, len);
                        size -= len;
                }
        }
        *dest[0] = '\0';
        return size;
}

size_t strscpy(char *dest, size_t size, const char *src) {
        char *s;

        s = dest;
        return strpcpy(&s, size, src);
}
