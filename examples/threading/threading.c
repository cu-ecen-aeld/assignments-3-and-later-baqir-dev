#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    thread_func_args->thread_complete_success= true;
    if(thread_func_args->wait_to_obtain_ms > 0)
    {
        sleep(thread_func_args->wait_to_obtain_ms/1000);
    }

    if(thread_func_args->mutex != NULL) {
        int rc = pthread_mutex_lock(thread_func_args->mutex);
        if (rc != 0) {
            ERROR_LOG("pthread_mutex_lock failed with %d\n", rc);
            thread_func_args->thread_complete_success = false;
        } else {
            if (thread_func_args->wait_to_release_ms > 0) {
                sleep(thread_func_args->wait_to_release_ms / 1000);
            }

            rc = pthread_mutex_unlock(thread_func_args->mutex);
            if (rc != 0) {
                ERROR_LOG("pthread_mutex_unlock failed with %d\n", rc);
                thread_func_args->thread_complete_success = false;
            }
        }
    }
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    bool success = false;
    struct thread_data* pothread_data = malloc(sizeof(struct thread_data));
    memset(pothread_data, 0, sizeof(struct thread_data));

    pothread_data->mutex = mutex;
    pothread_data->wait_to_obtain_ms = wait_to_obtain_ms;
    pothread_data->wait_to_release_ms = wait_to_release_ms;
    pthread_t tid;

    int rc = pthread_create(&tid, NULL, threadfunc, pothread_data);
    if(rc != 0){
        ERROR_LOG("pthread_create failed with %d\n", rc);
    } else {
        *thread = tid;
        success = true;
	DEBUG_LOG("thread created %d\n", tid);
    }


    return success;	
}

