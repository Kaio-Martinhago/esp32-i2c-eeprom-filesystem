#ifndef MEMORIA_H
#define MEMORIA_H

#include <stdint.h>

void memoria_init();
void memoria_fwrite(void *buffer, uint16_t posicao, uint16_t tam);
void memoria_fread(void *buffer, uint16_t posicao, uint16_t tam);

#endif