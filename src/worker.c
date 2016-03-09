#include <stdio.h>
#include <unistd.h>

#include "ortp/sessionset.h"

#include "cmr/worker.h"


typedef struct _cmr_cmd_req {
	void *(*command)(cmr_worker_t*, void*);
	void *arg;
} cmr_cmd_req_t;

typedef struct _cmr_cmd_resp {
	void *result;
} cmr_cmd_resp_t;

#define CMR_BUF_SIZE	160

static void *_cmr_worker_thread(void *arg);

cmr_worker_t *cmr_worker_create(int idx)
{
	int ret;
	cmr_worker_t *worker = NULL;
	
	worker = (cmr_worker_t*)malloc(sizeof(cmr_worker_t));
	if(!worker) {
		return NULL;
	}

	worker->idx = idx;
	worker->id = 0;
	cmr_rwlock_init(&worker->lock, NULL);
	cmr_mutex_init(&worker->cmd_lock, NULL);
	worker->cmd_req_pipe[0] = -1;
	worker->cmd_req_pipe[1] = -1;
	worker->cmd_resp_pipe[0] = -1;
	worker->cmd_resp_pipe[1] = -1;
	worker->chan_hash = NULL;
	worker->sess_hash = NULL;
	worker->next = NULL;

	return worker;
}

void cmr_worker_destroy(cmr_worker_t *worker)
{
	if(worker) {
		cmr_chan_t *chan = NULL;
		cmr_chan_t *tmp = NULL;

		cmr_worker_stop(worker);

		if(worker->chan_hash) {
			cmr_rwlock_wrlock(&worker->lock);
			HASH_ITER(worker_hh, worker->chan_hash, chan, tmp) {
				HASH_DELETE(worker_hh, worker->chan_hash, chan);
				cmr_chan_set_worker(chan, NULL);
				cmr_chan_destroy(chan);
			}
			cmr_rwlock_unlock(&worker->lock);
		}

		cmr_rwlock_destroy(&worker->lock);
		cmr_mutex_destroy(&worker->cmd_lock);
		free(worker);
	}
}

int cmr_worker_start(cmr_worker_t *worker)
{
	if(!g_cmr.init) {
		return ERR_NOT_INITIALIZED;
	}

	if(!worker) {
		return ERR_INVALID_PARAM;
	}

	if(worker->id) {
		// already started
		return SUCC;
	}

	if(!cmr_thread_create(&worker->thr, NULL, _cmr_worker_thread, worker)) {
		return ERR;
	}

	return SUCC;
}

int cmr_worker_is_run(cmr_worker_t *worker)
{
	if(worker && worker->id) {
		return 1;
	}
	return 0;
}

int cmr_worker_is_in_worker(cmr_worker_t *worker)
{
	if(worker) {
		if(worker->id == cmr_thread_self()) {
			return 1;
		}
	}
	return 0;
}

static void *_cmr_worker_stop_command(cmr_worker_t *worker, void *arg)
{
	// close cmd_pipe
	close(worker->cmd_req_pipe[0]);
	close(worker->cmd_req_pipe[1]);
	close(worker->cmd_resp_pipe[0]);
	close(worker->cmd_resp_pipe[1]);
	worker->cmd_req_pipe[0] = -1;
	worker->cmd_req_pipe[1] = -1;
	worker->cmd_resp_pipe[0] = -1;
	worker->cmd_resp_pipe[1] = -1;

	// disable worker
	worker->id = 0;

	return NULL;
}

void cmr_worker_stop(cmr_worker_t *worker)
{
	if(!g_cmr.init) {
		return;
	}

	if(!worker) {
		return;
	}

	if(!cmr_worker_is_run(worker)) {
		return;
	}

	if(cmr_worker_is_in_worker(worker)){
		_cmr_worker_stop_command(worker, NULL);
	}
	else {
		cmr_worker_command(worker, _cmr_worker_stop_command, NULL);
	}
}

void *cmr_worker_command(cmr_worker_t *worker, void *(*command)(cmr_worker_t*, void *), void *arg)
{
	cmr_cmd_req_t req;
	cmr_cmd_resp_t resp;

	if(!worker || !command) {
		return NULL;
	}

	req.command = command;
	req.arg = arg;
	resp.result = NULL;

	cmr_mutex_lock(&worker->cmd_lock);

	if(write(worker->cmd_req_pipe[1], &req, sizeof(cmr_cmd_req_t)) < 0) {
		cmr_mutex_unlock(&worker->cmd_lock);
		return NULL;
	}

	if(read(worker->cmd_resp_pipe[0], &resp, sizeof(cmr_cmd_resp_t)) < 0) {
		cmr_mutex_unlock(&worker->cmd_lock);
		return NULL;
	}

	cmr_mutex_unlock(&worker->cmd_lock);

	return resp.result;
}

static void *_cmr_worker_add_channel(cmr_worker_t *worker, void *arg)
{
	cmr_chan_t *chan = (cmr_chan_t*)arg;
	int count = 0;
	cmr_sess_t *sess = NULL;
	cmr_sess_t *stmp = NULL;

	if(cmr_chan_get_worker(chan) != NULL) {
		return (void*)ERR_ALREADY_USED;
	}

	if(cmr_worker_get_channel(worker, cmr_chan_get_id(chan))) {
		return (void*)ERR_ALREADY_EXIST;
	}

	cmr_rwlock_wrlock(&worker->lock);
	HASH_ADD(worker_hh, worker->chan_hash, id, sizeof(chan->id), chan);
	cmr_chan_set_worker(chan, worker);
	count = HASH_CNT(worker_hh, worker->chan_hash);
	cmr_rwlock_unlock(&worker->lock);

	HASH_ITER(chan_hh, chan->sess_hash, sess, stmp) {
		cmr_worker_register_session(worker, sess);
	}

	return (void*)(long)count;
}

int cmr_worker_add_channel(cmr_worker_t *worker, cmr_chan_t *chan)
{
	if(!worker || !chan) {
		return ERR_INVALID_PARAM;
	}

	if(cmr_worker_is_run(worker)
			&& !cmr_worker_is_in_worker(worker)) {
		return (int)(long)cmr_worker_command(worker, _cmr_worker_add_channel, chan);
	}
	return (int)(long)_cmr_worker_add_channel(worker, chan);
}

cmr_chan_t *cmr_worker_get_channel(cmr_worker_t *worker, long long chan_id)
{
	cmr_chan_t *chan = NULL;

	if(!worker) {
		return NULL;
	}

	cmr_rwlock_rdlock(&worker->lock);
	HASH_FIND(worker_hh, worker->chan_hash, &chan_id, sizeof(chan_id), chan);
	cmr_rwlock_unlock(&worker->lock);

	return chan;
}

int cmr_worker_get_all_channel(cmr_worker_t *worker, cmr_chan_t **chan_list, int list_len)
{
	int len=0;
	cmr_chan_t *chan=NULL;
	cmr_chan_t *tmp=NULL;

	if(!worker || !chan_list) {
		return ERR_INVALID_PARAM;
	}

	cmr_rwlock_rdlock(&worker->lock);
	HASH_ITER(worker_hh, worker->chan_hash, chan, tmp) {
		if(len < list_len) {
			chan_list[len++] = chan;
		}
		else {
			break;
		}
	}
	cmr_rwlock_unlock(&worker->lock);

	return len;
}

static void *_cmr_worker_remove_channel(cmr_worker_t *worker, void *arg)
{
	long long chan_id = (long long)arg;
	cmr_chan_t *chan = NULL;

	chan = cmr_worker_get_channel(worker, chan_id);
	if(chan) {
		cmr_sess_t *sess = NULL;
		cmr_sess_t *stmp = NULL;
		
		HASH_ITER(chan_hh, chan->sess_hash, sess, stmp) {
			cmr_worker_unregister_session(worker, sess);
		}

		cmr_rwlock_wrlock(&worker->lock);
		HASH_DELETE(worker_hh, worker->chan_hash, chan);
		cmr_chan_set_worker(chan, NULL);
		cmr_rwlock_unlock(&worker->lock);

		return chan;
	}

	return NULL;
}

cmr_chan_t *cmr_worker_remove_channel(cmr_worker_t *worker, long long chan_id)
{
	if(!worker) {
		return NULL;
	}

	if(cmr_worker_is_run(worker)
			&& !cmr_worker_is_in_worker(worker)) {
		return (cmr_chan_t*)cmr_worker_command(worker, _cmr_worker_remove_channel, (void*)chan_id);
	}

	return (cmr_chan_t*)_cmr_worker_remove_channel(worker, (void*)chan_id);
}

int cmr_worker_get_channel_count(cmr_worker_t *worker)
{
	if(!worker) {
		return ERR_INVALID_PARAM;
	}

	return HASH_CNT(worker_hh, worker->chan_hash);
}

static void *_cmr_worker_register_sessions(cmr_worker_t *worker, void *arg)
{
	cmr_sess_t *sess = (cmr_sess_t*)arg;
	int count;

	cmr_rwlock_wrlock(&worker->lock);
	HASH_ADD(worker_hh, worker->sess_hash, id, sizeof(sess->id), sess);
	count = HASH_CNT(worker_hh, worker->sess_hash);
	cmr_rwlock_unlock(&worker->lock);

	return (void*)(long)count;
}

int cmr_worker_register_session(cmr_worker_t *worker, cmr_sess_t *sess)
{
	if(!worker || !sess) {
		return ERR_INVALID_PARAM;
	}

	if(cmr_worker_is_run(worker)
			&& !cmr_worker_is_in_worker(worker)) {
		return (int)(long)cmr_worker_command(worker, _cmr_worker_register_sessions, sess);
	}
	return (int)(long)_cmr_worker_register_sessions(worker, sess);
}

static void *_cmr_worker_unregister_sessions(cmr_worker_t *worker, void *arg)
{
	cmr_sess_t *sess = (cmr_sess_t*)arg;
	int count;

	cmr_rwlock_wrlock(&worker->lock);
	HASH_DELETE(worker_hh, worker->sess_hash, sess);
	count = HASH_CNT(worker_hh, worker->sess_hash);
	cmr_rwlock_unlock(&worker->lock);

	return (void*)(long)count;
}

int cmr_worker_unregister_session(cmr_worker_t *worker, cmr_sess_t *sess)
{
	if(!worker || !sess) {
		return ERR_INVALID_PARAM;
	}

	if(cmr_worker_is_run(worker)
			&& !cmr_worker_is_in_worker(worker)) {
		return (int)(long)cmr_worker_command(worker, _cmr_worker_unregister_sessions, sess);
	}
	return (int)(long)_cmr_worker_unregister_sessions(worker, sess);
}

int cmr_worker_get_session_count(cmr_worker_t *worker)
{
	if(!worker) {
		return ERR_INVALID_PARAM;
	}

	return HASH_CNT(worker_hh, worker->sess_hash);
}

static void _cmr_worker_relay_packet(cmr_worker_t *worker, cmr_sess_t *sess)
{
	cmr_chan_t *chan = cmr_sess_get_chan(sess);
	mblk_t *mp = NULL;
	mblk_t *dmp = NULL;

	RtpSession *rcv_rawsess = cmr_sess_get_rawsess(sess);
	int packet_size = g_cmr.conf.packet_size;

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
	chan->ts += g_cmr.conf.inc_ts;
}

static void _cmr_worker_drop_packet(cmr_worker_t *worker, cmr_sess_t *sess)
{
	cmr_chan_t *chan = cmr_sess_get_chan(sess);
	mblk_t *mp = NULL;

	RtpSession *rcv_rawsess = cmr_sess_get_rawsess(sess);
	int packet_size = g_cmr.conf.packet_size;

	do {
		mp = rtp_session_recvm_with_ts(rcv_rawsess, chan->ts);
		if(mp != NULL) {
			freemsg(mp);
		}
	} while(mp != NULL);
}

static void _cmr_worker_proc_command(cmr_worker_t *worker)
{
	cmr_cmd_req_t req;
	cmr_cmd_resp_t resp;

	if(read(worker->cmd_req_pipe[0], &req, sizeof(cmr_cmd_req_t)) < 0) {
		return;
	}

	resp.result = req.command(worker, req.arg);

	if(write(worker->cmd_resp_pipe[1], &resp, sizeof(cmr_cmd_resp_t)) < 0) {
		return;
	}
}

static void *_cmr_worker_thread(void *arg)
{
	cmr_worker_t *worker = (cmr_worker_t*) arg;
	SessionSet *sess_set = session_set_new();

	pipe(worker->cmd_req_pipe);
	pipe(worker->cmd_resp_pipe);
	worker->id = cmr_thread_self();

	while(worker->id) {
		int k;
		cmr_sess_t *sess=NULL;
		cmr_sess_t *stmp=NULL;

		session_set_init(sess_set);

		// add channel's session to session_set
		//printf("regi sess count: %d\n", HASH_CNT(worker_hh, worker->sess_hash));
		cmr_rwlock_rdlock(&worker->lock);
		HASH_ITER(worker_hh, worker->sess_hash, sess, stmp) {
			session_set_set(sess_set, sess->raw_sess);
		}
		cmr_rwlock_unlock(&worker->lock);

		// add command pipe to session_set
		//ORTP_FD_SET(worker->cmd_req_pipe[0], &sess_set->rtpset);

		//printf("try...\n");
		printf("[%d]try select...\n", worker->idx);
		k = session_set_select(sess_set, NULL, NULL);
		printf("[%d]selected %d\n", worker->idx, k);

		if(k > 0) {
			// proc sess packet
			HASH_ITER(worker_hh, worker->sess_hash, sess, stmp) {
				if(session_set_is_set(sess_set, cmr_sess_get_rawsess(sess))) {
					//printf("recv sess(%lu)\n", sess->id);
					switch(cmr_sess_get_mode(sess)) {
						case SESS_MODE_SENDONLY:
						case SESS_MODE_SENDRECV:
							// relay packet
							_cmr_worker_relay_packet(worker, sess);
							break;
						default:
							// drop packet
							_cmr_worker_drop_packet(worker, sess);
							break;
					}
				}
			}

			// proc command
			/*
			if(ORTP_FD_ISSET(worker->cmd_req_pipe[0], &sess_set->rtpset)) {
				_cmr_worker_proc_command(worker);
			}
			*/
		}
		sleep(1);

	}
}


