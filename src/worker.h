#ifndef _CMR_WORKER_H_
#define _CMR_WORKER_H_

#include "ortp/sessionset.h"

#include "cmr/cmr.h"
#include "cmr/channel.h"

#define CMR_MAX_WORKER	64

#ifdef __cplusplus
extern "C" {
#endif

int worker_init(int count);
int worker_start();
int worker_stop();

int worker_put_job(void (*cmd)(void*), void *arg);
int worker_get_pending_job_count();

#ifdef __cplusplus
}
#endif


#endif /* _CMR_H_ */

