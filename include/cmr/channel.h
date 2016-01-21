#ifndef _CMR_CHANNEL_H_
#define _CMR_CHANNEL_H_

typedef struct _cmr_chan cmr_chan_t;

#include "cmr/util.h"
#include "cmr/session.h"
#include "cmr/worker.h"

struct _cmr_chan {
	long long id;
	cmr_worker_t *worker;
	cmr_rwlock_t lock;
	cmr_sess_t *sess_hash;
	int sess_count;
	UT_hash_handle cmr_hh;
	UT_hash_handle worker_hh;
};

#define cmr_chan_set_worker(_c, _w) (_c)->worker = (_w)
#define cmr_chan_get_worker(_c) (_c)->worker
#define cmr_chan_get_id(_c) (_c)->id

#ifdef __cplusplus
extern "C" {
#endif

cmr_chan_t *cmr_chan_create();
void cmr_chan_destroy(cmr_chan_t *chan);

// session functions
int cmr_chan_add_session(cmr_chan_t *chan, cmr_sess_t *sess);
cmr_sess_t *cmr_chan_get_session(cmr_chan_t *chan, long long sess_id);
int cmr_chan_get_all_session(cmr_chan_t *chan, cmr_sess_t **sess_list, int list_len);
cmr_sess_t *cmr_chan_remove_session(cmr_chan_t *chan, long long sess_id);


#ifdef __cplusplus
}
#endif

#endif /* _CMR_CHANNEL_H_ */
