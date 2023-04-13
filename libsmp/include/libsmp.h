#ifndef __LIBSMP_
#define __LIBSMP_

#include "smp_common.h"

int smp_destroy(void);
int smp_unregister(char *title);
int smp_broadcast_unregister(void);
int smp_send_bradcast_data(void *data, ...);
int smp_send_data(char *title, void *data, ...);
int smp_broadcast_register(int (*callback)(void *, unsigned int), ...);
int smp_register(char *title, int (*callback)(void *, unsigned int), ...);

#endif //__LIBSMP_
