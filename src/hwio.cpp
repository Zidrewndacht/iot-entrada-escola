#include "nomes.h"

/** *************************** HWIO *********************************/

TaskHandle_t  buzzer_task_handle = NULL, u8g2_task_handle = NULL;
QueueHandle_t beepCommandQueue; //queue funciona melhor para quantidade ajustável de bipes;
TimerHandle_t led_timer;        //timer funciona melhor para troca instantânea de status;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

/** LEDs: */
void led_timer_callback(TimerHandle_t xTimer) {  //apaga todos os LEDs ao expirar o timer:
    gpio_set_level(RED_LED_GPIO,   0);
    gpio_set_level(GREEN_LED_GPIO, 0);
    gpio_set_level(BLUE_LED_GPIO,  0);
    
    global_status.rgb.red = global_status.rgb.green = global_status.rgb.blue = 0;
}
void status_LED(bool red, bool green, bool blue, int duration) {
    global_status.rgb.red   = red;
    global_status.rgb.green = green;
    global_status.rgb.blue  = blue;

    gpio_set_level(RED_LED_GPIO,    red);
    gpio_set_level(GREEN_LED_GPIO,  green);
    gpio_set_level(BLUE_LED_GPIO,   blue);

    xTimerChangePeriod(led_timer, pdMS_TO_TICKS(duration), portMAX_DELAY);
    xTimerStart(led_timer, portMAX_DELAY);
}



/** Bipe: */
typedef struct {
    int beepCount;
    int beepLength;
} BeepCommand;
void buzzer_task(void *pvParameters){
    BeepCommand cmd;
    while(1) {
        if(xQueueReceive(beepCommandQueue, &cmd, portMAX_DELAY) == pdPASS) {
            for(int i = 0; i < cmd.beepCount; i++) {
                gpio_set_level(BUZZER_GPIO, 1);
                vTaskDelay(cmd.beepLength / portTICK_PERIOD_MS);
                gpio_set_level(BUZZER_GPIO, 0);
                if(i < cmd.beepCount - 1) {
                    vTaskDelay( (cmd.beepLength + 60) / portTICK_PERIOD_MS); // Delay between beeps
                }
            }
        }
    }
}
void beep(int count, int length) {
    BeepCommand cmd = {count, length};
    xQueueSend(beepCommandQueue, &cmd, portMAX_DELAY);
}


/** Formata endereço IPv6 adicionando zero paddings
 *  para melhor exibição em display via U8g2. */
String formatIPv6(String ip) {
    String ipv6 = "";
    int start = 0;
    for (int i = 0; i < 8; i++) {
        if (i) ipv6 += ":";
        int end = ip.indexOf(':', start);
        if (end == -1) end = ip.length();
        String segment = ip.substring(start, end);
        while (segment.length() < 4) {
            segment = "0" + segment;
        }
        ipv6 += segment;
        start = end + 1;
    }
    return ipv6;
}

/** Display: */
void u8g2_task (void *pvParameters){
    u8g2.clearBuffer();
    u8g2.drawBox(0,0,128,64);
    u8g2.drawBox(1,1,126,62);
    u8g2.setFont(u8g2_font_unifont_t_symbols);
    u8g2.drawUTF8(15, 26, "©");
    u8g2.setFont(u8g2_font_helvR08_tf ); 
    u8g2.drawStr(25,25, "2024 - Luis Alfredo");
    u8g2.drawStr(29,50, "CC BY-NC 4.0");
    u8g2.sendBuffer();
    vTaskDelay( 4000 / portTICK_PERIOD_MS); //boot continua enquanto exibe tela de copyright.
    while (1){
        u8g2.clearBuffer();
        {   // MQTT status
            char pendingStr[5];

            u8g2.setFont(u8g2_font_helvR08_tf ); 
            u8g2.drawStr(0,64, global_status.mqtt.connected ? "MQTT:  OK" : "MQTT:  Offline");
            u8g2.drawStr(96,64, "Fila:");
            u8g2.setFont(u8g2_font_t0_12_tf ); 
            sprintf(pendingStr, "%2d", global_status.mqtt.pending); 
            u8g2.drawStr(116,64, pendingStr);
        }
        if(global_status.rfid.awaitingNewCard){
            u8g2.drawBox(0,0,128,54);
            u8g2.setFont(u8g2_font_t0_22b_tf); 
            
            u8g2.drawStr(10,20,"Aguardando");
            u8g2.drawStr(18,35,"nova tag");
        } else {    //exibe WiFi, RAM e relógio apenas se não há cartão aguardando:
            //RAM:
            uint32_t freeHeapSize = xPortGetFreeHeapSize();
            char cpuRamStr[20];           
            u8g2.setFont(u8g2_font_helvR08_tf ); 
            sprintf(cpuRamStr, "%uk livre", freeHeapSize/1000);
            u8g2.drawStr(83, 8, cpuRamStr);
            // WiFi status
            u8g2.drawStr(0,8,"WLAN:");
            switch(global_status.wifi.wifi_status) {
                case WIFI_OFFLINE:{
                    u8g2.drawStr(38,8,"Offline");
                } break;
                case WIFI_IP4_OK:{
                    u8g2.setFont(u8g2_font_t0_12_tf  ); 
                    u8g2.drawStr(0,18,global_status.wifi.ipv4_address.c_str());
                    u8g2.setFont(u8g2_font_helvR08_tf ); 
                    u8g2.drawStr(0,28,"Sem IPv6");
                } break;
                case WIFI_IP6_OK:{
                    u8g2.drawStr(38,8,"OK");
                    String ipv6_part1;
                    String ipv6_part2;
                    //mostra IPv6 link-local até receber IPv6 externo/global:
                    if (global_status.wifi.ipv6_global_address[0] == '\0')  {
                        ipv6_part1 = "FE80::";
                        ipv6_part2 = global_status.wifi.ipv6_link_local_address.substring(6);
                    } else {
                        ipv6_part1 = formatIPv6(global_status.wifi.ipv6_global_address).substring(0, 20);
                        ipv6_part2 = formatIPv6(global_status.wifi.ipv6_global_address).substring(20);
                    }
                    ipv6_part1[20] = '\0';
                    ipv6_part2[20] = '\0';

                    u8g2.setFont(u8g2_font_t0_12_tf  );
                    u8g2.drawStr(0,18,ipv6_part1.c_str());
                    u8g2.drawStr(0,28,ipv6_part2.c_str());
                } break;
            }
            // NTP status
            switch(global_status.wifi.ntp_status) {
                case NTP_NOT_SYNCED:{
                    u8g2.setFont(u8g2_font_helvR08_tf ); 
                    u8g2.drawStr(0,44,"Iniciando cliente NTP");
                } break;
                case NTP_AWAITING:{
                    u8g2.setFont(u8g2_font_helvR08_tf ); 
                    u8g2.drawStr(0,44,"Aguardando sinc. NTP");
                } break;
                case NTP_UPDATED:{
                    char timeStr[13];
                    time_t now = time(NULL);
                    struct tm* t = localtime(&now);
                    u8g2.drawBox(0,30,128,24);

                    strftime(timeStr, sizeof(timeStr), "%H:%M", t);
                    u8g2.setFont(u8g2_font_logisoso22_tn  );
                    u8g2.drawStr(14,53,timeStr);
                    
                    strftime(timeStr, sizeof(timeStr), ":%S", t);
                    u8g2.setFont(u8g2_font_logisoso16_tn );
                    u8g2.drawStr(82,53,timeStr);
                } break;
            }
        }
        u8g2.sendBuffer();
        vTaskDelay( 50 / portTICK_PERIOD_MS); // 20Hz
    }
}


void u8g2_setup(){
    u8g2.begin();    
    
    u8g2.setContrast(0);
    u8g2.setFontMode(1);
    u8g2.setDrawColor(2);
    
    xTaskCreatePinnedToCore( u8g2_task, 
        "u8g2_task",      2048,       NULL, 
        7,          &u8g2_task_handle,   1);

    Serial.print("\n [U8g2] U8g2 inicializado.");
}


void hwio_setup(){
    gpio_config_t io_conf;
    io_conf.intr_type =     GPIO_INTR_DISABLE;
    io_conf.mode =          GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 
        ((1ULL << RED_LED_GPIO) | (1ULL << GREEN_LED_GPIO) | (1ULL << BLUE_LED_GPIO) | (1ULL << BUZZER_GPIO));
    io_conf.pull_down_en =  GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en =    GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    led_timer = xTimerCreate("LED Timer", pdMS_TO_TICKS(1000), pdFALSE, (void *)0, led_timer_callback);

    beepCommandQueue = xQueueCreate(10, sizeof(BeepCommand));
    xTaskCreatePinnedToCore( buzzer_task, 
        "buzzer_task",      2048,       NULL, 
        7,          &buzzer_task_handle,   1);
    
    Serial.print(F("\n [Init] GPIOs e timers inicializados."));
    status_LED(1,0,0,120000);   //vermelho sólido durante início do boot por até 2h
}