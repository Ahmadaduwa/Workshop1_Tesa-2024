#include "iot_app.h"

// Define global variables
pthread_mutex_t data_cond_mutex;
pthread_cond_t data_cond;
char *json_payload = NULL; // Initialize to NULL to ensure no memory is allocated initially

int main(int argc, char *argv[]) {
    const char db_name[] = "data.db";
    pthread_t rest_thr, mqtt_thr;
    int ret;

    if (argc < 3) {
        printf("Usage: iot_app REST_ENDPOINT MQTT_TOPIC\n");
        exit(EXIT_FAILURE);
    }

    // Initialize database and create table if it does not exist
    dbase_init(db_name);

    // Prepare IPC objects
    if (pthread_mutex_init(&data_cond_mutex, NULL) != 0) {
        fprintf(stderr, "Error initializing mutex\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_init(&data_cond, NULL) != 0) {
        fprintf(stderr, "Error initializing condition variable\n");
        pthread_mutex_destroy(&data_cond_mutex);
        exit(EXIT_FAILURE);
    }

    // Initialize threads
    ret = pthread_create(&rest_thr, NULL, rest_thr_fcn, (void*)argv[1]);
    if (ret != 0) {
        fprintf(stderr, "Error creating REST thread\n");
        pthread_mutex_destroy(&data_cond_mutex);
        pthread_cond_destroy(&data_cond);
        exit(EXIT_FAILURE);
    }

    ret = pthread_create(&mqtt_thr, NULL, mqtt_thr_fcn, (void*)argv[2]);
    if (ret != 0) {
        fprintf(stderr, "Error creating MQTT thread\n");
        pthread_cancel(rest_thr); // Cancel the REST thread if MQTT thread creation fails
        pthread_mutex_destroy(&data_cond_mutex);
        pthread_cond_destroy(&data_cond);
        exit(EXIT_FAILURE);
    }

    // Waiting for all threads to terminate
    pthread_join(rest_thr, NULL);
    pthread_join(mqtt_thr, NULL);

    // Clean up
    pthread_mutex_destroy(&data_cond_mutex);
    pthread_cond_destroy(&data_cond);
    if (json_payload != NULL) {
        free(json_payload);
    }

    return 0;
}
