#include "iot_app.h"
#include "db_helper.h"
#include <MQTTClient.h>
#include <cjson/cJSON.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h> // สำหรับการจัดการเวลา

const char MQTT_BROKER[] = "tcp://broker.emqx.io:1883";
const char MQTT_CLIENTID[] = "TGR2024_MQTT_CLIENT";
const char DB_NAME[] = "/home/wupi/Desktop/workshop/project/data.db";
const char PUBLISH_TOPIC[] = "data/query_result";

void *mqtt_thr_fcn(void *ptr)
{
    char *topic = (char *)ptr;
    int rc;

    printf("Starting MQTT thread\n");
    MQTTClient mqtt_client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_create(&mqtt_client, MQTT_BROKER, MQTT_CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    if ((rc = MQTTClient_connect(mqtt_client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    MQTTClient_subscribe(mqtt_client, topic, 0);
    printf("Subscribed to topic: %s\n", topic);

    while (1)
    {
        char *message = NULL;
        int message_len;
        MQTTClient_message *received_message = NULL;

        // Wait for message from MQTT
        MQTTClient_receive(mqtt_client, &topic, &message_len, &received_message, 1000);

        if (received_message != NULL)
        {
            message = (char *)received_message->payload;
            printf("Received JSON message from MQTT: %s\n", message);

            // Parse JSON payload
            cJSON *json = cJSON_Parse(message);
            if (json == NULL)
            {
                fprintf(stderr, "Error parsing JSON: Received non-JSON message - %s\n", message);
                printf("Raw MQTT message received: %s\n", message);
                // ข้ามไปยังข้อความถัดไปเพื่อหลีกเลี่ยงการประมวลผลข้อมูลที่ไม่ถูกต้อง
                continue;
            }
            else
            {
                // Extract command
                cJSON *check = cJSON_GetObjectItem(json, "check");
                if (check && cJSON_IsString(check))
                {
                    if (strcmp(check->valuestring, "ADD") == 0)
                    {
                        // Handle "ADD" command
                        cJSON *value = cJSON_GetObjectItem(json, "value");
                        if (value && cJSON_IsNumber(value))
                        {
                            printf("Adding value: %d to database\n", value->valueint);
                            dbase_append(DB_NAME, value->valueint); // เรียกใช้ dbase_append

                            // เพิ่ม timestamp ใน JSON payload
                            time_t now = time(NULL);
                            struct tm *t = localtime(&now);
                            char timestamp[20];
                            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
                            cJSON_AddStringToObject(json, "timestamp", timestamp);

                            // แปลง JSON เป็นสตริงเพื่อส่งไปยัง Firebase
                            char *json_with_timestamp = cJSON_Print(json);

                            // ส่งสัญญาณให้ rest_thr_fcn พร้อมข้อมูลใหม่
                            pthread_mutex_lock(&data_cond_mutex);
                            json_payload = strdup(json_with_timestamp); // คัดลอก JSON Payload ที่มี timestamp
                            pthread_cond_signal(&data_cond);            // ส่งสัญญาณให้ REST thread
                            pthread_mutex_unlock(&data_cond_mutex);

                            free(json_with_timestamp); // ปล่อยหน่วยความจำที่ใช้กับ JSON string
                        }
                    }
                    else if (strcmp(check->valuestring, "QUERY") == 0)
                    {
                        int last_value = dbase_query(DB_NAME);

                        cJSON *response_json = cJSON_CreateObject();
                        if (response_json)
                        {
                            cJSON_AddNumberToObject(response_json, "last_value", last_value);
                            char *response_payload = cJSON_Print(response_json);

                            if (response_payload)
                            {
                                MQTTClient_message pubmsg = MQTTClient_message_initializer;
                                pubmsg.payload = response_payload;
                                pubmsg.payloadlen = strlen(response_payload);
                                pubmsg.qos = 1;
                                pubmsg.retained = 0;
                                MQTTClient_publishMessage(mqtt_client, PUBLISH_TOPIC, &pubmsg, NULL);

                                printf("Published queried last value: %d to topic: %s\n", last_value, PUBLISH_TOPIC);
                                free(response_payload); // คืนค่าหน่วยความจำที่ใช้กับ response_payload
                            }

                            cJSON_Delete(response_json); // คืนค่าหน่วยความจำที่ใช้กับ response_json
                        }
                    }
                    // คุณสามารถเพิ่มส่วนของคำสั่งอื่นๆ เช่น "QUERY" ที่นี่ถ้าต้องการ
                }
                cJSON_Delete(json);
            }

            // Free MQTT message
            MQTTClient_freeMessage(&received_message);
        }
    }

    MQTTClient_destroy(&mqtt_client);
    return NULL;
}
