#ifndef _CMR_CHANNEL_H_
#define _CMR_CHANNEL_H_

typedef struct _cmr_chan cmr_chan_t;

#include "cmr/util.h"
#include "cmr/session.h"

struct _cmr_chan {
	unsigned long id;
	cmr_rwlock_t lock;
	cmr_sess_t *sess_hash;
	uint32_t ts;
	UT_hash_handle cmr_hh;
	UT_hash_handle worker_hh;
};

#define cmr_chan_get_id(_c) (_c)->id

#ifdef __cplusplus
extern "C" {
#endif

cmr_chan_t *cmr_chan_create();
void cmr_chan_destroy(cmr_chan_t *chan);

// session functions
int cmr_chan_add_session(cmr_chan_t *chan, cmr_sess_t *sess);
cmr_sess_t *cmr_chan_get_session(cmr_chan_t *chan, unsigned long sess_id);
int cmr_chan_get_all_session(cmr_chan_t *chan, cmr_sess_t ***sess_list, int list_len);
cmr_sess_t *cmr_chan_remove_session(cmr_chan_t *chan, unsigned long sess_id);
int cmr_chan_get_session_count(cmr_chan_t *chan);


#ifdef __cplusplus
}
#endif

#endif /* _CMR_CHANNEL_H_ */
