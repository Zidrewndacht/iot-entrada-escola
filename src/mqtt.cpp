#include "nomes.h"

TaskHandle_t  mqtt_setup_task_handle = NULL;

/*************************************** [ESP-IDF MQTT] **************************************/
esp_mqtt_client_handle_t client; 
String last_processed_uid = "";     //para verificação de gravação já executada;

const char* mqtt_uri =      "mqtts://********************";
const char* mqtt_username = "********************";
const char* mqtt_password = "********************";

/** zidrewndacht.freeddns.org ca_bundle do ZeroSSL: */
static const char *root_ca = R"EOF( 
-----BEGIN CERTIFICATE-----
MIIG1TCCBL2gAwIBAgIQbFWr29AHksedBwzYEZ7WvzANBgkqhkiG9w0BAQwFADCB
iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl
cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV
BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMjAw
MTMwMDAwMDAwWhcNMzAwMTI5MjM1OTU5WjBLMQswCQYDVQQGEwJBVDEQMA4GA1UE
ChMHWmVyb1NTTDEqMCgGA1UEAxMhWmVyb1NTTCBSU0EgRG9tYWluIFNlY3VyZSBT
aXRlIENBMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAhmlzfqO1Mdgj
4W3dpBPTVBX1AuvcAyG1fl0dUnw/MeueCWzRWTheZ35LVo91kLI3DDVaZKW+TBAs
JBjEbYmMwcWSTWYCg5334SF0+ctDAsFxsX+rTDh9kSrG/4mp6OShubLaEIUJiZo4
t873TuSd0Wj5DWt3DtpAG8T35l/v+xrN8ub8PSSoX5Vkgw+jWf4KQtNvUFLDq8mF
WhUnPL6jHAADXpvs4lTNYwOtx9yQtbpxwSt7QJY1+ICrmRJB6BuKRt/jfDJF9Jsc
RQVlHIxQdKAJl7oaVnXgDkqtk2qddd3kCDXd74gv813G91z7CjsGyJ93oJIlNS3U
gFbD6V54JMgZ3rSmotYbz98oZxX7MKbtCm1aJ/q+hTv2YK1yMxrnfcieKmOYBbFD
hnW5O6RMA703dBK92j6XRN2EttLkQuujZgy+jXRKtaWMIlkNkWJmOiHmErQngHvt
iNkIcjJumq1ddFX4iaTI40a6zgvIBtxFeDs2RfcaH73er7ctNUUqgQT5rFgJhMmF
x76rQgB5OZUkodb5k2ex7P+Gu4J86bS15094UuYcV09hVeknmTh5Ex9CBKipLS2W
2wKBakf+aVYnNCU6S0nASqt2xrZpGC1v7v6DhuepyyJtn3qSV2PoBiU5Sql+aARp
wUibQMGm44gjyNDqDlVp+ShLQlUH9x8CAwEAAaOCAXUwggFxMB8GA1UdIwQYMBaA
FFN5v1qqK0rPVIDh2JvAnfKyA2bLMB0GA1UdDgQWBBTI2XhootkZaNU9ct5fCj7c
tYaGpjAOBgNVHQ8BAf8EBAMCAYYwEgYDVR0TAQH/BAgwBgEB/wIBADAdBgNVHSUE
FjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwIgYDVR0gBBswGTANBgsrBgEEAbIxAQIC
TjAIBgZngQwBAgEwUAYDVR0fBEkwRzBFoEOgQYY/aHR0cDovL2NybC51c2VydHJ1
c3QuY29tL1VTRVJUcnVzdFJTQUNlcnRpZmljYXRpb25BdXRob3JpdHkuY3JsMHYG
CCsGAQUFBwEBBGowaDA/BggrBgEFBQcwAoYzaHR0cDovL2NydC51c2VydHJ1c3Qu
Y29tL1VTRVJUcnVzdFJTQUFkZFRydXN0Q0EuY3J0MCUGCCsGAQUFBzABhhlodHRw
Oi8vb2NzcC51c2VydHJ1c3QuY29tMA0GCSqGSIb3DQEBDAUAA4ICAQAVDwoIzQDV
ercT0eYqZjBNJ8VNWwVFlQOtZERqn5iWnEVaLZZdzxlbvz2Fx0ExUNuUEgYkIVM4
YocKkCQ7hO5noicoq/DrEYH5IuNcuW1I8JJZ9DLuB1fYvIHlZ2JG46iNbVKA3ygA
Ez86RvDQlt2C494qqPVItRjrz9YlJEGT0DrttyApq0YLFDzf+Z1pkMhh7c+7fXeJ
qmIhfJpduKc8HEQkYQQShen426S3H0JrIAbKcBCiyYFuOhfyvuwVCFDfFvrjADjd
4jX1uQXd161IyFRbm89s2Oj5oU1wDYz5sx+hoCuh6lSs+/uPuWomIq3y1GDFNafW
+LsHBU16lQo5Q2yh25laQsKRgyPmMpHJ98edm6y2sHUabASmRHxvGiuwwE25aDU0
2SAeepyImJ2CzB80YG7WxlynHqNhpE7xfC7PzQlLgmfEHdU+tHFeQazRQnrFkW2W
kqRGIq7cKRnyypvjPMkjeiV9lRdAM9fSJvsB3svUuu1coIG1xxI1yegoGM4r5QP4
RGIVvYaiI76C0djoSbQ/dkIUUXQuB8AL5jyH34g3BZaaXyvpmnV4ilppMXVAnAYG
ON51WhJ6W0xNdNJwzYASZYH+tmCWI+N60Gv2NNMGHwMZ7e9bXgzUCZH5FaBFDGR5
S9VWqHB73Q+OyIVvIbKYcSc2w/aSuFKGSA==
-----END CERTIFICATE-----
)EOF";
/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void MQTT_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){
    // Serial.printf("\n [ESP-IDF MQTT] Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data); //esp_mqtt_event_handle_t event = event_data; precisa de cast explícito em C++
    client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:{
            global_status.mqtt.connected = true;
            Serial.printf("\n [ESP-IDF MQTT] Conectado a %s como %s", mqtt_uri, mqtt_username);
            esp_mqtt_client_subscribe(client, "cad_novo",     2);  global_status.mqtt.pending++;  
            esp_mqtt_client_subscribe(client, "rfid_cancela", 1);  global_status.mqtt.pending++;
        } break;
        case MQTT_EVENT_DISCONNECTED:{
            global_status.mqtt.connected = false;
            Serial.printf("\n [ESP-IDF MQTT] Desconectado");
        } break;
        case MQTT_EVENT_SUBSCRIBED:{
            Serial.printf("\n [ESP-IDF MQTT] Subscribe concluído, msg_id:%d", event->msg_id);
            global_status.mqtt.pending--;  
        } break;
        case MQTT_EVENT_PUBLISHED:{
            Serial.printf("\n [ESP-IDF MQTT] Publish concluído, msg_id:%d", event->msg_id);  
            global_status.mqtt.pending--;  
        } break;
        case MQTT_EVENT_DATA:{ 
            char* topic = event->topic;
            char* payload = event->data;
            if(strcmp(topic,"cad_novo") == 0){
                cJSON *root = NULL; 
                root = cJSON_Parse(payload);
                if (root == NULL) { 
                    const char *error_ptr = cJSON_GetErrorPtr();
                    if (error_ptr != NULL) {
                        Serial.printf("\n [ESP-IDF MQTT] Falha ao deserializar JSON: %s\n", error_ptr);
                    }
                    cJSON_Delete(root); //necessário remover antes de retornar aqui.
                    return; 
                } else {
                    Serial.printf("\n [ESP-IDF MQTT] Solicitada gravação de novo cartão RFID.\n");
                    cJSON *uid = cJSON_GetObjectItemCaseSensitive(root, "UID");
                    char *uid_str = uid->valuestring;
                    if(strcmp(uid_str, last_processed_uid.c_str()) == 0) return; //Ignorando mensagem duplicada;

                    strncpy(mifare_data.matricula_aluno,    cJSON_GetObjectItemCaseSensitive(root, "Matr") ->   valuestring, sizeof(mifare_data.matricula_aluno));
                    strncpy(mifare_data.turma_aluno,        cJSON_GetObjectItemCaseSensitive(root, "Turma") ->  valuestring, sizeof(mifare_data.turma_aluno));
                    strncpy(mifare_data.nome_aluno,         cJSON_GetObjectItemCaseSensitive(root, "Aluno") ->  valuestring, sizeof(mifare_data.nome_aluno));
                    strncpy(mifare_data.tag_id,             cJSON_GetObjectItemCaseSensitive(root, "Tag_ID") -> valuestring, sizeof(mifare_data.tag_id));

                    global_status.rfid.awaitingNewCard = true; //publish será feito por RFID_task.
                    last_processed_uid = uid_str;
                }
                cJSON_Delete(root);
            } else if(strcmp(topic,"rfid_cancela") == 0){
                if (payload == NULL || payload[0] == '\0') return;  //ignorando mensagem retida anterior após removida
                global_status.rfid.awaitingNewCard = false;         //desiste de aguardar novo cartão;
                Serial.print("\n [ESP-IDF MQTT] Cancelada gravação de novo cartão RFID.");
                MQTT_publish("rfid_cancelado", "OK", false, 1);    //remove rfid_cancela anterior retido, precisa ter retenção;
                MQTT_publish("rfid_cancela", "", true, 1);         //remove rfid_cancela anterior retido, precisa ter retenção;
                MQTT_publish("cad_novo", "", true, 2);             //remove cad_novo anterior retido, precisa ter retenção;
                status_LED(1,0,0,1000);
            }
        } break;
        case MQTT_EVENT_ERROR:{ //código do exemplo:  https://github.com/espressif/esp-idf/blob/v5.1.4/examples/protocols/mqtt/ssl_mutual_auth/main/app_main.c
            Serial.printf("\n [ESP-IDF MQTT] Erro:");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                Serial.printf("\n   Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                Serial.printf("\n   Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
                Serial.printf("\n   Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                        strerror(event->error_handle->esp_transport_sock_errno));
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                Serial.printf("\n   Connection refused error: 0x%x", event->error_handle->connect_return_code);
            } else {
                Serial.printf("\n   Unknown error type: 0x%x", event->error_handle->error_type);
            }
        } break;
        case MQTT_EVENT_DELETED:{
            Serial.printf("\n [ESP-IDF MQTT] Mensagem descartada.");
        }
        default:{
            if (event->event_id == 7) break;    //7 = MQTT_EVENT_BEFORE_CONNECT, desnecessário.
            Serial.printf("\n [ESP-IDF MQTT] Evento desconhecido. ID: %d", event->event_id);
        } break;
    }
}

int MQTT_publish(const char* topic, String payload , boolean retained, uint8_t qos){    //alterar para queue própria?
    if(client == NULL) {
        Serial.printf("\n [MQTT wrapper] Tentativa de publish sem cliente MQTT disponível");
        return -1;
    }
    int msg_id = esp_mqtt_client_enqueue(client, topic, payload.c_str(), payload.length(), qos, retained, true);    //alterado para enqueue, non-blocking.
    Serial.printf("\n [MQTT wrapper] Publish solicitado. msg_id: %d QoS: %d. Tópico %s. Conteúdo:\n\t%s", msg_id , qos, topic, payload.c_str());
    global_status.mqtt.pending++;  

    return msg_id;
}

void MQTT_setup_task(void *pvParameters){ 
    wifi_ap_record_t ap_info;
    while (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    Serial.printf("\n [ESP-IDF MQTT] Oportunizando RA por 5s antes de tentar conexão MQTT IPv6");
    vTaskDelay(5000 / portTICK_PERIOD_MS); //aguarda mais 5s adicionais

    esp_mqtt_client_config_t mqtt_cfg = {};     //necessário pré-inicializar estrutura como vazia explicitamente para ESP-IDF!
    mqtt_cfg.broker.address.uri =                   mqtt_uri;
    // mqtt_cfg.broker.address.port =                  mqtt_port;   //sem necessidade em ESP-IDF quando é porta padrão.
    mqtt_cfg.broker.verification.certificate =      root_ca;        
    mqtt_cfg.credentials.username =                 mqtt_username;
    mqtt_cfg.credentials.authentication.password =  mqtt_password;
    mqtt_cfg.buffer.size =                          512;              //Usado apenas para cad_novo, rfid_cancela
    mqtt_cfg.buffer.out_size =                      512;              //Amplia fila de mensagens de saída, para rfid_lido
    mqtt_cfg.outbox.limit =                         65536;

    Serial.printf("\n [ESP-IDF MQTT] Configurando conexão a %s",mqtt_uri);
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, MQTT_event_handler, NULL);
    esp_mqtt_client_start(client);
    Serial.printf("\n [ESP-IDF MQTT] Cliente MQTT inicializado.");
    vTaskSuspend(NULL);
}

void MQTT_setup(){    
    xTaskCreatePinnedToCore( MQTT_setup_task, 
    "MQTT_setup_task",              10240,     NULL, 
    2,                 &mqtt_setup_task_handle,   0);
}