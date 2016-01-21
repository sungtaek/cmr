#ifndef _CMR_WORKER_H_
#define _CMR_WORKER_H_

#include "ortp/sessionset.h"

typedef struct _cmr_worker cmr_worker_t;

#include "cmr/util.h"
#include "cmr/cmr.h"
#include "cmr/channel.h"

struct _cmr_worker {
	int idx;
	cmr_t *cmr;
	cmr_thread_t thr;
	unsigned long id;
	cmr_mutex_t lock;
	cmr_mutex_t cmd_lock;
	int cmd_pipe[2];
	SessionSet *sess_set;
	int chan_count;
	cmr_chan_t *chan_hash;
	cmr_worker_t *next;
};

#define cmr_worker_set_cmr(_w, _c) (_w)->cmr = (_c)
#define cmr_worker_set_next(_w, _n) (_w)->next = (_n)
#define cmr_worker_get_cmr(_w) (_w)->cmr
#define cmr_worker_get_next(_w) (_w)->next
#define cmr_worker_get_idx(_w) (_w)->idx
#define cmr_worker_get_chan_count(_w) (_w)->chan_count

#ifdef __cplusplus
extern "C" {
#endif

cmr_worker_t *cmr_worker_create(int idx);
void cmr_worker_destroy(cmr_worker_t *worker);

int cmr_worker_start(cmr_worker_t *worker);
int cmr_worker_is_run(cmr_worker_t *worker);
int cmr_worker_is_in_worker(cmr_worker_t *worker);
void cmr_worker_stop(cmr_worker_t *worker);

void *cmr_worker_command(cmr_worker_t *worker, void *(*command)(void *), void *arg);

int cmr_worker_add_channel(cmr_worker_t *worker, cmr_chan_t *chan);
cmr_chan_t *cmr_worker_get_channel(cmr_worker_t *worker, long long chan_id);
int cmr_worker_get_all_channel(cmr_worker_t *worker, cmr_chan_t **chan_list, int list_len);
cmr_chan_t *cmr_worker_remove_channel(cmr_worker_t *worker, long long chan_id);

#ifdef __cplusplus
}
#endif


#endif /* _CMR_H_ */

