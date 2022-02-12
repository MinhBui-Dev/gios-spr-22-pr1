#include "gfserver-student.h"
#include "gfserver.h"
#include "content.h"
#include "workload.h"
#include "stdlib.h"
#include "pthread.h"
#include "steque.h"

extern pthread_cond_t work_cond;
extern pthread_mutex_t mutex;
extern steque_t *requestQueue;

//
//  The purpose of this function is to handle a get request
//
//  The ctx is a pointer to the "context" operation and it contains connection state
//  The path is the path being retrieved
//  The arg allows the registration of context that is passed into this routine.
//  Note: you don't need to use arg. The test code uses it in some cases, but
//        not in others.
//
gfh_error_t gfs_handler(gfcontext_t **ctx, const char *path, void* arg){
	file_request *request;
	if ((file_request*)malloc(sizeof(file_request)) == NULL)
	{
		printf("Failed to allocate memory for job request");
		return gfh_failure;
	}
	if (*ctx == NULL)
	{
		return gfh_failure;
	}	
	request->ctx = *ctx;
	request->path = path;
	pthread_mutex_lock(&mutex);
	steque_enqueue(requestQueue,request);
	pthread_mutex_unlock(&mutex);
	pthread_cond_signal(&work_cond);
	*ctx = NULL;
	return gfh_success;
}

