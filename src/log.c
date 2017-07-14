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
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

#include "sd-messages.h"

#include "fd-util.h"
#include "io-util.h"
#include "log.h"
#include "macro.h"
#include "stdio-util.h"
#include "string-util.h"
#include "terminal-util.h"
#include "util.h"

#define SNDBUF_SIZE (8*1024*1024)

static int log_max_level = LOG_INFO;

static int console_fd = STDERR_FILENO;

/* Akin to glibc's __abort_msg; which is private and we hence cannot
 * use here. */
static char *log_abort_msg = NULL;

static void log_close_console(void) {

        if (console_fd < 0)
                return;

        if (getpid() == 1) {
                if (console_fd >= 3)
                        safe_close(console_fd);

                console_fd = -1;
        }
}

static int log_open_console(void) {

        if (console_fd >= 0)
                return 0;

        if (getpid() == 1) {
                console_fd = open_terminal("/dev/console", O_WRONLY|O_NOCTTY|O_CLOEXEC);
                if (console_fd < 0)
                        return console_fd;
        } else
                console_fd = STDERR_FILENO;

        return 0;
}

static int write_to_console(
                int error,
                const char *file,
                int line,
                const char *func,
                const char *object_field,
                const char *object,
                const char *buffer) {

        struct iovec iovec[6] = {};
        unsigned n = 0;

        if (console_fd < 0)
                return 0;

        IOVEC_SET_STRING(iovec[n++], buffer);
        IOVEC_SET_STRING(iovec[n++], "\n");

        if (writev(console_fd, iovec, n) < 0) {

                if (errno == EIO && getpid() == 1) {

                        /* If somebody tried to kick us from our
                         * console tty (via vhangup() or suchlike),
                         * try to reconnect */

                        log_close_console();
                        log_open_console();

                        if (console_fd < 0)
                                return 0;

                        if (writev(console_fd, iovec, n) < 0)
                                return -errno;
                } else
                        return -errno;
        }

        return 1;
}

static int log_dispatch(
                int error,
                const char *file,
                int line,
                const char *func,
                const char *object_field,
                const char *object,
                char *buffer) {

        assert(buffer);

        if (error < 0)
                error = -error;

        do {
                char *e;
                int k = 0;

                buffer += strspn(buffer, NEWLINE);

                if (buffer[0] == 0)
                        break;

                if ((e = strpbrk(buffer, NEWLINE)))
                        *(e++) = 0;

                if (k <= 0)
                        (void) write_to_console(error, file, line, func, object_field, object, buffer);

                buffer = e;
        } while (buffer);

        return -error;
}

int log_internalv(
                int level,
                int error,
                const char *file,
                int line,
                const char *func,
                const char *format,
                va_list ap) {

        PROTECT_ERRNO;
        char buffer[LINE_MAX];

        if (error < 0)
                error = -error;

        if (_likely_(LOG_PRI(level) > log_max_level))
                return -error;

        /* Make sure that %m maps to the specified error */
        if (error != 0)
                errno = error;

        vsnprintf(buffer, sizeof(buffer), format, ap);

        return log_dispatch(error, file, line, func, NULL, NULL, buffer);
}

int log_internal(
                int level,
                int error,
                const char *file,
                int line,
                const char *func,
                const char *format, ...) {

        va_list ap;
        int r;

        va_start(ap, format);
        r = log_internalv(level, error, file, line, func, format, ap);
        va_end(ap);

        return r;
}

static void log_assert(
                int level,
                const char *text,
                const char *file,
                int line,
                const char *func,
                const char *format) {

        static char buffer[LINE_MAX];

        if (_likely_(LOG_PRI(level) > log_max_level))
                return;

        DISABLE_WARNING_FORMAT_NONLITERAL;
        xsprintf(buffer, format, text, file, line, func);
        REENABLE_WARNING;

        log_abort_msg = buffer;

        log_dispatch(0, file, line, func, NULL, NULL, buffer);
}

noreturn void log_assert_failed(const char *text, const char *file, int line, const char *func) {
        log_assert(LOG_CRIT, text, file, line, func, "Assertion '%s' failed at %s:%u, function %s(). Aborting.");
        abort();
}

noreturn void log_assert_failed_unreachable(const char *text, const char *file, int line, const char *func) {
        log_assert(LOG_CRIT, text, file, line, func, "Code should not be reached '%s' at %s:%u, function %s(). Aborting.");
        abort();
}

void log_assert_failed_return(const char *text, const char *file, int line, const char *func) {
        PROTECT_ERRNO;
        log_assert(LOG_DEBUG, text, file, line, func, "Assertion '%s' failed at %s:%u, function %s(). Ignoring.");
}

int log_oom_internal(const char *file, int line, const char *func) {
        log_internal(LOG_ERR, ENOMEM, file, line, func, "Out of memory.");
        return -ENOMEM;
}

int log_struct_internal(
                int level,
                int error,
                const char *file,
                int line,
                const char *func,
                const char *format, ...) {

        char buf[LINE_MAX];
        bool found = false;
        PROTECT_ERRNO;
        va_list ap;

        if (error < 0)
                error = -error;

        if (_likely_(LOG_PRI(level) > log_max_level))
                return -error;

        va_start(ap, format);
        while (format) {
                va_list aq;

                if (error != 0)
                        errno = error;

                va_copy(aq, ap);
                vsnprintf(buf, sizeof(buf), format, aq);
                va_end(aq);

                if (startswith(buf, "MESSAGE=")) {
                        found = true;
                        break;
                }

                VA_FORMAT_ADVANCE(format, ap);

                format = va_arg(ap, char *);
        }
        va_end(ap);

        if (!found)
                return -error;

        return log_dispatch(error, file, line, func, NULL, NULL, buf + 8);
}

int log_get_max_level(void) {
        return log_max_level;
}

int log_syntax_internal(
                const char *unit,
                int level,
                const char *config_file,
                unsigned config_line,
                int error,
                const char *file,
                int line,
                const char *func,
                const char *format, ...) {

        PROTECT_ERRNO;
        char buffer[LINE_MAX];
        int r;
        va_list ap;

        if (error < 0)
                error = -error;

        if (_likely_(LOG_PRI(level) > log_max_level))
                return -error;

        if (error != 0)
                errno = error;

        va_start(ap, format);
        vsnprintf(buffer, sizeof(buffer), format, ap);
        va_end(ap);

        r = log_struct_internal(
                        level, error,
                        file, line, func,
#ifdef HAVE_LIBSYSTEMD
                        LOG_MESSAGE_ID(SD_MESSAGE_INVALID_CONFIGURATION),
#endif
                        "CONFIG_FILE=%s", config_file,
                        "CONFIG_LINE=%u", config_line,
                        LOG_MESSAGE("[%s:%u] %s", config_file, config_line, buffer),
                        NULL);

        return r;
}
