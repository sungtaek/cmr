#include <stdio.h>
#include <unistd.h>

#include "ortp/sessionset.h"

#include "cmr/worker.h"

static void *_cmr_worker_thread(void *arg);
static void *_cmr_worker_stop_command(void *arg);

typedef struct _cmr_cmd_req {
	void *(*command)(void*);
	void *arg;
} cmr_cmd_req_t;

typedef struct _cmr_cmd_resp {
	void *result;
} cmr_cmd_resp_t;

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
	worker->sess_count = 0;
	cmr_mutex_init(&worker->cmd_lock, NULL);
	worker->cmd_pipe[0] = -1;
	worker->cmd_pipe[1] = -1;
	worker->sess_set = session_set_new();
	if(!worker->sess_set) {
		cmr_mutex_destroy(&worker->cmd_lock);
		free(worker);
		return NULL;
	}
	worker->chan_hash = NULL;
	worker->next = NULL;

	return worker;
}

void cmr_worker_destroy(cmr_worker_t *worker)
{
	// TODO
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

int cmr_worker_is_in_worker(cmr_worker_t *worker)
{
	if(worker) {
		if(worker->id == cmr_thread_self()) {
			return 1;
		}
	}
	return 0;
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

int cmr_worker_add_channel(cmr_worker_t *worker, cmr_chan_t *chan)
{
	// TODO
	return SUCC;
}

cmr_chan_t *cmr_worker_get_channel(cmr_worker_t *worker, long long chan_id)
{
	// TODO
	return NULL;
}

int cmr_worker_get_all_channel(cmr_worker_t *worker, cmr_chan_t **chan_list, int list_len)
{
	// TODO
	return SUCC;
}

cmr_chan_t *cmr_worker_remove_channel(cmr_worker_t *worker, long long chan_id)
{
	// TODO
	return NULL;
}

void cmr_worker_stop(cmr_worker_t *worker)
{
	if(!worker) {
		return;
	}

	if(cmr_worker_is_in_worker(worker)){
		_cmr_worker_stop_command(worker);
	}
	else {
		cmr_worker_command(worker, _cmr_worker_stop_command, worker);
	}
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

