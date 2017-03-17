#define _printf_(a,b) __attribute__ ((format (printf, a, b)))
#define _alloc_(...) __attribute__ ((alloc_size(__VA_ARGS__)))
#define _sentinel_ __attribute__ ((sentinel))
#define _unused_ __attribute__ ((unused))
#define _destructor_ __attribute__ ((destructor))
#define _pure_ __attribute__ ((pure))
#define _const_ __attribute__ ((const))
#define _deprecated_ __attribute__ ((deprecated))
#define _packed_ __attribute__ ((packed))
#define _malloc_ __attribute__ ((malloc))
#define _weak_ __attribute__ ((weak))
#define _likely_(x) (__builtin_expect(!!(x),1))
#define _unlikely_(x) (__builtin_expect(!!(x),0))
#define _public_ __attribute__ ((visibility("default")))
#define _hidden_ __attribute__ ((visibility("hidden")))
#define _weakref_(x) __attribute__((weakref(#x)))
#define _alignas_(x) __attribute__((aligned(__alignof(x))))
#define _cleanup_(x) __attribute__((cleanup(x)))

/* Define C11 thread_local attribute even on older gcc compiler
 * version */
#ifndef thread_local
/*
 * Don't break on glibc < 2.16 that doesn't define __STDC_NO_THREADS__
 * see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=53769
 */
#if __STDC_VERSION__ >= 201112L && !(defined(__STDC_NO_THREADS__) || (defined(__GNU_LIBRARY__) && __GLIBC__ == 2 && __GLIBC_MINOR__ < 16))
#define thread_local _Thread_local
#else
#define thread_local __thread
#endif
#endif

/* Define C11 noreturn without <stdnoreturn.h> and even on older gcc
 * compiler versions */
#ifndef noreturn
#if __STDC_VERSION__ >= 201112L
#define noreturn _Noreturn
#else
#define noreturn __attribute__((noreturn))
#endif
#endif
