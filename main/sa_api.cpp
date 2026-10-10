#include <iostream>
#include <cstring>
#include "sa_api.h"
#include "sa.h"
#include "memoria.h"

void inicializa_SA() {
    memoria_init();
}

void formata_SA() {
    std::cout << "[INFO] Formatando o Sistema de Arquivos..." << std::endl;

    // 1. Cabeçalho: O primeiro bloco livre é o Bloco 0 (ocupa os bytes 0 e 1)[cite: 23]
    uint16_t primeiro_bloco = 0;
    memoria_fwrite(&primeiro_bloco, 0, sizeof(uint16_t));

    // 2. Inodos: Criamos um inodo zerado e gravamos 12 vezes seguidas[cite: 23, 24]
    Inodo inodo_vazio;
    std::memset(&inodo_vazio, 0, sizeof(Inodo));
    inodo_vazio.ponteiro_indireto = 0xFFFF;

    uint16_t offset_atual = 2; // Pula os 2 bytes do cabeçalho
    for (int i = 0; i < MAX_ARQUIVOS; i++) {
        memoria_fwrite(&inodo_vazio, offset_atual, sizeof(Inodo));
        offset_atual += sizeof(Inodo); // O offset avança exatamente 45 bytes a cada volta
    }

    // 3. Lista Encadeada: Formata a área azul dos blocos de ponteiros[cite: 23]
    // A cada volta, gravamos 2 bytes contendo o número do PRÓXIMO bloco livre[cite: 23]
    for (uint16_t i = 0; i < MAX_BLOCOS; i++) {
        uint16_t proximo_bloco;
        
        // Se for o último bloco da memória, usamos 0xFFFF para marcar o "Fim da Linha"
        if (i == MAX_BLOCOS - 1) {
            proximo_bloco = 0xFFFF;
        } else {
            proximo_bloco = i + 1;
        }

        memoria_fwrite(&proximo_bloco, offset_atual, sizeof(uint16_t));
        offset_atual += sizeof(uint16_t);
    }

    std::cout << "[OK] Formatacao concluida! Sistema de Arquivos Zerado." << std::endl;
}

ARQUIVO* meu_fopen(const char* nome, const char* modo) {
    Inodo inodo_temp;
    uint16_t offset = OFFSET_INODOS; 
    int id_encontrado = -1;

    if (modo[0] == 'w') {
        // PASSO 1: Verifica se já existe para sobrescrever (Overwrite)
        for (int i = 0; i < MAX_ARQUIVOS; i++) {
            memoria_fread(&inodo_temp, offset + (i * sizeof(Inodo)), sizeof(Inodo));
            if (inodo_temp.status == 1 && std::strncmp(inodo_temp.nome, nome, 8) == 0) {
                // Devolve o bloco extra para a lista, se existir[cite: 24]
                if (inodo_temp.ponteiro_indireto != 0xFFFF) {
                    liberar_bloco(inodo_temp.ponteiro_indireto);
                }
                // Limpa o inodo fisicamente no disco
                std::memset(&inodo_temp, 0, sizeof(Inodo));
                inodo_temp.ponteiro_indireto = 0xFFFF;
                memoria_fwrite(&inodo_temp, offset + (i * sizeof(Inodo)), sizeof(Inodo));
                break; // Arquivo limpo, podemos sair deste laço
            }
        }

        // PASSO 2: Procurar uma entrada livre (status == 0)[cite: 24]
        for (int i = 0; i < MAX_ARQUIVOS; i++) {
            memoria_fread(&inodo_temp, offset + (i * sizeof(Inodo)), sizeof(Inodo));
            if (inodo_temp.status == 0) {
                id_encontrado = i;
                
                inodo_temp.status = 1;
                std::strncpy(inodo_temp.nome, nome, 8);
                inodo_temp.tamanho = 0;
                inodo_temp.ponteiro_indireto = 0xFFFF; // Garante ausência de bloco extra
                
                memoria_fwrite(&inodo_temp, offset + (i * sizeof(Inodo)), sizeof(Inodo));
                break;
            }
        }
    } 
    else if (modo[0] == 'r') {
        // MODO LEITURA (Mantém-se igual)
        for (int i = 0; i < MAX_ARQUIVOS; i++) {
            memoria_fread(&inodo_temp, offset + (i * sizeof(Inodo)), sizeof(Inodo));
            if (inodo_temp.status == 1 && std::strncmp(inodo_temp.nome, nome, 8) == 0) {
                id_encontrado = i;
                break;
            }
        }
    }

    if (id_encontrado != -1) {
        ARQUIVO* novo_arquivo = new ARQUIVO;
        novo_arquivo->inodo_id = id_encontrado;
        novo_arquivo->modo = modo[0];
        novo_arquivo->cursor = 0;
        return novo_arquivo;
    }

    return nullptr;
}


void meu_fclose(ARQUIVO* arquivo) {
    if (arquivo != nullptr) {
        delete arquivo; // Liberta a memória RAM usada pelo descritor do ficheiro
    }
}


void meu_fwrite(ARQUIVO* arquivo, uint8_t* buffer, uint16_t tamanho_escrita) {
    if (arquivo == nullptr || arquivo->modo != 'w') {
        std::cout << "[ERRO] Ficheiro invalido ou nao aberto para escrita." << std::endl;
        return;
    }

    // Impede que a escrita ultrapasse o limite absoluto do sistema (64 bytes)
    if (arquivo->cursor + tamanho_escrita > 64) {
        std::cout << "[ERRO] Espaco insuficiente. O limite maximo de um ficheiro e 64 bytes." << std::endl;
        tamanho_escrita = 64 - arquivo->cursor; // Trunca a escrita para o que resta
        if (tamanho_escrita == 0) return;
    }

    // 1. Resgata o Inodo atual do disco
    Inodo inodo;
    uint16_t endereco_inodo = OFFSET_INODOS + (arquivo->inodo_id * sizeof(Inodo));
    memoria_fread(&inodo, endereco_inodo, sizeof(Inodo));

    uint16_t bytes_escritos = 0;

    // 2. PREENCHIMENTO DO BLOCO DIRETO (Até 32 bytes)[cite: 23]
    if (arquivo->cursor < 32) {
        uint16_t espaco_direto = 32 - arquivo->cursor;
        uint16_t para_escrever_agora = (tamanho_escrita < espaco_direto) ? tamanho_escrita : espaco_direto;

        for(uint16_t i = 0; i < para_escrever_agora; i++) {
            inodo.bloco_direto[arquivo->cursor + i] = buffer[bytes_escritos++];
        }
        arquivo->cursor += para_escrever_agora;
    }

    // 3. PREENCHIMENTO DO BLOCO INDIRETO (Mais 32 bytes)[cite: 24]
    uint16_t faltam_escrever = tamanho_escrita - bytes_escritos;
    
    if (faltam_escrever > 0) {
        // Se ainda não temos um bloco extra alocado, pedimos um à lista ligada
        if (inodo.ponteiro_indireto == 0xFFFF) {
            inodo.ponteiro_indireto = alocar_bloco();
            if (inodo.ponteiro_indireto == 0xFFFF) {
                std::cout << "[ERRO] Disco cheio! Nao ha blocos amarelos disponiveis." << std::endl;
                return;
            }
            std::cout << "[INFO] Bloco extra " << inodo.ponteiro_indireto << " alocado para o ficheiro." << std::endl;
        }

        // Calcula exatamente onde escrever na área amarela
        // Deslocamento: Início da área de dados + (índice do bloco * 32 bytes)[cite: 23]
        uint16_t endereco_dados = OFFSET_DADOS + (inodo.ponteiro_indireto * 32);
        
        // Posição local dentro do bloco amarelo (cursor - 32)
        uint16_t posicao_no_bloco_amarelo = arquivo->cursor - 32;

        for(uint16_t i = 0; i < faltam_escrever; i++) {
            // Escreve diretamente no disco byte a byte (poderia ser otimizado com um array temporário)
            uint8_t byte_atual = buffer[bytes_escritos++];
            memoria_fwrite(&byte_atual, endereco_dados + posicao_no_bloco_amarelo + i, 1);
        }
        
        arquivo->cursor += faltam_escrever;
    }

    // 4. Atualiza o tamanho e guarda o Inodo modificado de volta[cite: 24]
    if (arquivo->cursor > inodo.tamanho) {
        inodo.tamanho = arquivo->cursor;
    }
    memoria_fwrite(&inodo, endereco_inodo, sizeof(Inodo));
    
    std::cout << "[OK] Escrita de " << tamanho_escrita << " bytes concluida. Cursor atual: " << arquivo->cursor << "/64" << std::endl;
}



void meu_fread(ARQUIVO* arquivo, uint8_t* buffer, uint16_t tamanho_leitura) {
    if (arquivo == nullptr || arquivo->modo != 'r') {
        std::cout << "[ERRO] Arquivo invalido ou nao aberto para leitura." << std::endl;
        return;
    }

    // 1. Resgata o Inodo para saber o tamanho e os dados
    Inodo inodo;
    uint16_t endereco_inodo = OFFSET_INODOS + (arquivo->inodo_id * sizeof(Inodo));
    memoria_fread(&inodo, endereco_inodo, sizeof(Inodo));

    // Impede a leitura além do tamanho real do arquivo
    if (arquivo->cursor + tamanho_leitura > inodo.tamanho) {
        tamanho_leitura = inodo.tamanho - arquivo->cursor;
    }

    if (tamanho_leitura == 0) return; // Nada mais a ler (EOF)

    uint16_t bytes_lidos = 0;

    // 2. LEITURA DO BLOCO DIRETO (Até 32 bytes)[cite: 23]
    if (arquivo->cursor < 32) {
        uint16_t espaco_direto = 32 - arquivo->cursor;
        uint16_t para_ler_agora = (tamanho_leitura < espaco_direto) ? tamanho_leitura : espaco_direto;

        for (uint16_t i = 0; i < para_ler_agora; i++) {
            buffer[bytes_lidos++] = inodo.bloco_direto[arquivo->cursor + i];
        }
        arquivo->cursor += para_ler_agora;
    }

    // 3. LEITURA DO BLOCO INDIRETO (Área Amarela)[cite: 24]
    uint16_t faltam_ler = tamanho_leitura - bytes_lidos;
    
    if (faltam_ler > 0 && inodo.ponteiro_indireto != 0xFFFF) {
        // Encontra o endereço físico do bloco extra do arquivo[cite: 23]
        uint16_t endereco_dados = OFFSET_DADOS + (inodo.ponteiro_indireto * 32);
        
        // Descobre onde o cursor parou dentro do bloco amarelo (cursor - 32)
        uint16_t posicao_no_bloco = arquivo->cursor - 32;

        for (uint16_t i = 0; i < faltam_ler; i++) {
            // Lê do disco byte a byte para preencher o resto do buffer
            memoria_fread(&buffer[bytes_lidos++], endereco_dados + posicao_no_bloco + i, 1);
        }
        arquivo->cursor += faltam_ler;
    }
}

uint16_t meu_ftell(ARQUIVO* arquivo) {
    if (arquivo == nullptr) return 0;
    return arquivo->cursor;
}


void meu_fseek(ARQUIVO* arquivo, int16_t offset, int origem) {
    if (arquivo == nullptr) return;

    // Precisamos buscar o Inodo no disco para saber qual é o limite (tamanho real)
    Inodo inodo;
    uint16_t endereco_inodo = OFFSET_INODOS + (arquivo->inodo_id * sizeof(Inodo));
    memoria_fread(&inodo, endereco_inodo, sizeof(Inodo));

    int16_t nova_posicao = arquivo->cursor;

    // Aplica o deslocamento baseado no ponto de origem
    if (origem == SEEK_SET) {
        nova_posicao = offset;                // A partir do início (posição absoluta)
    } else if (origem == SEEK_CUR) {
        nova_posicao += offset;               // A partir de onde o cursor já está
    } else if (origem == SEEK_END) {
        nova_posicao = inodo.tamanho + offset; // A partir do fim do arquivo (offset deve ser negativo)
    }

    // Travas de segurança: O cursor não pode ser negativo nem maior que o arquivo
    if (nova_posicao < 0) {
        nova_posicao = 0;
    } else if (nova_posicao > inodo.tamanho) {
        nova_posicao = inodo.tamanho;
    }

    // Efetiva o teletransporte
    arquivo->cursor = nova_posicao;
}

int meu_feof(ARQUIVO* arquivo) {
    if (arquivo == nullptr) return 1; // Se o arquivo não existe, já acabou.

    Inodo inodo;
    uint16_t endereco_inodo = OFFSET_INODOS + (arquivo->inodo_id * sizeof(Inodo));
    memoria_fread(&inodo, endereco_inodo, sizeof(Inodo));

    // Retorna 1 (verdadeiro) se o cursor alcançou ou ultrapassou o tamanho do arquivo
    if (arquivo->cursor >= inodo.tamanho) {
        return 1;
    }
    
    return 0; // Falso: ainda tem coisas para ler
}


void meu_remove(const char* nome) {
    Inodo inodo;
    uint16_t offset = OFFSET_INODOS;

    for (int i = 0; i < MAX_ARQUIVOS; i++) {
        memoria_fread(&inodo, offset + (i * sizeof(Inodo)), sizeof(Inodo));
        
        // Verifica se o inodo está ocupado e se o nome confere
        if (inodo.status == 1 && std::strncmp(inodo.nome, nome, 8) == 0) {
            
            // 1. Se o arquivo utilizava um bloco amarelo extra, devolve para a lista azul[cite: 24]
            if (inodo.ponteiro_indireto != 0xFFFF) {
                liberar_bloco(inodo.ponteiro_indireto);
                std::cout << "[INFO] Bloco extra " << inodo.ponteiro_indireto << " devolvido a lista livre." << std::endl;
            }
            
            // 2. Zera a entrada de arquivo para torná-la disponível novamente[cite: 24]
            std::memset(&inodo, 0, sizeof(Inodo)); // O status volta a ser 0
            inodo.ponteiro_indireto = 0xFFFF;      // Garante a consistência sem alocações falsas
            
            // 3. Salva a exclusão no disco
            memoria_fwrite(&inodo, offset + (i * sizeof(Inodo)), sizeof(Inodo));
            
            std::cout << "[OK] Arquivo '" << nome << "' removido com sucesso!" << std::endl;
            return;
        }
    }
    std::cout << "[ERRO] Arquivo '" << nome << "' nao encontrado." << std::endl;
}