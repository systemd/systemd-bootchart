#pragma once

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

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

#include "list.h"

#define MAXCPUS        16
#define MAXPIDS     65535

struct block_stat_struct {
        /* /proc/vmstat pgpgin & pgpgout */
        int bi;
        int bo;
};

/* per process, per sample data we will log */
struct ps_sched_struct {
        /* /proc/<n>/schedstat fields 1 & 2 */
        double runtime;
        double waittime;
        int pss;
        struct list_sample_data *sampledata;
        struct ps_sched_struct *next;
        struct ps_sched_struct *prev;
        struct ps_sched_struct *cross; /* cross pointer */
        struct ps_struct *ps_new;
};

struct list_sample_data {
        double runtime[MAXCPUS];
        double waittime[MAXCPUS];
        double sampletime;
        int entropy_avail;
        struct block_stat_struct blockstat;
        LIST_FIELDS(struct list_sample_data, link); /* DLL */
        int counter;
};

/* process info */
struct ps_struct {
        struct ps_struct *next_ps;    /* SLL pointer */
        struct ps_struct *parent;     /* ppid ref */
        struct ps_struct *children;   /* children */
        struct ps_struct *next;       /* siblings */

        /* must match - otherwise it's a new process with same PID */
        char name[256];
        int pid;
        int ppid;
        char *cgroup;

        /* cache fd's */
        int sched;
        int schedstat;
        FILE *smaps;

        /* pointers to first/last seen timestamps */
        struct ps_sched_struct *first;
        struct ps_sched_struct *last;

        /* records actual start time, may be way before bootchart runs */
        double starttime;

        /* record human readable total cpu time */
        double total;

        /* largest PSS size found */
        int pss_max;

        /* for drawing connection lines later */
        double pos_x;
        double pos_y;

        struct ps_sched_struct *sample;
};

extern bool arg_relative;
extern bool arg_filter;
extern bool arg_show_cmdline;
extern bool arg_show_cgroup;
extern bool arg_pss;
extern bool arg_entropy;
extern bool arg_percpu;
extern bool arg_initcall;
extern int  arg_samples_len;
extern double arg_hz;
extern double arg_scale_x;
extern double arg_scale_y;

extern char arg_output_path[PATH_MAX];
extern char arg_init_path[PATH_MAX];
