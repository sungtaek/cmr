#include <stdio.h>

#include "cmr/cmr.h"
#include "cmr/channel.h"
#include "cmr/util.h"

cmr_chan_t *cmr_chan_create()
{
	cmr_chan_t *chan = NULL;

	if(!cmr_is_init()) {
		return NULL;
	}

	chan = (cmr_chan_t*)malloc(sizeof(cmr_chan_t));
	if(!chan) {
		return NULL;
	}

	chan->id = gen_unique_id();
	cmr_rwlock_init(&chan->lock, NULL);
	chan->sess_hash = NULL;
	chan->ts = 0;

	return chan;
}

void cmr_chan_destroy(cmr_chan_t *chan)
{
	if(chan) {
		cmr_sess_t *sess = NULL;
		cmr_sess_t *tmp = NULL;

		// destroy sessions
		if(chan->sess_hash) {
			cmr_rwlock_wrlock(&chan->lock);
			HASH_ITER(chan_hh, chan->sess_hash, sess, tmp) {
				HASH_DELETE(chan_hh, chan->sess_hash, sess);
				cmr_sess_set_chan(sess, NULL);
				cmr_sess_destroy(sess);
			}
			cmr_rwlock_unlock(&chan->lock);
		}

		// destroy channel
		cmr_rwlock_destroy(&chan->lock);

		free(chan);
	}
}


int cmr_chan_add_session(cmr_chan_t *chan, cmr_sess_t *sess)
{
	int count = 0;

	if(!chan || !sess) {
		return ERR_INVALID_PARAM;
	}

	if(cmr_sess_get_chan(sess)) {
		return ERR_ALREADY_USED;
	}

	if(cmr_chan_get_session(chan, cmr_sess_get_id(sess))) {
		return ERR_ALREADY_EXIST;
	}

	cmr_rwlock_wrlock(&chan->lock);
	HASH_ADD(chan_hh, chan->sess_hash, id, sizeof(sess->id), sess);
	cmr_sess_set_chan(sess, chan);
	count = HASH_CNT(chan_hh, chan->sess_hash);
	cmr_rwlock_unlock(&chan->lock);

	return count;
}

cmr_sess_t *cmr_chan_get_session(cmr_chan_t *chan, unsigned long sess_id)
{
	cmr_sess_t *sess = NULL;

	if(!chan) {
		return NULL;
	}

	cmr_rwlock_rdlock(&chan->lock);
	HASH_FIND(chan_hh, chan->sess_hash, &sess_id, sizeof(sess_id), sess);
	cmr_rwlock_unlock(&chan->lock);

	return sess;
}

int cmr_chan_get_all_session(cmr_chan_t *chan, cmr_sess_t ***sess_list, int list_len)
{
	int len=0;
	cmr_sess_t *sess=NULL;
	cmr_sess_t *tmp=NULL;
	
	if(!chan || !sess_list) {
		return ERR_INVALID_PARAM;
	}

	cmr_rwlock_rdlock(&chan->lock);
	len = HASH_CNT(chan_hh, chan->sess_hash);
	if(len > 0) {
		if(*sess_list == NULL) {
			list_len = len;
			*sess_list = (cmr_sess_t**)malloc(sizeof(cmr_sess_t *) * list_len);
		}
		len = 0;
		HASH_ITER(chan_hh, chan->sess_hash, sess, tmp) {
			if(len < list_len) {
				(*sess_list)[len++] = sess;
			}
			else {
				break;
			}
		}
	}
	cmr_rwlock_unlock(&chan->lock);

	return len;
}

cmr_sess_t *cmr_chan_remove_session(cmr_chan_t *chan, unsigned long sess_id)
{
	cmr_sess_t *sess = NULL;

	if(!chan) {
		return NULL;
	}

	sess = cmr_chan_get_session(chan, sess_id);
	if(sess) {
		cmr_rwlock_wrlock(&chan->lock);
		HASH_DELETE(chan_hh, chan->sess_hash, sess);
		cmr_sess_set_chan(sess, NULL);
		cmr_rwlock_unlock(&chan->lock);
	}

	return sess;
}

int cmr_chan_get_session_count(cmr_chan_t *chan)
{
	if(!chan) {
		return ERR_INVALID_PARAM;
	}

	return HASH_CNT(chan_hh, chan->sess_hash);
}

