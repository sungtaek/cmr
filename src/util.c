#include <stdio.h>
#include <search.h>

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


