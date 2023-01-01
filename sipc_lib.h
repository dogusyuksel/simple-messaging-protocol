#ifndef __SIPC_LIB_
#define __SIPC_LIB_

#include "sipc_common.h"

int sipc_destroy(_sipc_identifier *identifier);
int sipc_unregister(_sipc_identifier *identifier, char *title);
int sipc_broadcast(_sipc_identifier *identifier, void *data, unsigned int len);
int sipc_send_data(_sipc_identifier *identifier, char *title, void *data, unsigned int len);
int sipc_register(_sipc_identifier *identifier, char *title, int (*callback)(void *, unsigned int));

#endif //__SIPC_LIB_
