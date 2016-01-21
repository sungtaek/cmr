#include <stdio.h>
#include <stdlib.h>

#include "ortp/ortp.h"

#include "cmr/util.h"
#include "cmr/cmr.h"

static void _cmr_destroy_worker_pool(cmr_t *cmr);

cmr_t *cmr_create(cmr_conf_t conf)
{
	int i;

	// create cmr
	cmr_t *cmr = (cmr_t *)malloc(sizeof(cmr_t));
	cmr->conf = conf;

	for(i=0; i<cmr->conf.worker_count; i++) {
		// create worker
		cmr_worker_t *worker = cmr_worker_create(i);
		if(!worker) {
			_cmr_destroy_worker_pool(cmr);
			return NULL;
		}

		// attach worker to cmr
		cmr_worker_set_cmr(worker, cmr);
		cmr_worker_set_next(worker, cmr->worker_pool);
		cmr->worker_pool = worker;
	}

	cmr->chan_hash = NULL;
	ortp_init();
	ortp_scheduler_init();

	printf("cmr created!!\n");
	return cmr;
}

int cmr_start(cmr_t *cmr)
{
	cmr_worker_t *worker = NULL;

	if(!cmr) {
		return ERR_INVALID_PARAM;
	}

	// start worker
	worker = cmr->worker_pool;
	while(worker) {
		cmr_worker_start(worker);
		printf("start worker %d ...\n", cmr_worker_get_idx(worker));
		worker = cmr_worker_get_next(worker);
	}
	
	return SUCC;
}

int cmr_stop(cmr_t *cmr)
{
	cmr_worker_t *worker = NULL;

	if(!cmr) {
		return ERR_INVALID_PARAM;
	}

	// stop worker
	worker = cmr->worker_pool;
	while(worker) {
		cmr_worker_stop(worker);
		printf("stop worker %d ...\n", cmr_worker_get_idx(worker));
		worker = cmr_worker_get_next(worker);
	}

	return SUCC;
}

void cmr_destroy(cmr_t *cmr)
{
	if(cmr) {
		cmr_stop(cmr);
		_cmr_destroy_worker_pool(cmr);
		free(cmr);
	}
}

int cmr_add_channel(cmr_t *cmr, cmr_chan_t *chan)
{
	cmr_worker_t *worker = NULL;
	cmr_worker_t *min_worker = NULL;

	if(!cmr || !chan) {
		return ERR_INVALID_PARAM;
	}

	// find min_session worker
	worker = cmr->worker_pool;
	while(worker) {
		if(min_worker) {
			if(cmr_worker_get_chan_count(min_worker) > cmr_worker_get_chan_count(worker)) {
				min_worker = worker;
			}
		}
		else {
			min_worker = worker;
		}
		worker = cmr_worker_get_next(worker);
	}

	if(!min_worker) {
		return ERR_SESSION_FULL;
	}

	HASH_ADD(cmr_hh, cmr->chan_hash, id, sizeof(chan->id), chan);
	return cmr_worker_add_channel(min_worker, chan);
}

cmr_chan_t *cmr_get_channel(cmr_t *cmr, long long chan_id)
{
	cmr_chan_t *chan = NULL;

	if(!cmr) {
		return NULL;
	}

	HASH_FIND(cmr_hh, cmr->chan_hash, &chan_id, sizeof(chan_id), chan);
	return chan;
}

int cmr_get_all_channel(cmr_t *cmr, cmr_chan_t **chan_list, int list_len)
{
	int len=0;
	cmr_chan_t *chan = NULL, *tmp;

	HASH_ITER(cmr_hh, cmr->chan_hash, chan, tmp) {
		if(len < list_len) {
			chan_list[len++] = chan;
		}
		else {
			break;
		}
	}
	return len;
}

int cmr_get_all_channel_in_worker(cmr_t *cmr, int worker_idx, cmr_chan_t **chan_list, int list_len)
{
	int i, len=0;
	cmr_worker_t *worker = NULL;

	for(worker = cmr->worker_pool, i=0; worker && i<worker_idx; i++) {
		worker = worker->next;
	}

	if(worker) {
		len = cmr_worker_get_all_channel(worker, chan_list, list_len);
	}

	return len;
}

cmr_chan_t *cmr_remove_channel(cmr_t *cmr, long long chan_id)
{
	cmr_chan_t *chan = NULL;

	chan = cmr_get_channel(cmr, chan_id);
	if(chan) {
		if(chan->worker) {
			cmr_worker_remove_channel(chan->worker, chan_id);
		}
		HASH_DELETE(cmr_hh, cmr->chan_hash, chan);
	}

	return chan;
}

static void _cmr_destroy_worker_pool(cmr_t *cmr)
{
	if(cmr && cmr->worker_pool) {
		cmr_worker_t *worker = cmr->worker_pool;
		while(worker) {
			cmr_worker_t *next = cmr_worker_get_next(worker);
			cmr_worker_destroy(worker);
			worker = next;
		}
		cmr->worker_pool = NULL;
	}
}

