#ifndef _CMR_WORKER_H_
#define _CMR_WORKER_H_

#include "ortp/sessionset.h"

#include "cmr/util.h"

typedef struct _cmr cmr_t;

typedef struct _cmr_worker {
	cmr_t	*cmr;
	cmr_thread_t thr;
	int chann_count;
	int sess_count;
	SessionSet *sess_set;
} cmr_worker_t;

#ifdef __cplusplus
extern "C" {
#endif

cmr_worker_t *cmr_worker_create(cmr_t *cmr);
void cmr_worker_run(cmr_worker_t *worker);

#ifdef __cplusplus
}
#endif


#endif /* _CMR_H_ */

