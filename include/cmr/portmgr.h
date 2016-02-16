#ifndef _PORTMGR_H_
#define _PORTMGR_H_

typedef struct _portmgr portmgr_t;

#include "cmr/util.h"

struct _portmgr {
	int start_port;
	int end_port;
	int range;
	int idx;
	char *pool;
	cmr_mutex_t lock;
};

#ifdef __cplusplus
extern "C" {
#endif

portmgr_t *portmgr_create(int start_port, int end_port);
void portmgr_destroy(portmgr_t *portmgr);

int portmgr_alloc(portmgr_t *portmgr);
void portmgr_return(portmgr_t *portmgr, int port);


#ifdef __cplusplus
}
#endif


#endif /* _PORTMGR_H_ */

