#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "ortp/sessionset.h"

#include "cmr/util.h"

#include "worker.h"

typedef struct _job {
	void (*cmd)(void*);
	void *arg;
	struct _job *next;
} job_t;

typedef struct _worker {
	int init;
	int count;
	cmr_thread_t thr[CMR_MAX_WORKER];
	volatile int run;
	job_t *queue;
	cmr_cond_t queue_cond;
	cmr_mutex_t queue_lock;
} worker_t;

worker_t g_worker;

static void *_worker_thread(void *arg);

int worker_init(int count)
{
	if(g_worker.init) {
		return ERR_ALREADY_INITIALIZED;
	}

	if(count < 1 || count > CMR_MAX_WORKER) {
		return ERR_INVALID_PARAM;
	}

	g_worker.count = count;
	g_worker.run = 0;
	g_worker.queue = NULL;
	cmr_cond_init(&g_worker.queue_cond, NULL);
	cmr_mutex_init(&g_worker.queue_lock, NULL);

	g_worker.init = 1;

	return SUCC;
}

int worker_start()
{
	int ret, i;

	if(!g_worker.init) {
		return ERR_NOT_INITIALIZED;
	}

	if(!g_worker.run) {
		g_worker.run = 1;
		for(i=0; i<g_worker.count; i++) {
			if((ret = cmr_thread_create(&g_worker.thr[i], NULL, _worker_thread, (void*)(long)i))) {
				printf("cmr_thread_create fail(err:%d)\n", ret);
				return ERR;
			}
		}
	}

	return SUCC;
}

int worker_stop()
{
	int i;

	if(!g_worker.init) {
		return ERR_NOT_INITIALIZED;
	}

	if(g_worker.run) {
		g_worker.run = 0;
		cmr_cond_broadcast(&g_worker.queue_cond);
		for(i=0; i<g_worker.count; i++) {
			cmr_thread_join(g_worker.thr[i], NULL);
		}
	}

	return SUCC;
}

int worker_put_job(void (*cmd)(void*), void *arg)
{
	job_t *job = NULL;

	if(!g_worker.init) {
		return ERR_NOT_INITIALIZED;
	}

	job = (job_t*)malloc(sizeof(job_t));
	if(!job) {
		return ERR_SYSTEM_MEMORY;
	}
	job->cmd = cmd;
	job->arg = arg;
	job->next = NULL;

	LL_APPEND(g_worker.queue, job);
	cmr_cond_signal(&g_worker.queue_cond);

	return SUCC;
}

int cmr_worker_get_pending_job_count()
{
	int count = 0;
	job_t *job = NULL;

	if(!g_worker.init) {
		return ERR_NOT_INITIALIZED;
	}

	LL_COUNT(g_worker.queue, job, count);

	return count;
}

static void *_worker_thread(void *arg)
{
	int idx = (int)(long)arg;
	job_t *job = NULL;
	struct timespec time_to_wait = {0, 0};

	printf("worker_%02d start!\n", idx);

	while(g_worker.run) {

		// pop job at head
		cmr_mutex_lock(&g_worker.queue_lock);
		if(g_worker.queue) {
			job = g_worker.queue;
			LL_DELETE(g_worker.queue, job);
		}
		else {
			job = NULL;
		}
		cmr_mutex_unlock(&g_worker.queue_lock);

		if(job) {
			job->cmd(job->arg);
			free(job);
		}
		else {
			// wait signal
			time_to_wait.tv_sec = time(NULL) + 1;
			time_to_wait.tv_nsec = 0;

			cmr_mutex_lock(&g_worker.queue_lock);
			cmr_cond_timedwait(&g_worker.queue_cond, &g_worker.queue_lock, &time_to_wait);
			cmr_mutex_unlock(&g_worker.queue_lock);
		}
	}

	printf("worker_%02d stopped\n", idx);
}

