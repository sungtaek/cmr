#ifndef _CMR_UTIL_H_
#define _CMR_UTIL_H_

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <pthread.h>
#include <search.h>

#include "uthash.h"
#include "utlist.h"
#include "utarray.h"

// Thread utils

typedef pthread_t cmr_thread_t;
typedef pthread_attr_t cmr_thread_attr_t;
typedef pthread_mutex_t cmr_mutex_t;
typedef pthread_rwlock_t cmr_rwlock_t;
typedef pthread_rwlockattr_t cmr_rwlockattr_t;
typedef pthread_cond_t cmr_cond_t;

#ifdef __cplusplus
extern "C" {
#endif

int __cmr_thread_join(cmr_thread_t thread, void **ptr);
int __cmr_thread_create(cmr_thread_t *thread, cmr_thread_attr_t *attr, void * (*routine)(void*), void *arg);
unsigned long __cmr_thread_self(void);

#ifdef __cplusplus
}
#endif

#define cmr_thread_create      __cmr_thread_create
#define cmr_thread_join        __cmr_thread_join
#define cmr_thread_self        __cmr_thread_self
#define cmr_thread_exit        pthread_exit
#define cmr_mutex_init         pthread_mutex_init
#define cmr_mutex_lock         pthread_mutex_lock
#define cmr_mutex_unlock       pthread_mutex_unlock
#define cmr_mutex_destroy      pthread_mutex_destroy
#define cmr_rwlock_init        pthread_rwlock_init
#define cmr_rwlock_rdlock      pthread_rwlock_rdlock
#define cmr_rwlock_wrlock      pthread_rwlock_wrlock
#define cmr_rwlock_unlock      pthread_rwlock_unlock
#define cmr_rwlock_destroy     pthread_rwlock_destroy
#define cmr_cond_init          pthread_cond_init
#define cmr_cond_signal        pthread_cond_signal
#define cmr_cond_broadcast     pthread_cond_broadcast
#define cmr_cond_wait          pthread_cond_wait
#define cmr_cond_destroy       pthread_cond_destroy

#define SUCC				0
#define ERR					-1
#define ERR_INVALID_PARAM	-2
#define ERR_SESSION_FULL	-3
#define ERR_SYSTEM_MEMORY	-100


#endif /* _CMR_UTIL_H_ */

