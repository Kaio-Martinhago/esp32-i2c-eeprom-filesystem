#ifndef SA_API_H
#define SA_API_H

#include <cstdint>
#include <cstdio>

// Estrutura que funciona como o "descritor" do ficheiro aberto.
// Fica guardada na memória RAM, não no disco (EEPROM).
typedef struct {
    uint8_t inodo_id;    // Guarda qual dos 12 inodos (0 a 11) este ficheiro usa
    char modo;           // Modo de abertura: 'r' (leitura) ou 'w' (escrita)
    uint16_t cursor;     // O "ponteiro" interno que diz em que byte estamos a ler/escrever
} ARQUIVO;

void inicializa_SA();
void formata_SA();

// Funções da API de alto nível solicitadas[cite: 25]
ARQUIVO* meu_fopen(const char* nome, const char* modo);
void meu_fclose(ARQUIVO* arquivo);

void meu_fwrite(ARQUIVO* arquivo, uint8_t* buffer, uint16_t tamanho);

void meu_fread(ARQUIVO* arquivo, uint8_t* buffer, uint16_t tamanho_leitura);
uint16_t meu_ftell(ARQUIVO* arquivo);


void meu_fseek(ARQUIVO* arquivo, int16_t offset, int origem);
int meu_feof(ARQUIVO* arquivo);

void meu_remove(const char* nome);
#endif