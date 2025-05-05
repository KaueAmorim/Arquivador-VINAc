#ifndef DIRETORIO_H
#define DIRETORIO_H

#include "membro.h"
#include <stdio.h>

struct Diretorio {
    struct Membro *membros;  // Vetor dinâmico de membros
    int quantidade;          // Quantidade atual de membros
    int capacidade;          // Tamanho alocado do vetor
};

/**
 * Cria um novo diretório vazio.
 */
struct Diretorio *criar_diretorio();

/**
 * Libera toda a memória associada ao diretório.
 */
void destruir_diretorio(struct Diretorio *dir);

/**
 * Garante que o vetor de membros tenha capacidade suficiente
 * para armazenar todos os membros até dir->quantidade.
 * Se necessário, realoca a memória com o dobro da capacidade.
 *
 * Retorna 1 em caso de sucesso, 0 em caso de falha (erro de alocação).
 */
int garantir_capacidade_diretorio(struct Diretorio *dir);

/**
 * Retorna o índice de um membro pelo nome dentro do diretório.
 * Retorna -1 se o membro não for encontrado.
 */
int encontrar_indice(const struct Diretorio *dir, const char *nome);

/**
 * Adiciona um novo membro ao diretório.
 * Se já existir um membro com o mesmo nome, substitui.
 */
int adicionar_membro(struct Diretorio *dir, struct Membro novo);

/**
 * Remove um membro pelo nome.
 * Retorna 1 se remover com sucesso, 0 se não encontrar.
 */
int remover_membro(struct Diretorio *dir, const char *nome);

/**
 * Move um membro para depois de outro.
 * Retorna 1 em caso de sucesso, 0 se um dos membros não for encontrado.
 */
int mover_membro(struct Diretorio *dir, const char *membro, const char *target);

/**
 * Busca e retorna um ponteiro para um membro pelo nome.
 * Retorna NULL se não encontrar.
 */
struct Membro *buscar_membro(struct Diretorio *dir, const char *nome);

#endif
