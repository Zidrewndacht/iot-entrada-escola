#include "nomes.h"

/** Feedback:
 * Boot:
 *      vermelho sólido: Init, conclui com 1 bipe curto.
 *      vermelho lento: Conectando WiFi; conclui com 2 bipes curtos
 *      vermelho rápido: Aguardando NTP; conclui com 3 bipes curtos
 *      apagado:	OK
 * Leitura RFID:
 *      Verde: Lido, 1 bipe médio, envia via MQTT.
 *      Vermelho: Ignorado, 2 (falha RFID) ou 3 (falha NTP) bipes médios, não envia via MQTT;
 * Gravação RFID:
 *      Azul: Piscando enquando aguarda gravação, 4 bipes curtos;
 *      verde quando gravado, um bipe longo.
 * MQTT:
 *      atualmente sem feedback. 
*/

TaskHandle_t  ntp_keep = NULL, rfid_client = NULL;

/** *************************** RFID genérico *********************************/

void rfid_client_task(void *pvParameters){
    mifare_setup(); //inicializa leitor MFRC522, em rfid.cpp
    while(1) {         
        if (global_status.rfid.awaitingNewCard) {
            int msg_id = MQTT_publish("cad_aguardando", "OK", false, 1); 
            Serial.print("\n [RFID vTask] Aguardando gravação de novo cartão MIFARE com os dados a seguir:");
            Serial.printf("\n  Mat: \t\t%s\n  Nome: \t%s\n  Turma: \t%s\n  Tag ID: \t%s\n",
                mifare_data.matricula_aluno,mifare_data.nome_aluno,mifare_data.turma_aluno,mifare_data.tag_id);

            if (!gravar_mifare()) {
                global_status.rfid.awaitingNewCard = 0;         //remove flag apenas após sucesso
                MQTT_publish("cad_gravado", "OK",  false, 2);   //sem retenção //informa gravação concluída somente se flag foi removida;
                MQTT_publish("cad_novo", "", true, 2);          //remove cad_novo anterior retido, precisa ter retenção;
                beep(1, 750);
                status_LED(0,1,0,2000);
                continue;
            }
            beep(2, 50);
            status_LED(0,0,1,1000);
		    vTaskDelay(3000 / portTICK_PERIOD_MS);  //...enquanto não gravar ou cancelar, tenta novamente a cada 3 segundos
        } else {
            global_status.rfid.read_status = ler_mifare();  // 0 para nenhum cartão, -1 para falha, 1 para sucesso.
            if (global_status.rfid.read_status == 1){       //se cartão lido com sucesso, monte JSON e envie via MQTT:
                if (time(NULL) < 1609459200) {              //se relógio incorreto, ignore cartão, não sabemos o horário de passagem.
                    beep(3, 60);
                    status_LED(1,0,0,2000);

                    Serial.print("\n [RFID vTask] Aguardando NTP antes de registrar leituras.");
                    continue;   //return provavelmente mataria a vTask.
                }
                beep(1, 90);
                status_LED(0,1,0,2000);

                cJSON *root;
                char *json;
                time_t now;  time(&now);

                root = cJSON_CreateObject();
                cJSON_AddNumberToObject(root, "timestamp",  (double)now);
                cJSON_AddStringToObject(root, "Matr",       mifare_data.matricula_aluno);
                cJSON_AddStringToObject(root, "Turma",      mifare_data.turma_aluno);
                cJSON_AddStringToObject(root, "Aluno",      mifare_data.nome_aluno);
                cJSON_AddStringToObject(root, "Tag_ID",     mifare_data.tag_id);

                json = cJSON_Print(root);
                
                char topic[64];
                sprintf(topic, "rfid_lido/%s", mifare_data.matricula_aluno);
                int msg_id = MQTT_publish(topic, json, false, 2); //sem retenção, individual por estudante.

                cJSON_Delete(root);
                free(json);
            } else if (global_status.rfid.read_status == -1){ //se falha de leitura, 2 bipes.
                beep(2, 60);
                status_LED(1,0,0,2000);
                //feedback serial é provido diretamente pelo acesso a hardware em rfid.cpp.
            }  
		    vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}

void ntp_keep_task(void *pvParameters){ 
    // 1/1/2021, 0h. Se o horário atual for antes disso, nunca foi sincronizado:
    if (time(NULL) < 1609459200 ) global_status.wifi.ntp_status = NTP_NOT_SYNCED;   
    wifi_ap_record_t ap_info;
    while (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {  // pisca rápido enquanto aguarda WiFi:
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        status_LED(1,0,0,500);
    }
    vTaskDelay(1500 / portTICK_PERIOD_MS); //aguarda mais 1,5s adicionais

    beep(2, 5);    //Pronto para acessar NTP
    global_status.wifi.ntp_status = NTP_AWAITING;
    
	while(1) {
        Serial.print(F("\n [NTP vTask] Solicitando data e hora. "));
        configTime(-3 * 3600, 0, "pool.ntp.br", "time.nist.gov");
        while (time(NULL) < 1609459200 ){           // 1/1/2021, 0h. Se o horário atual for antes disso, está errado.
            vTaskDelay(300 / portTICK_PERIOD_MS);
            status_LED(1,0,0,150);
        }
        time_t agora = time(NULL);  
        Serial.printf("\n [NTP vTask] Data e hora obtidas via NTP: %s", ctime(&agora));
        
        if(global_status.wifi.ntp_status != NTP_UPDATED) beep(3, 5);  //bipe apenas na primeira atualização.
        status_LED(0,0,0,1);    //apaga LEDs de status de boot.
        global_status.wifi.ntp_status = NTP_UPDATED;

        vTaskDelay(7200000 / portTICK_PERIOD_MS);   //atualiza novamente em duas horas.
	}
}

void WiFi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    esp_netif_t *netif = (esp_netif_t*) arg;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        global_status.wifi.wifi_status = WIFI_OFFLINE;
        Serial.print("\n [WiFi Event]  Desconectado da WiFi. Tentando reconectar.");
        global_status.wifi.ipv4_address = "";
        global_status.wifi.ipv6_link_local_address = "";
        global_status.wifi.ipv6_global_address = "";
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {     //recebido IPv4:
        global_status.wifi.wifi_status = WIFI_IP4_OK;
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        global_status.wifi.ipv4_address = ip4addr_ntoa((ip4_addr_t*)&event->ip_info.ip);
        Serial.printf("\n [WiFi Event] Obtido IPv4: %s\n", global_status.wifi.ipv4_address.c_str());
        esp_netif_create_ip6_linklocal(netif); // Changed this line
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_GOT_IP6) {     //recebido IPv6:
        global_status.wifi.wifi_status = WIFI_IP6_OK;
        ip_event_got_ip6_t* event = (ip_event_got_ip6_t*) event_data;
        if (ip6_addr_isglobal((ip6_addr_t*)&event->ip6_info.ip)) {           //Verifica se endereço é global:
            global_status.wifi.ipv6_global_address = ip6addr_ntoa((ip6_addr_t*)&event->ip6_info.ip);
            Serial.printf("\n [WiFi Event] Obtido IPv6 global: %s\n", global_status.wifi.ipv6_global_address.c_str());
        } else {
            global_status.wifi.ipv6_link_local_address = ip6addr_ntoa((ip6_addr_t*)&event->ip6_info.ip);
            Serial.printf("\n [WiFi Event] Obtido IPv6 link-local: %s\n", global_status.wifi.ipv6_link_local_address.c_str());
        }
    }
}
void WiFi_setup() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,    &WiFi_event_handler, sta_netif);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,   &WiFi_event_handler, sta_netif);
    esp_event_handler_register(IP_EVENT, IP_EVENT_GOT_IP6,      &WiFi_event_handler, sta_netif);
    esp_wifi_set_mode(WIFI_MODE_STA);
    wifi_config_t sta_config = {};
    strcpy((char*)sta_config.sta.ssid,      STA_SSID);
    strcpy((char*)sta_config.sta.password,  STA_PASS);
    esp_wifi_set_config(WIFI_IF_STA, &sta_config);
    esp_wifi_start();
}

void setup() {  
    u8g2_setup();
    hwio_setup();
    Serial.begin(115200); 
    Serial.print("\n [Init] Iniciando."); Serial.printf(" RAM livre: %" PRIu32 " bytes", esp_get_free_heap_size());
    SPI.begin();      //necessário para MIFARE. Também para LittleFS posteriormente?

    //inits abaixo aparentemente requeridos para ESP-IDF MQTT, conforme exemplo.
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    WiFi_setup();
    MQTT_setup();   //em mqtt.cpp, assíncrona
    
    xTaskCreatePinnedToCore( ntp_keep_task, 
        "NTP_client_task",  2048,     NULL, 
        1,                 &ntp_keep,   0);

    xTaskCreatePinnedToCore( rfid_client_task, 
        "RFID_client_task",  4096,     NULL, 
        6,                 &rfid_client,   1);
    //  ^ Prioridade acima de ESP-MQTT (5)

    Serial.print("\n [Init] Setup OK.");
} 

void loop() { vTaskSuspend(NULL); } //Função apenas requerida pelo framework Arduino, não será utilizada.
