#pragma once

#include <unistd.h>
#include <sys/syscall.h>

#ifndef CGROUP_SUPER_MAGIC
#define CGROUP_SUPER_MAGIC 0x27e0eb
#endif

#ifndef TMPFS_MAGIC
#define TMPFS_MAGIC 0x01021994
#endif

#if !HAVE_DECL_GETTID
static inline pid_t gettid(void) {
        return (pid_t) syscall(SYS_gettid);
}
#endif

#ifndef __NR_getrandom
#  if defined __x86_64__
#    define __NR_getrandom 318
#  elif defined(__i386__)
#    define __NR_getrandom 355
#  elif defined(__arm__)
#    define __NR_getrandom 384
# elif defined(__aarch64__)
#    define __NR_getrandom 278
# elif defined(__riscv)
#    define __NR_getrandom 278
#  elif defined(__ia64__)
#    define __NR_getrandom 1339
#  elif defined(__m68k__)
#    define __NR_getrandom 352
#  elif defined(__s390x__)
#    define __NR_getrandom 349
#  elif defined(__powerpc__)
#    define __NR_getrandom 359
#  elif defined _MIPS_SIM
#    if _MIPS_SIM == _MIPS_SIM_ABI32
#      define __NR_getrandom 4353
#    endif
#    if _MIPS_SIM == _MIPS_SIM_NABI32
#      define __NR_getrandom 6317
#    endif
#    if _MIPS_SIM == _MIPS_SIM_ABI64
#      define __NR_getrandom 5313
#    endif
#  else
#    warning "__NR_getrandom unknown for your architecture"
#    define __NR_getrandom 0xffffffff
#  endif
#endif

#if !HAVE_DECL_GETRANDOM
static inline int getrandom(void *buffer, size_t count, unsigned flags) {
        return syscall(__NR_getrandom, buffer, count, flags);
}
#endif

#ifndef GRND_NONBLOCK
#define GRND_NONBLOCK 0x0001
#endif
