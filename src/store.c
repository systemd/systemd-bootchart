/***
  This file is part of systemd.

  Copyright (C) 2009-2013 Intel Corporation

  Authors:
    Auke Kok <auke-jan.h.kok@intel.com>

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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "alloc-util.h"
#include "bootchart.h"
#include "cgroup-util.h"
#include "def.h"
#include "dirent-util.h"
#include "fd-util.h"
#include "fileio.h"
#include "formats-util.h"
#include "log.h"
#include "parse-util.h"
#include "store.h"
#include "string-util.h"
#include "strxcpyx.h"
#include "time-util.h"

/*
 * Alloc a static 4k buffer for stdio - primarily used to increase
 * PSS buffering from the default 1k stdin buffer to reduce
 * read() overhead.
 */
static char smaps_buf[4096];
static int skip = 0;

double gettime_ns(void) {
        struct timespec n;

        clock_gettime(clock_boottime_or_monotonic(), &n);

        return (n.tv_sec + (n.tv_nsec / (double) NSEC_PER_SEC));
}

static char *bufgetline(char *buf) {
        char *c;

        if (!buf)
                return NULL;

        c = strchr(buf, '\n');
        if (c)
                c++;

        return c;
}

static int pid_cmdline_strscpy(int procfd, char *buffer, size_t buf_len, int pid) {
        char filename[PATH_MAX];
        _cleanup_close_ int fd = -1;
        ssize_t n;

        sprintf(filename, "%d/cmdline", pid);
        fd = openat(procfd, filename, O_RDONLY|O_CLOEXEC);
        if (fd < 0)
                return -errno;

        n = read(fd, buffer, buf_len-1);
        if (n > 0) {
                int i;
                for (i = 0; i < n; i++)
                        if (buffer[i] == '\0')
                                buffer[i] = ' ';
                buffer[n] = '\0';
        }

        return 0;
}

static void garbage_collect_dead_processes(struct ps_struct *ps_first) {
        struct ps_struct *ps;
        struct ps_struct *ps_next;

        ps = ps_first;
        while ((ps_next = ps->next_running)) {
                if (!ps_next->still_running) {
                        /* close the stream and fds */
                        ps_next->schedstat = safe_close(ps_next->schedstat);
                        ps_next->sched = safe_close(ps_next->sched);
                        if (ps_next->smaps) {
                                fclose(ps_next->smaps);
                                ps_next->smaps = NULL;
                        }

                        ps->next_running = ps_next->next_running;
                } else {
                        ps = ps_next;
                }
                /* this resets the flag for both running and dead processes*/
                ps_next->still_running = false;
        }
}


int log_sample(DIR *proc,
               int sample,
               struct ps_struct *ps_first,
               struct list_sample_data **ptr,
               int *pscount,
               int *cpus) {

        static int vmstat = -1;
        _cleanup_free_ char *buf_schedstat = NULL;
        char buf[4096];
        char key[256];
        char val[256];
        char rt[256];
        char wt[256];
        char *m;
        int r;
        int c;
        int p;
        int mod;
        static int e_fd = -1;
        ssize_t s;
        ssize_t n;
        struct dirent *ent;
        int fd;
        struct list_sample_data *sampledata;
        struct ps_sched_struct *ps_prev = NULL;
        int procfd;
        int taskfd = -1;

        sampledata = *ptr;

        procfd = dirfd(proc);
        if (procfd < 0)
                return -errno;

        if (vmstat < 0) {
                /* block stuff */
                vmstat = openat(procfd, "vmstat", O_RDONLY|O_CLOEXEC);
                if (vmstat < 0)
                        return log_error_errno(errno, "Failed to open /proc/vmstat: %m");
        }

        n = pread(vmstat, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
                vmstat = safe_close(vmstat);
                if (n < 0)
                        return -errno;
                return -ENODATA;
        }

        buf[n] = '\0';

        m = buf;
        while (m) {
                if (sscanf(m, "%s %s", key, val) < 2)
                        goto vmstat_next;
                if (streq(key, "pgpgin"))
                        sampledata->blockstat.bi = atoi(val);
                if (streq(key, "pgpgout")) {
                        sampledata->blockstat.bo = atoi(val);
                        break;
                }
vmstat_next:
                m = bufgetline(m);
                if (!m)
                        break;
        }

        /* Parse "/proc/schedstat" for overall CPU utilization */
        r = read_full_file("/proc/schedstat", &buf_schedstat, NULL);
        if (r < 0)
            return log_error_errno(r, "Unable to read schedstat: %m");

        m = buf_schedstat;
        while (m) {
                if (sscanf(m, "%s %*s %*s %*s %*s %*s %*s %s %s", key, rt, wt) < 3)
                        goto schedstat_next;

                if (strstr(key, "cpu")) {
                        r = safe_atoi((const char*)(key+3), &c);
                        if (r < 0 || c > MAXCPUS -1)
                                /* Oops, we only have room for MAXCPUS data */
                                break;
                        sampledata->runtime[c] = atoll(rt);
                        sampledata->waittime[c] = atoll(wt);

                        if (c == *cpus)
                                *cpus = c + 1;
                }
schedstat_next:
                m = bufgetline(m);
                if (!m)
                        break;
        }

        if (arg_entropy) {
                if (e_fd < 0) {
                        e_fd = openat(procfd, "sys/kernel/random/entropy_avail", O_RDONLY|O_CLOEXEC);
                        if (e_fd < 0)
                                return log_error_errno(errno, "Failed to open /proc/sys/kernel/random/entropy_avail: %m");
                }

                n = pread(e_fd, buf, sizeof(buf) - 1, 0);
                if (n <= 0) {
                        e_fd = safe_close(e_fd);
                } else {
                        buf[n] = '\0';
                        sampledata->entropy_avail = atoi(buf);
                }
        }

        while ((ent = readdir(proc)) != NULL) {
                char filename[PATH_MAX];
                int pid;
                struct ps_struct *ps;

                if ((ent->d_name[0] < '0') || (ent->d_name[0] > '9'))
                        continue;

                pid = atoi(ent->d_name);

                if (pid >= MAXPIDS)
                        continue;

                ps = ps_first;
                while (ps->next_running) {
                        ps = ps->next_running;
                        if (ps->pid == pid)
                                break;
                }

                /* end of our LL? then append a new record */
                if (ps->pid != pid) {
                        _cleanup_fclose_ FILE *st = NULL;
                        char t[32];
                        struct ps_struct *parent;

                        ps->next_ps = ps->next_running = new0(struct ps_struct, 1);
                        if (!ps->next_ps)
                                return log_oom();

                        ps = ps->next_ps;
                        ps->pid = pid;
                        ps->sched = -1;
                        ps->schedstat = -1;

                        ps->sample = new0(struct ps_sched_struct, 1);
                        if (!ps->sample)
                                return log_oom();

                        ps->sample->sampledata = sampledata;

                        (*pscount)++;

                        /* mark our first sample */
                        ps->first = ps->last = ps->sample;

                        /* get name, start time; requires CONFIG_SCHED_DEBUG in kernel */
                        if (ps->sched < 0) {
                                sprintf(filename, "%d/sched", pid);
                                ps->sched = openat(procfd, filename, O_RDONLY|O_CLOEXEC);
                                if (ps->sched < 0)
                                        goto no_sched;
                        }

                        s = pread(ps->sched, buf, sizeof(buf) - 1, 0);
                        if (s <= 0) {
                                ps->sched = safe_close(ps->sched);
                                goto no_sched;
                        }
                        buf[s] = '\0';

                        if (!sscanf(buf, "%s %*s %*s", key))
                                goto no_sched;

                        strscpy(ps->name, sizeof(ps->name), key);

                        /* discard line 2 */
                        m = bufgetline(buf);
                        if (!m)
                                goto no_sched;

                        m = bufgetline(m);
                        if (!m)
                                goto no_sched;

                        if (!sscanf(m, "%*s %*s %s", t))
                                goto no_sched;

                        r = safe_atod(t, &ps->starttime);
                        if (r < 0)
                                goto no_sched;

                        ps->starttime /= 1000.0;

no_sched:
                        /* cmdline */
                        if (arg_show_cmdline)
                                pid_cmdline_strscpy(procfd, ps->name, sizeof(ps->name), pid);

                        if (arg_show_cgroup)
                                /* if this fails, that's OK */
                                cg_pid_get_path(SYSTEMD_CGROUP_CONTROLLER,
                                                ps->pid, &ps->cgroup);

                        /* ppid */
                        sprintf(filename, "%d/stat", pid);
                        fd = openat(procfd, filename, O_RDONLY|O_CLOEXEC);
                        if (fd < 0)
                                continue;

                        st = fdopen(fd, "re");
                        if (!st) {
                                close(fd);
                                continue;
                        }

                        if (!fscanf(st, "%*s %*s %*s %i", &p))
                                continue;

                        ps->ppid = p;

                        /*
                         * setup child pointers
                         *
                         * these are used to paint the tree coherently later
                         * each parent has a LL of children, and a LL of siblings
                         */
                        if (pid == 1)
                                continue; /* nothing to do for init atm */

                        /* kthreadd has ppid=0, which breaks our tree ordering */
                        if (ps->ppid == 0)
                                ps->ppid = 1;

                        parent = ps_first;
                        while ((parent->next_ps && parent->pid != ps->ppid))
                                parent = parent->next_ps;

                        if (parent->pid != ps->ppid) {
                                /* orphan */
                                ps->ppid = 1;
                                parent = ps_first->next_ps;
                        }

                        ps->parent = parent;

                        /*
                         * append ourselves to the list of children
                         * TODO: consider if prepending is OK for efficiency here.
                         */
                        {
                                struct ps_struct **children = &parent->children;
                                while (*children)
                                        children = &(*children)->next;
                                *children = ps;
                        }
                }

                /* else -> found pid, append data in ps */

                /* below here is all continuous logging parts - we get here on every
                 * iteration */

                /* rt, wt */
                if (ps->schedstat < 0) {
                        sprintf(filename, "%d/schedstat", pid);
                        ps->schedstat = openat(procfd, filename, O_RDONLY|O_CLOEXEC);
                        if (ps->schedstat < 0)
                                continue;
                }

                s = pread(ps->schedstat, buf, sizeof(buf) - 1, 0);
                if (s <= 0)
                        continue;

                buf[s] = '\0';

                if (!sscanf(buf, "%s %s %*s", rt, wt))
                        continue;

                ps->sample->next = new0(struct ps_sched_struct, 1);
                if (!ps->sample->next)
                        return log_oom();

                ps->sample->next->prev = ps->sample;
                ps->sample = ps->sample->next;
                ps->last = ps->sample;
                ps->sample->runtime = atoll(rt);
                ps->sample->waittime = atoll(wt);
                ps->sample->sampledata = sampledata;
                ps->sample->ps_new = ps;
                if (ps_prev)
                        ps_prev->cross = ps->sample;

                ps_prev = ps->sample;
                ps->total = (ps->last->runtime - ps->first->runtime)
                            / 1000000000.0;

                /* Take into account CPU runtime/waittime spent in non-main threads of the process
                 * by parsing "/proc/[pid]/task/[tid]/schedstat" for all [tid] != [pid]
                 * See https://github.com/systemd/systemd/issues/139
                 */

                /* Browse directory "/proc/[pid]/task" to know the thread ids of process [pid] */
                snprintf(filename, sizeof(filename), PID_FMT "/task", pid);
                taskfd = openat(procfd, filename, O_RDONLY|O_DIRECTORY|O_CLOEXEC);
                if (taskfd >= 0) {
                        _cleanup_closedir_ DIR *taskdir = NULL;

                        taskdir = fdopendir(taskfd);
                        if (!taskdir) {
                                safe_close(taskfd);
                                return -errno;
                        }
                        FOREACH_DIRENT(ent, taskdir, break) {
                                int tid = -1;
                                _cleanup_close_ int tid_schedstat = -1;
                                long long delta_rt;
                                long long delta_wt;

                                if ((ent->d_name[0] < '0') || (ent->d_name[0] > '9'))
                                        continue;

                                /* Skip main thread as it was already accounted */
                                r = safe_atoi(ent->d_name, &tid);
                                if (r < 0 || tid == pid)
                                        continue;

                                /* Parse "/proc/[pid]/task/[tid]/schedstat" */
                                snprintf(filename, sizeof(filename), PID_FMT "/schedstat", tid);
                                tid_schedstat = openat(taskfd, filename, O_RDONLY|O_CLOEXEC);

                                if (tid_schedstat == -1)
                                        continue;

                                s = pread(tid_schedstat, buf, sizeof(buf) - 1, 0);
                                if (s <= 0)
                                        continue;
                                buf[s] = '\0';

                                if (!sscanf(buf, "%s %s %*s", rt, wt))
                                        continue;

                                r = safe_atolli(rt, &delta_rt);
                                if (r < 0)
                                    continue;
                                r = safe_atolli(rt, &delta_wt);
                                if (r < 0)
                                    continue;
                                ps->sample->runtime  += delta_rt;
                                ps->sample->waittime += delta_wt;
                        }
                }

                if (!arg_pss)
                        goto catch_rename;

                /* Pss */
                if (!ps->smaps) {
                        sprintf(filename, "%d/smaps", pid);
                        fd = openat(procfd, filename, O_RDONLY|O_CLOEXEC);
                        if (fd < 0)
                                continue;
                        ps->smaps = fdopen(fd, "re");
                        if (!ps->smaps) {
                                close(fd);
                                continue;
                        }
                        setvbuf(ps->smaps, smaps_buf, _IOFBF, sizeof(smaps_buf));
                } else {
                        rewind(ps->smaps);
                }

                /* test to see if we need to skip another field */
                if (skip == 0) {
                        if (fgets(buf, sizeof(buf), ps->smaps) == NULL) {
                                continue;
                        }
                        if (fread(buf, 1, 28 * 15, ps->smaps) != (28 * 15)) {
                                continue;
                        }
                        if (buf[392] == 'V') {
                                skip = 2;
                        }
                        else {
                                skip = 1;
                        }
                        rewind(ps->smaps);
                }

                while (1) {
                        int pss_kb;

                        /* skip one line, this contains the object mapped. */
                        if (fgets(buf, sizeof(buf), ps->smaps) == NULL) {
                                break;
                        }
                        /* then there's a 28 char 14 line block */
                        if (fread(buf, 1, 28 * 14, ps->smaps) != 28 * 14) {
                                break;
                        }
                        pss_kb = atoi(&buf[61]);
                        ps->sample->pss += pss_kb;

                        /* skip one more line if this is a newer kernel */
                        if (skip == 2) {
                               if (fgets(buf, sizeof(buf), ps->smaps) == NULL)
                                       break;
                        }
                }

                if (ps->sample->pss > ps->pss_max)
                        ps->pss_max = ps->sample->pss;

catch_rename:
                /* catch process rename, try to randomize time */
                mod = (arg_hz < 4.0) ? 4.0 : (arg_hz / 4.0);
                if (((sample - ps->pid) + pid) % (int)(mod) == 0) {

                        /* re-fetch name */
                        /* get name, start time */
                        if (ps->sched < 0) {
                                sprintf(filename, "%d/sched", pid);
                                ps->sched = openat(procfd, filename, O_RDONLY|O_CLOEXEC);
                                if (ps->sched < 0)
                                        goto no_sched2;
                        }

                        s = pread(ps->sched, buf, sizeof(buf) - 1, 0);
                        if (s <= 0)
                                continue;

                        buf[s] = '\0';

                        if (!sscanf(buf, "%s %*s %*s", key))
                                continue;

                        strscpy(ps->name, sizeof(ps->name), key);

no_sched2:
                        /* cmdline */
                        if (arg_show_cmdline)
                                pid_cmdline_strscpy(procfd, ps->name, sizeof(ps->name), pid);
                }
                ps->still_running = true;
        }

        garbage_collect_dead_processes(ps_first);

        return 0;
}
