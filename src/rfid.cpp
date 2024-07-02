#include "nomes.h"

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
MFRC522::MIFARE_Key mifare_key;
MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
MFRC522::StatusCode mifare_status;

void mifare_setup(){ 
    Serial.print(F("\n [MFRC522] Iniciando MFRC522..."));    
    mfrc522.PCD_Init();
    for( byte i = 0; i < 6; i++ ) mifare_key.keyByte[i] = 0xFF;
    Serial.print(F("\n [MFRC522] MFRC522 iniciado e chaves definidas.")); 
}

void mifare_data_exibir(){  //refeito em um único printf:
    Serial.printf("\n  Mat: \t\t%s\n  Nome: \t%s\n  Turma: \t%s\n  Tag ID: \t%s\n",
        mifare_data.matricula_aluno,mifare_data.nome_aluno,mifare_data.turma_aluno,mifare_data.tag_id);
}

void mifare_close(){
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    yield();
}

int ler_mifare(){  //retorna true se há novo cartão lido ou false se não há.
    if ( ! mfrc522.PICC_IsNewCardPresent()) return 0;   //não colocar busca em função separada para permitir aborto.
    if ( ! mfrc522.PICC_ReadCardSerial())   return -1;  //não colocar escolha em função separada para permitir aborto.
    Serial.print(F("\n [MFRC522] MIFARE Detectado:"));
    // mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));
    
    int array_temp[8]={1,2,4,5,6,8,9,10};   //setores permitidos para leitura/escrita de informação no cartão
    for( int i=0; i<8; i++){ 
        //não colocar autenticação em função separada para permitir aborto.
        mifare_status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, array_temp[i], &mifare_key, &(mfrc522.uid));
        if (mifare_status != MFRC522::STATUS_OK) {
            Serial.print(F("\n [MFRC522] PCD_Authenticate() falhou: ")); Serial.println(mfrc522.GetStatusCodeName(mifare_status)); 
            mifare_close();
            return -1;
        } //else Serial.print(F(" Success - "));
        uint8_t len = sizeof mifare_data.mifare_bloco[i] + 2;  //Necessário para mfrc522.MIFARE_Read
        uint8_t block_buffer[18];
        mifare_status = mfrc522.MIFARE_Read(array_temp[i], block_buffer, &len);
        if (mifare_status != MFRC522::STATUS_OK) {
            Serial.print(F("\n [MFRC522] MIFARE_Read() falhou: "));  Serial.print(mfrc522.GetStatusCodeName(mifare_status));
            mifare_close();
            return -1;
        }
        for(int n = 0; n < 16; n++) mifare_data.mifare_bloco[i][n] = block_buffer[n];
    }
    mifare_close();
    mifare_data_exibir();
    yield(); 
    return 1;
}

bool gravar_mifare(){   //true usado para indicar erro, usado por flag que chama a função
    if ( ! mfrc522.PICC_IsNewCardPresent()) return true; //não colocar busca em função separada para permitir aborto.
    if ( ! mfrc522.PICC_ReadCardSerial())   return true; //não colocar escolha em função separada para permitir aborto.
    Serial.print(F(" [MFRC522] MIFARE Detectado para gravação:"));
    mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));
    Serial.print(F(" [MFRC522] Iniciando gravação de 8 blocos com MIFARE_Write(): "));
    int array_temp[8]={1,2,4,5,6,8,9,10};
    for( int i=0; i<8; i++){
        //Serial.print(i);
        mifare_status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, array_temp[i], &mifare_key, &(mfrc522.uid));
        if (mifare_status != MFRC522::STATUS_OK) {
            Serial.print(F("\n [MFRC522] PCD_Authenticate() falhou: "));
            Serial.println(mfrc522.GetStatusCodeName(mifare_status)); 
            mifare_close();
            return true;
        } //else Serial.print(F(" Success - "));
        mifare_status = mfrc522.MIFARE_Write(array_temp[i], mifare_data.mifare_bloco[i], 16);
        if (mifare_status != MFRC522::STATUS_OK) {
            Serial.print(F("\n [MFRC522] MIFARE_Write() falhou: "));
            Serial.print(mfrc522.GetStatusCodeName(mifare_status));
            mifare_close();
            return true;
        } else Serial.printf("%d OK. ",i);
    } 
    Serial.println(F("\n [MFRC522] Gravação concluída. "));
    mifare_close();
    return false;
}