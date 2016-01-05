#ifndef _CMR_H_
#define _CMR_H_

typedef struct _cmr_conf cmr_conf_t;
typedef struct _cmr cmr_t;

#include "cmr/util.h"
#include "cmr/worker.h"
#include "cmr/channel.h"

struct _cmr_conf {
	int worker_count;
	int max_sess_per_chan;
	int max_sess_per_thr;
	int port_start;
	int port_end;
};

#define CMR_CONF_DEFAULT(_c) { \
	(_c).worker_count = 4; \
	(_c).max_sess_per_chan = -1; \
	(_c).max_sess_per_thr = -1; \
	(_c).port_start = 49152; \
	(_c).port_end = 65535; \
}

struct _cmr {
	cmr_conf_t conf;
	cmr_worker_t *worker_pool;
	cmr_chan_t *chan_hash;
};

#ifdef __cplusplus
extern "C" {
#endif

cmr_t *cmr_create(cmr_conf_t conf);
int cmr_start(cmr_t *cmr);
int cmr_stop(cmr_t *cmr);
void cmr_destroy(cmr_t *cmr);

// channel functions
int cmr_add_channel(cmr_t *cmr, cmr_chan_t *chan);
cmr_chan_t *cmr_get_channel(cmr_t *cmr, long long chan_id);
int cmr_get_all_channel(cmr_t *cmr, cmr_chan_t **chan_list, int list_len);
int cmr_get_all_channel_in_worker(cmr_t *cmr, int worker_idx, cmr_chan_t **chan_list, int list_len);
cmr_chan_t *cmr_remove_channel(cmr_t *cmr, long long chan_id);


#ifdef __cplusplus
}
#endif


#endif /* _CMR_H_ */

