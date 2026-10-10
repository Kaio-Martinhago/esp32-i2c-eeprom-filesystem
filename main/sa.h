#ifndef SA_H
#define SA_H

#include <cstdint>

// Força o compilador a empacotar a struct sem adicionar bytes extras (padding)
#pragma pack(push, 1)

// Estrutura de 45 bytes representando cada entrada de arquivo (Inodo)
struct Inodo {
    uint8_t  status;             // 1 byte  (0 = livre, 1 = ocupado)[cite: 24]
    char     nome[8];            // 8 bytes para o nome[cite: 24]
    uint16_t tamanho;            // 2 bytes para o tamanho do conteúdo[cite: 24]
    uint8_t  bloco_direto[32];   // 32 bytes de armazenamento direto de dados[cite: 24]
    uint16_t ponteiro_indireto;  // 2 bytes indicando o primeiro bloco extra[cite: 24]
};

#pragma pack(pop)

// Definições físicas do sistema
#define MAX_ARQUIVOS 12        

// A memória AT24C512 possui 65.536 bytes no total[cite: 1]
// Gastaremos: Cabeçalho (2 bytes) + 12 Inodos de 45 bytes (540) = 542 bytes ocupados[cite: 23]
// Vamos alocar 1000 blocos extras (cada par de ponteiro + dado usa 34 bytes).
// Isso ocupará 34.000 bytes adicionais, cabendo com folga na memória.
#define MAX_BLOCOS 12
// --- MAPA DE MEMÓRIA DO SISTEMA ---
#define OFFSET_CABECALHO 0
#define OFFSET_INODOS    2      // Cabeçalho tem 2 bytes
#define OFFSET_PONTEIROS 542    // 2 + (12 arquivos * 45 bytes) = 542
#define OFFSET_DADOS     566   // 542 + (12 blocos azuis * 2 bytes) = 566

// Declaração da função interna de alocação
uint16_t alocar_bloco();
void liberar_bloco(uint16_t bloco);
#endif