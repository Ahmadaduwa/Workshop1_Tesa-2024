#include "iot_app.h"
#include "db_helper.h"
#include <stdio.h>      // เพิ่มเพื่อใช้ printf และ fprintf
#include <stdlib.h>     // เพิ่มเพื่อใช้ exit, EXIT_FAILURE และ free
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t data_cond_mutex;
pthread_cond_t data_cond;
char *json_payload = NULL;

int main(int argc, char *argv[]) {
    const char db_name[] = "/home/wupi/Desktop/workshop/project/data.db";
    pthread_t rest_thr, mqtt_thr;
    int ret;

    if (argc < 3) {
        printf("Usage: iot_app REST_ENDPOINT MQTT_TOPIC\n");
        exit(EXIT_FAILURE);
    }

    dbase_init(db_name);

    if (pthread_mutex_init(&data_cond_mutex, NULL) != 0) {
        fprintf(stderr, "Error initializing mutex\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_init(&data_cond, NULL) != 0) {
        fprintf(stderr, "Error initializing condition variable\n");
        pthread_mutex_destroy(&data_cond_mutex);
        exit(EXIT_FAILURE);
    }

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
        pthread_cancel(rest_thr);
        pthread_mutex_destroy(&data_cond_mutex);
        pthread_cond_destroy(&data_cond);
        exit(EXIT_FAILURE);
    }

    pthread_join(rest_thr, NULL);
    pthread_join(mqtt_thr, NULL);

    pthread_mutex_destroy(&data_cond_mutex);
    pthread_cond_destroy(&data_cond);
    if (json_payload != NULL) {
        free(json_payload);
    }

    return 0;
}
