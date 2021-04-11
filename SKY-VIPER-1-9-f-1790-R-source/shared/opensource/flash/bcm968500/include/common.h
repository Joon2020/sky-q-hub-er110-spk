#ifndef _COMMON_H
#define _COMMON_H

#include <linux/types.h>
#include <linux/bitops.h>
#include "lib_printf.h"
#include "cfe_timer.h"

typedef struct cons_s {
    char *str;
    int num;
} cons_t;

#define udelay(usec) cfe_usleep(usec)

#define min(X, Y)				\
	({ typeof (X) __x = (X), __y = (Y);	\
		(__x < __y) ? __x : __y; })

#define max(X, Y)				\
	({ typeof (X) __x = (X), __y = (Y);	\
		(__x > __y) ? __x : __y; })

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define ffs generic_ffs

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

#define CONFIG_SYS_HZ   1000
#define reset_timer()
#define get_timer(x)    0

#endif
