#include "iot_app.h"
#include <curl/curl.h>
#include <string.h>
#include <cjson/cJSON.h>

// Private constants
const char base_url[] = "https://tgr2024-d1a5b-default-rtdb.asia-southeast1.firebasedatabase.app/%s.json";

// ตัวแปร json payload ที่จะรับจาก MQTT
extern char *json_payload;

void *rest_thr_fcn(void *ptr) {
    CURL *curl_handle;
    CURLcode res;
    char url[150];

    printf("Starting REST thread\n");

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_handle = curl_easy_init();
    if (!curl_handle) {
        fprintf(stderr, "Error initializing curl\n");
        return NULL;
    }

    // สร้าง URL เต็มโดยใส่ endpoint จาก topic
    snprintf(url, sizeof(url), base_url, (char *)ptr);
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 1);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 1);
    curl_easy_setopt(curl_handle, CURLOPT_CAINFO, "/home/wupi/Desktop/workshop/project/GTS Root R1.crt");

    while (1) {
        // รอรับสัญญาณข้อมูลใหม่จาก mqtt_thr_fcn
        pthread_mutex_lock(&data_cond_mutex);
        pthread_cond_wait(&data_cond, &data_cond_mutex);

        if (json_payload != NULL) {
            // แสดงข้อมูล JSON ที่จะส่งใน Terminal
            printf("Sending JSON Payload to Firebase: %s\n", json_payload);

            // ส่ง JSON Payload ไปยัง Firebase
            curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, json_payload);
            res = curl_easy_perform(curl_handle);
            if (res != CURLE_OK) {
                fprintf(stderr, "Failed to send to Firebase: %s\n", curl_easy_strerror(res));
            } else {
                printf("Data sent successfully to Firebase\n");
            }

            free(json_payload);  // ปล่อยหน่วยความจำของ JSON Payload ที่ส่ง
            json_payload = NULL;
        }

        pthread_mutex_unlock(&data_cond_mutex);
        sleep(3);  // ช่วงเวลาระหว่างการส่งข้อมูล
    }

    curl_easy_cleanup(curl_handle);
    return NULL;
}
