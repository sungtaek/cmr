#include <stdio.h>

#include "orpt/sessionset.h"

#include "cmr/worker.h"

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
	if(!cmr_thread_create(&worker->thr, NULL)) {
		free(worker);
		return NULL;
	}
	worker->sess_count = 0;
	pipe(worker->cmd_pipe);
	worker->sess_set = session_set_new();
	if(!sess_set) {
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

void cmr_worker_start(cmr_worker_t *worker)
{
	// TODO
}

void *cmr_worker_command(cmr_worker_t *worker, void *(*command)(void *), void *arg)
{
	// TODO
	return NULL;
}

int cmr_worker_add_channel(cmr_worker_t *worker, cmr_chan_t *chan)
{
	// TODO
	return SUCC
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
	// TODO
}

