#include <stdio.h>
#include <sys/time.h>

#include "cmr/util.h"

int __cmr_thread_join(cmr_thread_t thread, void **ptr)
{
	int err=pthread_join(thread, ptr);
	return err;
}

int __cmr_thread_create(cmr_thread_t *thread, cmr_thread_attr_t *attr, void * (*routine)(void*), void *arg)
{
	pthread_attr_t my_attr;
	pthread_attr_init(&my_attr);
	if (attr)
		my_attr = *attr;
	return pthread_create(thread, &my_attr, routine, arg);
}

unsigned long __cmr_thread_self(void)
{
	return (unsigned long)pthread_self();
}


static unsigned long g_seq = 0;
unsigned long gen_unique_id()
{
	struct timeval t;
	unsigned long id;
	unsigned long seq = ++g_seq;
	gettimeofday(&t,NULL);
	id = (t.tv_sec * 1000 * 1000) + (t.tv_usec * 1000) << 42;
	id |= (seq % 16777216) << 24;
	return id;
}

