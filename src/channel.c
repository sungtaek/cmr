#include <stdio.h>

#include "cmr/util.h"
#include "cmr/channel.h"

cmr_chan_t *cmr_chan_create()
{
	cmr_chan_t *chan = NULL;

	chan = (cmr_chan_t*)malloc(sizeof(cmr_chan_t));
	if(!chan) {
		return NULL;
	}

	chan->id = gen_unique_id();
	chan->worker = NULL;
	chan->sess_hash = NULL;
	chan->sess_count = 0;

	return chan;
}

void cmr_chan_destroy(cmr_chan_t *chan)
{
	if(chan) {
		cmr_sess_t* cur, tmp;

		// TODO: check namespace
		if(chan->worker) {
			cmr_worker_remove_channel(chan->worker, chan->id);
		}
		if(chan->sess_hash) {
			HASH_ITER(chan_hh, chan->sess_hash, cur, tmp) {
				HASH_DEL(chan->sess_hash, cur);
				cur->chan = NULL;
				cmr_sess_destroy(cur);
			}
		}
	}
}


// session functions
int cmr_chan_add_session(cmr_chan_t *chan, cmr_sess_t *sess);
cmr_sess_t *cmr_chan_get_session(cmr_chan_t *chan, long long sess_id);
int cmr_chan_get_all_session(cmr_chan_t *chan, cmr_sess_t **sess_list, int list_len);
cmr_sess_t *cmr_chan_remove_session(cmr_chan_t *chan, long long sess_id);

