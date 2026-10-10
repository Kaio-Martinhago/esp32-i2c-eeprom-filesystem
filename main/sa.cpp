#include "sa.h"
#include "memoria.h"

uint16_t alocar_bloco() {
    uint16_t bloco_alocado;
    
    // 1. Lê o cabeçalho (posição 0) para ver qual é o primeiro bloco livre
    memoria_fread(&bloco_alocado, OFFSET_CABECALHO, sizeof(uint16_t));
    
    if (bloco_alocado == 0xFFFF) {
        return 0xFFFF; // Erro: O disco está cheio (lista encadeada terminou)
    }

    // 2. Vai até a área azul (ponteiros) para descobrir quem é o próximo da fila[cite: 23, 24]
    uint16_t proximo_livre;
    uint16_t endereco_lista = OFFSET_PONTEIROS + (bloco_alocado * sizeof(uint16_t));
    memoria_fread(&proximo_livre, endereco_lista, sizeof(uint16_t));

    // 3. Atualiza o cabeçalho informando o novo primeiro bloco livre
    memoria_fwrite(&proximo_livre, OFFSET_CABECALHO, sizeof(uint16_t));

    return bloco_alocado; // Retorna o ID do bloco amarelo que agora pertence ao arquivo
}

void liberar_bloco(uint16_t bloco) {
    if (bloco == 0xFFFF) return; // Proteção contra blocos nulos

    // 1. Descobre quem é o primeiro bloco livre atual lendo o cabeçalho
    uint16_t primeiro_livre_atual;
    memoria_fread(&primeiro_livre_atual, OFFSET_CABECALHO, sizeof(uint16_t));

    // 2. Faz o ponteiro do bloco que estamos a devolver (na zona azul) apontar para o antigo "primeiro"
    uint16_t endereco_lista = OFFSET_PONTEIROS + (bloco * sizeof(uint16_t));
    memoria_fwrite(&primeiro_livre_atual, endereco_lista, sizeof(uint16_t));

    // 3. Atualiza o cabeçalho indicando que o bloco devolvido é o novo "primeiro" livre da fila
    memoria_fwrite(&bloco, OFFSET_CABECALHO, sizeof(uint16_t));
}