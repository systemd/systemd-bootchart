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

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "alloc-util.h"
#include "fileio.h"
#include "macro.h"
#include "parse-util.h"
#include "process-util.h"
#include "special.h"
#include "string-util.h"
#include "util.h"
#include "virt.h"

static const char * const rlmap[] = {
        "emergency", SPECIAL_EMERGENCY_TARGET,
        "-b",        SPECIAL_EMERGENCY_TARGET,
        "rescue",    SPECIAL_RESCUE_TARGET,
        "single",    SPECIAL_RESCUE_TARGET,
        "-s",        SPECIAL_RESCUE_TARGET,
        "s",         SPECIAL_RESCUE_TARGET,
        "S",         SPECIAL_RESCUE_TARGET,
        "1",         SPECIAL_RESCUE_TARGET,
        "2",         SPECIAL_MULTI_USER_TARGET,
        "3",         SPECIAL_MULTI_USER_TARGET,
        "4",         SPECIAL_MULTI_USER_TARGET,
        "5",         SPECIAL_GRAPHICAL_TARGET,
};

const char* runlevel_to_target(const char *word) {
        size_t i;

        if (!word)
                return NULL;

        for (i = 0; i < ELEMENTSOF(rlmap); i += 2)
                if (streq(word, rlmap[i]))
                        return rlmap[i+1];

        return NULL;
}
