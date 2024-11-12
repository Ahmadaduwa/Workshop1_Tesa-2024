#ifndef IOT_APP_H
#define IOT_APP_H

#include <pthread.h>

// Shared variables for synchronization
extern pthread_mutex_t data_cond_mutex;
extern pthread_cond_t data_cond;
extern char *json_payload;

// Function prototypes
void *rest_thr_fcn(void *ptr);
void *mqtt_thr_fcn(void *ptr);
    
#endif // IOT_APP_H
