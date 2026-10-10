#include <iostream>
#include <cstdio>
#include <cstring>
#include "sa_api.h"

// O ESP-IDF exige essa declaração para iniciar códigos C++
extern "C" void app_main(void) {
    std::cout << "Sistema de Arquivos Embarcados - Teste no ESP32" << std::endl;
    
    // As chamadas abaixo estão comentadas por enquanto para não dar erro 
    // de "função não declarada", já que ainda vamos criá-las.
    
    inicializa_SA();
    formata_SA(); 


//    meu_remove("test.txt");


    const char* modo = "w";

    ARQUIVO* arquivo = meu_fopen("viva.txt", modo);
    const char* texto_longo = "janelas macias abertas na sacada";        
    meu_fwrite(arquivo, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(texto_longo)), std::strlen(texto_longo));
    meu_fclose(arquivo);

    ARQUIVO* arquivo2 = meu_fopen("nova.txt", "w");
    const char* texto_longo2 = "vozes vazias viajam vagando velozes";
    meu_fwrite(arquivo2, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(texto_longo2)), std::strlen(texto_longo2));
    meu_fclose(arquivo2);

    ARQUIVO* arquivo3 = meu_fopen("leva.txt", "w");
    const char* texto_longo3 = "leves leves leves leves leves leves leves";
    meu_fwrite(arquivo3, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(texto_longo3)), std::strlen(texto_longo3));
    meu_fclose(arquivo3);

    ARQUIVO* arquivo4 = meu_fopen("pipa.txt", "w");
    const char* texto_longo4 = "pipocas pipocando pipocando pipocando pipocando";
    meu_fwrite(arquivo4, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(texto_longo4)), std::strlen(texto_longo4));
    meu_fclose(arquivo4);

    bool flag = true; // Alterado para true para já realizar o teste de leitura

    if(!arquivo) {
        std::cout << "[ERRO] Nao foi possivel abrir o arquivo." << std::endl;
        return; // Removido o '1' pois app_main é void
    }

    if (flag){
        
        // Reabrindo o arquivo em modo leitura para o teste funcionar
        arquivo = meu_fopen("viva.txt", "r");
        
        if (arquivo && arquivo->modo == 'r') {
            std::uint8_t buffer[65];
            std::memset(buffer, 0, sizeof(buffer));
            meu_fread(arquivo, buffer, 64);
            std::cout << "Conteudo lido do arquivo: " << std::endl  << buffer << std::endl;
        
            
            std::uint8_t buffer2[65];
            std::memset(buffer2, 0, sizeof(buffer2));
            meu_fseek(arquivo, 19, SEEK_SET);
            meu_fread(arquivo, buffer2, 11);
            std::cout << "Lido do meio do arquivo: " << std::endl  << buffer2 << std::endl;

        
        }

        if (arquivo && arquivo->modo == 'w') {
            const char* texto_longo = "janelas macias abertas na sacada";        
            meu_fwrite(arquivo, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(texto_longo)), std::strlen(texto_longo));
        }

        if (meu_feof(arquivo)) {
            std::cout << "Fim do arquivo atingido." << std::endl;
        } else {
            std::cout << "Ainda ha dados para ler." << std::endl;
        }

        meu_fclose(arquivo);

    }

}