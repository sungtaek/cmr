#include <stdio.h>
#include <unistd.h>

#include "ortp/sessionset.h"

#include "cmr/worker.h"


typedef struct _cmr_cmd_req {
	void *(*command)(void*);
	void *arg;
} cmr_cmd_req_t;

typedef struct _cmr_cmd_resp {
	void *result;
} cmr_cmd_resp_t;

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
	worker->cmr = NULL;
	worker->id = 0;
	cmr_rwlock_init(&worker->lock, NULL);
	cmr_mutex_init(&worker->cmd_lock, NULL);
	worker->cmd_pipe[0] = -1;
	worker->cmd_pipe[1] = -1;
	worker->sess_set = session_set_new();
	if(!worker->sess_set) {
		cmr_rwlock_destroy(&worker->lock);
		cmr_mutex_destroy(&worker->cmd_lock);
		free(worker);
		return NULL;
	}
	worker->chan_count = 0;
	worker->chan_hash = NULL;
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
	if(!worker) {
		return;
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

static void *_cmr_worker_stop_command(void *arg)
{
	cmr_worker_t *worker = (cmr_worker_t*)arg;

	// close cmd_pipe
	close(worker->cmd_pipe[0]);
	close(worker->cmd_pipe[1]);
	worker->cmd_pipe[0] = -1;
	worker->cmd_pipe[1] = -1;

	// disable worker
	worker->id = 0;

	return NULL;
}

void cmr_worker_stop(cmr_worker_t *worker)
{
	if(!worker) {
		return;
	}

	if(!cmr_worker_is_run(worker)) {
		return;
	}

	if(cmr_worker_is_in_worker(worker)){
		_cmr_worker_stop_command(worker);
	}
	else {
		cmr_worker_command(worker, _cmr_worker_stop_command, worker);
	}
}

void *cmr_worker_command(cmr_worker_t *worker, void *(*command)(void *), void *arg)
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

	if(write(worker->cmd_pipe[0], &req, sizeof(cmr_cmd_req_t)) < 0) {
		cmr_mutex_unlock(&worker->cmd_lock);
		return NULL;
	}

	if(read(worker->cmd_pipe[0], &resp, sizeof(cmr_cmd_resp_t)) < 0) {
		cmr_mutex_unlock(&worker->cmd_lock);
		return NULL;
	}

	cmr_mutex_unlock(&worker->cmd_lock);

	return resp.result;
}

typedef struct _add_chan_arg {
	cmr_worker_t *worker;
	cmr_chan_t *chan;
} add_chan_arg_t;

static void *_cmr_worker_add_channel(void *arg)
{
	cmr_worker_t *worker = ((add_chan_arg_t*)arg)->worker;
	cmr_chan_t *chan = ((add_chan_arg_t*)arg)->chan;

	if(cmr_chan_get_worker(chan) != NULL) {
		return (void*)ERR_ALREADY_USED;
	}

	if(cmr_worker_get_channel(worker, cmr_chan_get_id(chan))) {
		return (void*)ERR_ALREADY_EXIST;
	}

	cmr_rwlock_wrlock(&worker->lock);
	HASH_ADD(worker_hh, worker->chan_hash, id, sizeof(chan->id), chan);
	cmr_chan_set_worker(chan, worker);
	worker->chan_count++;
	cmr_rwlock_unlock(&worker->lock);

	return (void*)(long)worker->chan_count;
}

int cmr_worker_add_channel(cmr_worker_t *worker, cmr_chan_t *chan)
{
	add_chan_arg_t arg;

	if(!worker || !chan) {
		return ERR_INVALID_PARAM;
	}

	arg.worker = worker;
	arg.chan = chan;

	if(cmr_worker_is_run(worker)
			&& cmr_worker_is_in_worker(worker)) {
		return (int)(long)cmr_worker_command(worker, _cmr_worker_add_channel, &arg);
	}
	return (int)(long)_cmr_worker_add_channel(&arg);
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

typedef struct _remove_chan_arg {
	cmr_worker_t *worker;
	long long chan_id;
} remove_chan_arg_t;

static void *_cmr_worker_remove_channel(void *arg)
{
	cmr_worker_t *worker = ((remove_chan_arg_t*)arg)->worker;
	long long chan_id = ((remove_chan_arg_t*)arg)->chan_id;
	cmr_chan_t *chan = NULL;

	chan = cmr_worker_get_channel(worker, chan_id);
	if(chan) {
		cmr_rwlock_wrlock(&worker->lock);
		HASH_DELETE(worker_hh, worker->chan_hash, chan);
		cmr_chan_set_worker(chan, NULL);
		worker->chan_count--;
		cmr_rwlock_unlock(&worker->lock);
		return chan;
	}

	return NULL;
}

cmr_chan_t *cmr_worker_remove_channel(cmr_worker_t *worker, long long chan_id)
{
	remove_chan_arg_t arg;

	if(!worker) {
		return NULL;
	}

	arg.worker = worker;
	arg.chan_id = chan_id;
	if(cmr_worker_is_run(worker)
			&& cmr_worker_is_in_worker(worker)) {
		return (cmr_chan_t*)cmr_worker_command(worker, _cmr_worker_remove_channel, &arg);
	}

	return (cmr_chan_t*)_cmr_worker_remove_channel(&arg);
}



static void *_cmr_worker_thread(void *arg)
{
	cmr_worker_t *worker = (cmr_worker_t*) arg;

	pipe(worker->cmd_pipe);
	worker->id = cmr_thread_self();

	while(worker->id) {
		// TUDO
	}
}


