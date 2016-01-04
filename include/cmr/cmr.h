#ifndef _CMR_H_
#define _CMR_H_

#include "cmr/worker.h"
#include "cmr/util.h"

typedef struct _cmr_conf {
	int worker_count;
	int max_sess_per_chann;
	int max_sess_per_thr;
	int port_start;
	int port_end;
} cmr_conf_t;

#define CMR_CONF_DEFAULT(_c) { \
	(_c).worker_count = 4; \
	(_c).max_sess_per_chann = -1; \
	(_c).max_sess_per_thr = -1; \
	(_c).port_start = 49152; \
	(_c).port_end = 65535; \
}

typedef struct _cmr {
	cmr_conf_t conf;
	cmr_worker_t *worker_pool;
} cmr_t;

#ifdef __cplusplus
extern "C" {
#endif

cmr_t *cmr_create(cmr_conf_t conf);
int cmr_start(cmr_t *cmr);
int cmr_stop(cmr_t *cmr);
void cmr_destroy(cmr_t *cmr);

#ifdef __cplusplus
}
#endif


#endif /* _CMR_H_ */

