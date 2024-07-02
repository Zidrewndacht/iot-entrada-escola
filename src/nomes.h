#ifndef NOMES_H_ERJMFASKDLKARIGJAOE_     // Evita reinclusão do arquivo de headers
#define NOMES_H_ERJMFASKDLKARIGJAOE_

/** Credenciais WiFi, temporariamente aqui até desenvolver método de ajuste remoto  */
#define STA_SSID "********************"
#define STA_PASS "********************"

/*========================== Includes Bibliotecas Padrão C ==========================*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>         // usado para va_list

/*=========================== Includes Arduino ======================================*/
#include <Arduino.h>               
#include <MFRC522.h>
#include <U8g2lib.h>

/*============================ Includes ESP-IDF =====================================*/
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h" 
#include "cJSON.h"

/** Recompilado manualmente pois requer configuração em menuconfig para aumentar tempo limite de mensagem na caixa de saída (padrão 30s?!)
    #define CONFIG_MQTT_USE_CUSTOM_CONFIG 1
    #define CONFIG_MQTT_REPORT_DELETED_MESSAGES 1
    #define CONFIG_MQTT_OUTBOX_EXPIRED_TIMEOUT_MS (24*60*60*1000) */
#include "esp-mqtt/mqtt_client.h"

#include <lwip/sockets.h>     //usados para requisição HTTP feita para exibição de IP remoto.
#include <lwip/netdb.h>

/*============================ RFID, MQTT, HWIO =================================== */

/** https://www.youtube.com/watch?v=LY-1DHTxRAk
Pinos Recomendados para uso geral:
4, 5, 16, 17, 18, 19, 23, 25, 26, 27, 32, 33, 
ADC2 não funciona com WiFi!
34, 35, VP, VN são apenas entrada! Podem ser usados para ADC.
21, 22: I2C
*/

/** Pinos em uso:
 * RC522 SDA:        5
 * RC522 SCLK:      18
 * RC522 MISO:      19
 * RC522 MOSI:      23
 * RC522 RST:        0
 * BUZZER:          32
 * LED RGB:         25, 26, 27
*/

#define SS_PIN GPIO_NUM_5    //RC522 SDA
#define RST_PIN GPIO_NUM_0   //RC522 RST

#define BUZZER_GPIO GPIO_NUM_32
#define RED_LED_GPIO GPIO_NUM_25
#define GREEN_LED_GPIO GPIO_NUM_26
#define BLUE_LED_GPIO GPIO_NUM_27

void mifare_setup();
int ler_mifare();
bool gravar_mifare();

void hwio_setup();
void u8g2_setup();

void beep(int count, int length);   //usado por main e MQTT
void status_LED(bool red, bool green, bool blue, int duration);

void MQTT_setup();
int MQTT_publish(const char* topic, String payload , boolean retained, uint8_t qos);
extern bool awaitingNewCard;    //usado por RFID e MQTT

/** Formato das variáveis globais: */
union mifare_data {                    //pode ser necessário ampliar para armazenar telefone e e-mail posteriormente.
    char    readFS[128];
    uint8_t writeFS[128];
    uint8_t mifare_bloco[8][16];       //8 blocos de 16 bytes cada.
    struct {
        char matricula_aluno[16];
        char turma_aluno[16];
        char nome_aluno[80];
        char tag_id[16];
    };
};  extern union mifare_data mifare_data;

typedef enum {
    NTP_NOT_SYNCED,
    NTP_AWAITING,
    NTP_UPDATED,
} NTP_status;

typedef enum {
    WIFI_OFFLINE,
    WIFI_IP4_OK,
    WIFI_IP6_OK,
} WIFI_status;

struct global_status {
    struct {
        WIFI_status wifi_status;
        String ipv4_address;
        String ipv6_link_local_address;
        String ipv6_global_address;
        // char ipv6_ext_address[128];
        NTP_status ntp_status; 
    } wifi;
    struct {
        bool connected;
        int code;
        int pending;   //contagem de mensagens pendentes no buffer. Usar ++ e--=
        int reconnect_retries;
    } mqtt;
    struct {
        bool init;
        int read_status;
        bool awaitingNewCard;
    } rfid;
    struct {
        bool red;
        bool green;
        bool blue;
    } rgb;
}; extern struct global_status global_status;

#endif  //Fim da proteção contra reinclusão.