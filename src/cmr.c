#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ortp/ortp.h"
#include "ortp/sessionset.h"

#include "cmr/cmr.h"
#include "cmr/channel.h"
#include "cmr/session.h"
#include "cmr/util.h"

#include "worker.h"

typedef struct _cmr {
	int init;

	cmr_chan_t *chan_hash;
	cmr_sess_t *sess_hash;
	cmr_rwlock_t chan_lock;
	cmr_rwlock_t sess_lock;

	// selector
	cmr_thread_t sel_thr;
	volatile int sel_run;
	cmr_cond_t done_cond;
	cmr_mutex_t done_lock;
} cmr_t;

cmr_t g_cmr;
cmr_conf_t g_conf;

static void *_cmr_selector(void *arg);
static void _cmr_process_session(void *arg);
static void _cmr_process_relay_packet(cmr_sess_t *sess);
static void _cmr_process_drop_packet(cmr_sess_t *sess);

cmr_conf_t cmr_get_conf()
{
	return g_conf;
}

int cmr_is_init()
{
	return g_cmr.init;
}

int cmr_init(cmr_conf_t conf)
{
	int ret, i;

	if(g_cmr.init) {
		return ERR_ALREADY_INITIALIZED;
	}

	g_conf = conf;

	if((ret = portmgr_init(g_conf.port_start, g_conf.port_end)) < 0) {
		printf("portmgr_init fail");
		return ret;
	}

	if((ret = worker_init(g_conf.worker_count)) < 0) {
		printf("worker_init fail");
		return ret;
	}

	g_cmr.chan_hash = NULL;
	g_cmr.sess_hash = NULL;

	cmr_rwlock_init(&g_cmr.chan_lock, NULL);
	cmr_rwlock_init(&g_cmr.sess_lock, NULL);

	g_cmr.sel_run = 0;
	cmr_cond_init(&g_cmr.done_cond, NULL);
	cmr_mutex_init(&g_cmr.done_lock, NULL);

	ortp_init();
	ortp_scheduler_init();
	//ortp_set_log_level_mask(ORTP_DEBUG|ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR);
	ortp_set_log_level_mask(ORTP_WARNING|ORTP_ERROR);

	g_cmr.init = 1;

	printf("cmr created!!\n");
	return SUCC;
}

int cmr_start()
{
	int ret;

	if(!g_cmr.init) {
		return ERR_NOT_INITIALIZED;
	}

	// start worker
	if((ret = worker_start()) < 0) {
		printf("worker_start fail(ret:%d)\n", ret);
		return ret;
	}

	// start selector
	if(!g_cmr.sel_run) {
		g_cmr.sel_run = 1;
		if((ret = cmr_thread_create(&g_cmr.sel_thr, NULL, _cmr_selector, NULL))) {
			printf("create selector fail(err:%d)\n", ret);
			worker_stop();
			return ret;
		}
	}
	
	return SUCC;
}

int cmr_stop()
{
	int ret;

	if(!g_cmr.init) {
		return ERR_NOT_INITIALIZED;
	}

	// stop selector
	if(g_cmr.sel_run) {
		g_cmr.sel_run = 0;
		cmr_thread_join(g_cmr.sel_thr, NULL);
	}

	// stop worker
	if((ret = worker_stop()) < 0) {
		printf("worker_stop fail");
		return ret;
	}

	return SUCC;
}

int cmr_add_channel(cmr_chan_t *chan)
{
	int ret, i, sess_len;
	int count = 0;
	cmr_sess_t **sess_list = NULL;
	
	if(!g_cmr.init) {
		return ERR_NOT_INITIALIZED;
	}

	if(!chan) {
		return ERR_INVALID_PARAM;
	}

	if(cmr_get_channel(cmr_chan_get_id(chan))) {
		return ERR_ALREADY_EXIST;
	}

	cmr_rwlock_wrlock(&g_cmr.chan_lock);
	HASH_ADD(cmr_hh, g_cmr.chan_hash, id, sizeof(unsigned long), chan);
	count = HASH_CNT(cmr_hh, g_cmr.chan_hash);
	cmr_rwlock_unlock(&g_cmr.chan_lock);

	if((ret = cmr_chan_get_all_session(chan, &sess_list, 0)) < 0) {
		return ret;
	}
	sess_len = ret;

	cmr_rwlock_rdlock(&g_cmr.sess_lock);
	for(i=0; i<sess_len; i++) {
		HASH_ADD(cmr_hh, g_cmr.sess_hash, id, sizeof(unsigned long), sess_list[i]);
	}
	cmr_rwlock_unlock(&g_cmr.sess_lock);

	free(sess_list);

	return count;
}

cmr_chan_t *cmr_get_channel(unsigned long chan_id)
{
	cmr_chan_t *chan = NULL;

	if(!g_cmr.init) {
		return NULL;
	}

	cmr_rwlock_rdlock(&g_cmr.chan_lock);
	HASH_FIND(cmr_hh, g_cmr.chan_hash, &chan_id, sizeof(chan_id), chan);
	cmr_rwlock_unlock(&g_cmr.chan_lock);

	return chan;
}

int cmr_get_all_channel(cmr_chan_t ***chan_list, int list_len)
{
	int len=0;
	cmr_chan_t *chan = NULL, *tmp;

	if(!g_cmr.init) {
		return ERR_NOT_INITIALIZED;
	}

	if(!chan_list) {
		return ERR_INVALID_PARAM;
	}

	cmr_rwlock_rdlock(&g_cmr.chan_lock);
	len = HASH_CNT(cmr_hh, g_cmr.chan_hash);
	if(len > 0) {
		if(*chan_list == NULL) {
			list_len = len;
			*chan_list = (cmr_chan_t**)malloc(sizeof(cmr_chan_t *) * list_len);
		}
		len = 0;
		HASH_ITER(cmr_hh, g_cmr.chan_hash, chan, tmp) {
			if(len < list_len) {
				(*chan_list)[len++] = chan;
			}
			else {
				break;
			}
		}
	}
	cmr_rwlock_unlock(&g_cmr.chan_lock);
	
	return len;
}

cmr_chan_t *cmr_remove_channel(unsigned long chan_id)
{
	cmr_chan_t *chan = NULL;

	if(!g_cmr.init) {
		return NULL;
	}

	chan = cmr_get_channel(chan_id);
	if(chan) {
		int ret, i;
		cmr_sess_t **sess_list = NULL;
		if((ret = cmr_chan_get_all_session(chan, &sess_list, 0)) > 0) {
			cmr_rwlock_wrlock(&g_cmr.sess_lock);
			for(i=0; i<ret; i++) {
				HASH_DELETE(cmr_hh, g_cmr.sess_hash, sess_list[i]);
			}
			cmr_rwlock_unlock(&g_cmr.sess_lock);
			free(sess_list);
		}

		cmr_rwlock_wrlock(&g_cmr.chan_lock);
		HASH_DELETE(cmr_hh, g_cmr.chan_hash, chan);
		cmr_rwlock_unlock(&g_cmr.chan_lock);
	}

	return chan;
}

int cmr_get_channel_count()
{
	return HASH_CNT(cmr_hh, g_cmr.chan_hash);
}

cmr_sess_t *cmr_get_session(unsigned long sess_id)
{
	cmr_sess_t *sess = NULL;

	if(!g_cmr.init) {
		return NULL;
	}

	cmr_rwlock_rdlock(&g_cmr.sess_lock);
	HASH_FIND(cmr_hh, g_cmr.sess_hash, &sess_id, sizeof(sess_id), sess);
	cmr_rwlock_unlock(&g_cmr.sess_lock);

	return sess;
}

int cmr_get_session_count()
{
	return HASH_CNT(cmr_hh, g_cmr.sess_hash);
}

static void *_cmr_selector(void *arg)
{
	int ret, k;
	struct timespec time_to_wait = {0, 0};
	cmr_sess_t *sess = NULL;
	cmr_sess_t *stmp = NULL;
	SessionSet *sess_set = session_set_new();

	printf("selector start!\n");

	while(g_cmr.sel_run) {

		session_set_init(sess_set);

		cmr_rwlock_rdlock(&g_cmr.sess_lock);
		HASH_ITER(cmr_hh, g_cmr.sess_hash, sess, stmp) {
			session_set_set(sess_set, sess->raw_sess);
		}
		cmr_rwlock_unlock(&g_cmr.sess_lock);

		k = session_set_select(sess_set, NULL, NULL);
		if(k > 0) {
			k=0;
			cmr_rwlock_rdlock(&g_cmr.sess_lock);
			HASH_ITER(cmr_hh, g_cmr.sess_hash, sess, stmp) {
				if(session_set_is_set(sess_set, cmr_sess_get_rawsess(sess))) {
					if((ret = worker_put_job(_cmr_process_session,
									(void*)cmr_sess_get_id(sess))) < 0) {
						printf("worker_put_job fail(ret:%d)\n", ret);
					}
					k++;
				}
			}
			cmr_rwlock_unlock(&g_cmr.sess_lock);

			//printf("put %d jobs\n", k);

			while(g_cmr.sel_run && cmr_worker_get_pending_job_count() > 0) {
				time_to_wait.tv_sec = time(NULL) + 1;
				time_to_wait.tv_nsec = 0;

				cmr_mutex_lock(&g_cmr.done_lock);
				cmr_cond_timedwait(&g_cmr.done_cond, &g_cmr.done_lock, &time_to_wait);
				cmr_mutex_unlock(&g_cmr.done_lock);
			}
		}

	}

	printf("selector stopped\n");
}

static void _cmr_process_session(void *arg)
{
	unsigned long sess_id = (unsigned long)arg;
	cmr_sess_t *sess = cmr_get_session(sess_id);

	if(sess) {
		switch(cmr_sess_get_mode(sess)) {
			case SESS_MODE_SENDONLY:
			case SESS_MODE_SENDRECV:
				_cmr_process_relay_packet(sess);
				break;
			default:
				_cmr_process_drop_packet(sess);
				break;
		}
	}

	cmr_cond_signal(&g_cmr.done_cond);
}

static void _cmr_process_relay_packet(cmr_sess_t *sess)
{
	cmr_chan_t *chan = cmr_sess_get_chan(sess);
	mblk_t *mp = NULL;
	mblk_t *dmp = NULL;

	RtpSession *rcv_rawsess = cmr_sess_get_rawsess(sess);

	do {
		mp = rtp_session_recvm_with_ts(rcv_rawsess, chan->ts);
		if(mp != NULL) {
			cmr_sess_t *snd_sess = NULL;
			cmr_sess_t *stmp = NULL;

			//printf("recv (%lu) %d bytes\n", sess->id, msgdsize(mp));
			cmr_rwlock_rdlock(&chan->lock);
			HASH_ITER(chan_hh, chan->sess_hash, snd_sess, stmp) {
				RtpSession *snd_rawsess = cmr_sess_get_rawsess(snd_sess);
				if(snd_sess != sess && cmr_sess_get_mode(snd_sess) != SESS_MODE_SENDONLY) {
					dmp = copymsg(mp);
					//printf("send (%lu) %d bytes\n", snd_sess->id, msgdsize(dmp));
					rtp_session_sendm_with_ts(snd_rawsess, dmp, chan->ts);
				}
			}
			cmr_rwlock_unlock(&chan->lock);
			freemsg(mp);
		}
	} while(mp != NULL);
	chan->ts += g_conf.inc_ts;
}

static void _cmr_process_drop_packet(cmr_sess_t *sess)
{
	cmr_chan_t *chan = cmr_sess_get_chan(sess);
	mblk_t *mp = NULL;

	RtpSession *rcv_rawsess = cmr_sess_get_rawsess(sess);

	do {
		mp = rtp_session_recvm_with_ts(rcv_rawsess, chan->ts);
		if(mp != NULL) {
			freemsg(mp);
		}
	} while(mp != NULL);
}
