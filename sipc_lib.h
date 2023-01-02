#ifndef __SIPC_LIB_
#define __SIPC_LIB_

#include "sipc_common.h"

int sipc_destroy(void);
int sipc_unregister(char *title);
int sipc_send_data(char *title, void *data, unsigned int len);
int sipc_register(char *title, int (*callback)(void *, unsigned int));

#endif //__SIPC_LIB_
