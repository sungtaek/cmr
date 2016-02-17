#ifndef _CMR_H_
#define _CMR_H_

typedef struct _cmr_conf cmr_conf_t;
typedef struct _cmr cmr_t;

#include "cmr/util.h"
#include "cmr/worker.h"
#include "cmr/channel.h"
#include "cmr/portmgr.h"

struct _cmr_conf {
	char *local_host;
	char *public_host;
	int worker_count;
	int max_sess_per_chan;
	int max_sess_per_thr;
	int port_start;
	int port_end;
};

#define CMR_CONF_DEFAULT(_c) { \
	(_c).local_host = "0.0.0.0"; \
	(_c).public_host = "0.0.0.0"; \
	(_c).worker_count = 4; \
	(_c).max_sess_per_chan = -1; \
	(_c).max_sess_per_thr = -1; \
	(_c).port_start = 49152; \
	(_c).port_end = 65535; \
}

struct _cmr {
	int init;
	cmr_conf_t conf;
	portmgr_t *portmgr;
	cmr_worker_t *worker_pool;
	cmr_chan_t *chan_hash;
};

extern cmr_t g_cmr;

#ifdef __cplusplus
extern "C" {
#endif

int cmr_init(cmr_conf_t conf);
int cmr_start();
int cmr_stop();

// channel functions
int cmr_add_channel(cmr_chan_t *chan);
cmr_chan_t *cmr_get_channel(long long chan_id);
int cmr_get_all_channel(cmr_chan_t **chan_list, int list_len);
int cmr_get_all_channel_in_worker(int worker_idx, cmr_chan_t **chan_list, int list_len);
cmr_chan_t *cmr_remove_channel(long long chan_id);


#ifdef __cplusplus
}
#endif


#endif /* _CMR_H_ */

