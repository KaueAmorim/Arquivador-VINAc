#ifndef ARCHIVE_H
#define ARCHIVE_H

#include "membro.h"
#include "diretorio.h"
#include "operacoes.h"

/**
 * Cria e inicializa um buffer.
 */
struct Buffer *criar_buffer(struct Diretorio *dir);

/**
 * Redimensiona um buffer.
 */
int redimensionar_buffer(struct Buffer *buffer, int novo_tamanho);

/**
 * Libera toda a memória associada ao buffer.
 */
void destruir_buffer(struct Buffer *buffer);

/**
 * Atualiza os offsets de todos os membros, começando após o diretório.
 */
void atualizar_offsets(struct Diretorio *dir);

/**
 * Desloca um membro no arquivo vc em 'deslocamento' bytes.
 */
int deslocar_membro(FILE *vc, struct Buffer *buffer, struct Membro *m, long deslocamento);

/**
 * Lê o diretório do início de um arquivo archive (.vc).
 */
int ler_diretorio(FILE *vc, struct Diretorio *dir);

/**
 * Salva o diretório no início de um arquivo archive (.vc).
 */
int escrever_diretorio(FILE *vc, const struct Diretorio *dir);

#endif