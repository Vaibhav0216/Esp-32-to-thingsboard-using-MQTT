#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "esp_random.h"



// Replace with your network credentials
#define WIFI_SSID "AIRTEL_E5172_CF61"
#define WIFI_PASS "E730F9417D0"

// MQTT Broker URI for secure connection
#define MQTT_BROKER_URI "mqtts://thingsboard.cloud:8883"
#define ACCESS_TOKEN "7rYAbn0Rp0T6tAdQ1fXe"

static const char *TAG = "MQTT_EXAMPLE";

static esp_mqtt_client_handle_t client;

static const char mqtt_eclipse_org_pem_start[] = R"EOF(
-----BEGIN CERTIFICATE-----
MIIELzCCAxegAwIBAgIUOf/DU37zRr1vCaN38zIDI9moXhQwDQYJKoZIhvcNAQEL
BQAwgaYxCzAJBgNVBAYTAklOMRMwEQYDVQQIDApNYWhhcmFzdHJhMQ0wCwYDVQQH
DARQdW5lMQ4wDAYDVQQKDAVBbXNldDEjMCEGA1UECwwaIlJlc2VhcmNoIGFuZCBE
ZXZlbG9wbWVudCIxEDAOBgNVBAMMB1ZhaWJoYXYxLDAqBgkqhkiG9w0BCQEWHW1h
dWxpa3NoaXJzYWdhcnZrMThAZ21haWwuY29tMB4XDTI0MDcyNjA1MjM1MVoXDTI5
MDgwMjA1MjM1MVowgaYxCzAJBgNVBAYTAklOMRMwEQYDVQQIDApNYWhhcmFzdHJh
MQ0wCwYDVQQHDARQdW5lMQ4wDAYDVQQKDAVBbXNldDEjMCEGA1UECwwaIlJlc2Vh
cmNoIGFuZCBEZXZlbG9wbWVudCIxEDAOBgNVBAMMB1ZhaWJoYXYxLDAqBgkqhkiG
9w0BCQEWHW1hdWxpa3NoaXJzYWdhcnZrMThAZ21haWwuY29tMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA5S7ubEu8VVP6fGBIoV+ICood1RD+9Jp2IiW3
jDDJ7s3nGZL6muFnmtH25L4LVi2VmQnxmS0zwhiiRlJl0yH0Si11/bNBIGwstEOr
oXLie/jfMguH7at2SKUbNJu2xn7gSfB8BnUBetxfCNmR0tT/6ok0e7QsSUYcGdcU
QmU6u0wgtWt4r+ehr1FHdhN8GO8e4UsLXaahs5BH+sOUiPQvTGAWDf0SueRRglpS
y6pNUlZ2dhr0ZTOCX9xFs+DxEkTcp52AYV4RoZCpy7Ew4UigdT1NePLL2pZV/cup
uV3GjtFZDkIYO5YPLDAiEe+BIBiQ7X5vPX3OjVwWCJjTu5WUHwIDAQABo1MwUTAd
BgNVHQ4EFgQU6DZUpp0spihMDv9TZXnGxT3pmRMwHwYDVR0jBBgwFoAU6DZUpp0s
pihMDv9TZXnGxT3pmRMwDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOC
AQEAMvtkeGnPTErUj71GZ2cCCX8Uj68DzggVoJmvDe2mDjaunsw35v6QVzRgjno8
iZv52X5Vi07MBwXkwwSo0G4DXbvV87h5XH214gBwnXhz+pG8J7yPt9B0DXnY+kp1
vtvgokEMNTS8xYlulsGHAgZdKu0IyxSKvQKzu2vxI1KbK0deMr6OGz6iB6Bi+LfZ
Fqsq1yenTz5ZUBAeb5Fbl6m3atEgX6aFpv5kMRWfgjvd1FNbl3bNv0N72YehOJl5
yRjEtxxVH1pKtyFDxP03ICsevCZ3cKajIedX9CulHyG4gc7JWP59D4ZkiBxX7Le5
0XbVWZ2rs3HQxPjks3HqWV1ehA==
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----)EOF";

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            esp_mqtt_client_subscribe(client, "v1/devices/me/rpc/request/+", 0);
            esp_mqtt_client_subscribe(client, "v1/devices/me/attributes", 0);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED");
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED");
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);

            if (strncmp(event->topic, "v1/devices/me/rpc/request/", strlen("v1/devices/me/rpc/request/")) == 0) {
                printf("Data received from RPC request topic.\r\n");
            } else if (strncmp(event->topic, "v1/devices/me/attributes", strlen("v1/devices/me/attributes")) == 0) {
                printf("Data received from attributes topic.\r\n");
            } else if (strncmp(event->topic, "v1/devices/me/attributes/response", event->topic_len) == 0)  {
                printf("Data received from shared attributes topic.\r\n");
            } else {
                printf("Data received from unknown topic.\r\n");
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id: %" PRId32, event_id);
            break;
    }
}

void wifi_init_sta(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

void app_main(void) {
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    nvs_flash_init();
    wifi_init_sta();

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
          .address.uri = MQTT_BROKER_URI,
          .verification.certificate = (const char *)mqtt_eclipse_org_pem_start
        },
        .credentials = {
            .username = ACCESS_TOKEN
            // .authentication.password = ""
        },
    };


    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    while (1) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "temperature", esp_random() % 70 + 20);
        cJSON_AddNumberToObject(root, "humidity", esp_random() % 6 + 36);
        char *json_string = cJSON_Print(root);
        esp_mqtt_client_publish(client, "v1/devices/me/telemetry", json_string, 0, 1, 0);
        cJSON_Delete(root);
        free(json_string);

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
