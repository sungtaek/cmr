#ifndef _CMR_WORKER_H_
#define _CMR_WORKER_H_

#include "ortp/sessionset.h"

#include "cmr/util.h"

typedef struct _cmr cmr_t;

typedef struct _cmr_worker {
	int idx;
	cmr_thread_t thr;
	int sess_count;
	int command_fd;
	SessionSet *sess_set;
} cmr_worker_t;

#ifdef __cplusplus
extern "C" {
#endif

cmr_worker_t *cmr_worker_create();
void cmr_worker_start(cmr_worker_t *worker);
void *cmr_worker_command(


#ifdef __cplusplus
}
#endif


#endif /* _CMR_H_ */

