#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "memoria.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2C_MASTER_SDA_IO           (gpio_num_t)21
#define I2C_MASTER_SCL_IO           (gpio_num_t)22
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000 // Voltando para a velocidade padrão
#define EEPROM_ADDR                 0x50

void memoria_init() {
    printf("Inicializando barramento I2C do ESP32...\n");
    
    i2c_config_t conf;
    memset(&conf, 0, sizeof(i2c_config_t));
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    
    printf("I2C inicializado. Testando comunicacao...\n");
    
    // Ping corrigido: Lendo 1 byte para ver se o chip responde sem usar buffer NULL
    uint8_t dummy_ping = 0;
    esp_err_t err = i2c_master_read_from_device(I2C_MASTER_NUM, EEPROM_ADDR, &dummy_ping, 1, pdMS_TO_TICKS(1000));
    if (err == ESP_OK) {
        printf("[OK] EEPROM AT24C512 detectada no endereco 0x50!\n");
    } else {
        printf("[ERRO CRITICO] EEPROM NAO RESPONDEU! Verifique os jumpers SDA e SCL.\n");
        return;
    }

    // Teste de Sanidade de Gravação (Write Protect)
    uint8_t teste_escrita = 0xAB;
    uint8_t teste_leitura = 0x00;
    memoria_fwrite(&teste_escrita, 65000, 1); // Grava num cantinho inútil da memória
    memoria_fread(&teste_leitura, 65000, 1);
    
    if (teste_leitura == 0xAB) {
        printf("[OK] A memoria esta gravando fisicamente sem bloqueios!\n");
    } else {
        printf("[ERRO DE HARDWARE] O chip nao esta salvando os dados! Verifique se o Pino 7 (WP) esta MESMO ligado ao GND.\n");
    }
}

void memoria_fwrite(void *buffer, uint16_t posicao, uint16_t tam) {
    if (tam == 0) return;

    uint8_t *buf = (uint8_t*)buffer;
    
    // Gravando BYTE POR BYTE (Mais seguro para evitar problemas de página)
    for(uint16_t i = 0; i < tam; i++) {
        uint8_t payload[3];
        payload[0] = ((posicao + i) >> 8) & 0xFF;
        payload[1] = (posicao + i) & 0xFF;       
        payload[2] = buf[i];                     

        i2c_master_write_to_device(I2C_MASTER_NUM, EEPROM_ADDR, payload, 3, pdMS_TO_TICKS(100));
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void memoria_fread(void *buffer, uint16_t posicao, uint16_t tam) {
    if (tam == 0) return;

    uint8_t endereco[2];
    endereco[0] = (posicao >> 8) & 0xFF;
    endereco[1] = posicao & 0xFF;        

    i2c_master_write_read_device(I2C_MASTER_NUM, EEPROM_ADDR, endereco, 2, (uint8_t*)buffer, tam, pdMS_TO_TICKS(1000));
}