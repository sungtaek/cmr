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
	cmr_rwlock_init(&chan->lock, NULL);
	chan->sess_hash = NULL;
	chan->sess_count = 0;

	return chan;
}

void cmr_chan_destroy(cmr_chan_t *chan)
{
	if(chan) {
		cmr_sess_t *sess = NULL;
		cmr_sess_t *tmp = NULL;

		if(chan->worker) {
			cmr_worker_remove_channel(chan->worker, chan->id);
		}

		if(chan->sess_hash) {
			cmr_rwlock_wrlock(&chan->lock);
			HASH_ITER(chan_hh, chan->sess_hash, sess, tmp) {
				HASH_DELETE(chan_hh, chan->sess_hash, sess);
				cmr_sess_set_chan(sess, NULL);
				cmr_sess_destroy(sess);
			}
			cmr_rwlock_unlock(&chan->lock);
		}

		cmr_rwlock_destroy(&chan->lock);
		free(chan);
	}
}


// session functions
typedef struct _add_sess_arg {
	cmr_chan_t *chan;
	cmr_sess_t *sess;
} add_sess_arg_t;

static void *_cmr_chan_add_session(void *arg)
{
	cmr_chan_t *chan = ((add_sess_arg_t*)arg)->chan;
	cmr_sess_t *sess = ((add_sess_arg_t*)arg)->sess;

	if(cmr_sess_get_chan(sess) != NULL) {
		return (void*)ERR_ALREADY_USED;
	}

	if(cmr_chan_get_session(chan, cmr_sess_get_id(sess))) {
		return (void*)ERR_ALREADY_EXIST;
	}

	cmr_rwlock_wrlock(&chan->lock);
	HASH_ADD(chan_hh, chan->sess_hash, id, sizeof(sess->id), sess);
	cmr_sess_set_chan(sess, chan);
	chan->sess_count++;
	cmr_rwlock_unlock(&chan->lock);
	
	return (void*)(long)chan->sess_count;
}

int cmr_chan_add_session(cmr_chan_t *chan, cmr_sess_t *sess)
{
	add_sess_arg_t arg;

	if(!chan || !sess) {
		return ERR_INVALID_PARAM;
	}

	arg.chan = chan;
	arg.sess = sess;

	if(cmr_worker_is_run(chan->worker)
			&& cmr_worker_is_in_worker(chan->worker)) {
		return (int)(long)cmr_worker_command(chan->worker, _cmr_chan_add_session, &arg);
	}
	return (int)(long)_cmr_chan_add_session(&arg);
}

cmr_sess_t *cmr_chan_get_session(cmr_chan_t *chan, long long sess_id)
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

int cmr_chan_get_all_session(cmr_chan_t *chan, cmr_sess_t **sess_list, int list_len)
{
	int len=0;
	cmr_sess_t *sess=NULL;
	cmr_sess_t *tmp=NULL;
	
	if(!chan || !sess_list) {
		return ERR_INVALID_PARAM;
	}

	cmr_rwlock_rdlock(&chan->lock);
	HASH_ITER(chan_hh, chan->sess_hash, sess, tmp) {
		if(len < list_len) {
			sess_list[len++] = sess;
		}
		else {
			break;
		}
	}
	cmr_rwlock_unlock(&chan->lock);
	
	return len;
}

typedef struct _remove_sess_arg {
	cmr_chan_t *chan;
	long long sess_id;
} remove_sess_arg_t;

static void *_cmr_chan_remove_session(void *arg)
{
	cmr_chan_t *chan = ((remove_sess_arg_t*)arg)->chan;
	long long sess_id = ((remove_sess_arg_t*)arg)->sess_id;
	cmr_sess_t *sess = NULL;
	
	sess = cmr_chan_get_session(chan, sess_id);
	if(sess) {
		cmr_rwlock_wrlock(&chan->lock);
		HASH_DELETE(chan_hh, chan->sess_hash, sess);
		cmr_sess_set_chan(sess, NULL);
		chan->sess_count--;
		cmr_rwlock_unlock(&chan->lock);
		return sess;
	}

	return NULL;
}

cmr_sess_t *cmr_chan_remove_session(cmr_chan_t *chan, long long sess_id)
{
	remove_sess_arg_t arg;

	if(!chan) {
		return NULL;
	}

    arg.chan = chan;
    arg.sess_id = sess_id;
	if(cmr_worker_is_run(chan->worker)
			&& cmr_worker_is_in_worker(chan->worker)) {
		return (cmr_sess_t*)cmr_worker_command(chan->worker, _cmr_chan_remove_session, &arg);
	}
	
	return (cmr_sess_t*)_cmr_chan_remove_session(&arg);
}


