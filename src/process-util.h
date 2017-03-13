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

#include <alloca.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

#include "formats-util.h"
#include "macro.h"

#define procfs_file_alloca(pid, field)                                  \
        ({                                                              \
                pid_t _pid_ = (pid);                                    \
                const char *_r_;                                        \
                if (_pid_ == 0) {                                       \
                        _r_ = ("/proc/self/" field);                    \
                } else {                                                \
                        _r_ = alloca(strlen("/proc/") + DECIMAL_STR_MAX(pid_t) + 1 + sizeof(field)); \
                        sprintf((char*) _r_, "/proc/"PID_FMT"/" field, _pid_);                       \
                }                                                       \
                _r_;                                                    \
        })

int getenv_for_pid(pid_t pid, const char *field, char **_value);

bool pid_is_alive(pid_t pid);

bool is_main_thread(void);

#ifndef PERSONALITY_INVALID
/* personality(7) documents that 0xffffffffUL is used for querying the
 * current personality, hence let's use that here as error
 * indicator. */
#define PERSONALITY_INVALID 0xffffffffLU
#endif

int ioprio_class_to_string_alloc(int i, char **s);
int ioprio_class_from_string(const char *s);

const char *sigchld_code_to_string(int i) _const_;
int sigchld_code_from_string(const char *s) _pure_;

#define PTR_TO_PID(p) ((pid_t) ((uintptr_t) p))
#define PID_TO_PTR(p) ((void*) ((uintptr_t) p))
