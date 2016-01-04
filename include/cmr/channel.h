#ifndef _CMR_CHANNEL_H_
#define _CMR_CHANNEL_H_

#include "cmr/session.h"

typedef struct _cmr_chan {
	long long id;
	int worker_idx;
	// sess_hash;
	cmr_sess_t *sess_link;
	int sess_count;
	struct _cmr_chan *next;
} cmr_chan_t;

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
