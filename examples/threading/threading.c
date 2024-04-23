#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    // NOTE: could use nanosleep here but that requires using timespec
    struct thread_data *data = (struct thread_data *) thread_param;

    // first we wait and return if command fails
    int rc = usleep(data->wait_to_obtain_ms * 1000);
    if (rc != 0) {
        ERROR_LOG("Failed to sleep.");
        data->thread_complete_success = false;
        return thread_param;
    }

    // if we lock successfully, sleep and wait to unlock
    if (pthread_mutex_lock(data->mutex) == 0) {
        // sleep and return if command fails
        rc = usleep(data->wait_to_release_ms * 1000);
        if (rc != 0) {
            ERROR_LOG("Failed to sleep.");
            data->thread_complete_success = false;
            return thread_param;
        }

        // unlock, return if unsuccessful
        rc = pthread_mutex_unlock(data->mutex);
        if (rc == 0) {
            data->thread_complete_success = true;
        } else {
            ERROR_LOG("Failed to mutex unlock.");
            data->thread_complete_success = false;
            return thread_param;
        }
    } else {
        ERROR_LOG("Failed to obtain mutex lock.");
        data->thread_complete_success = false;
        return thread_param;
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
    struct thread_data *data = (struct thread_data *)malloc(sizeof(struct thread_data));

    if (data == NULL) {
        return false;
    }

    // initialize the thread structure
    data->mutex = mutex;
    data->wait_to_obtain_ms = wait_to_obtain_ms;
    data->wait_to_release_ms = wait_to_release_ms;

    // create the thread
    int result = pthread_create(thread,
                                NULL,
                                threadfunc,
                                (void *)data);

    // if the thread creation fails, free the thread structure and return false
    if (result != 0) {
        free(data);
        return false;
    }
    return true;
}

