#ifndef MEMBRO_H
#define MEMBRO_H

#include <stddef.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct Membro{
    char nome[1025];            // Nome do arquivo (sem espaços)
    uid_t uid;                  // UID do dono
    size_t tamanho_original;    // Tamanho real do arquivo
    size_t tamanho_armazenado;  // Tamanho no archive (.vc)
    time_t data_modificacao;    // Última modificação
    int ordem;                  // Ordem de inserção
    long offset;                // Posição no arquivo .vc
};

/**
 * Cria um membro com base em um arquivo no disco.
 * Preenche metadados como UID, tamanho, data de modificação etc.
 * `ordem` é a ordem de inserção (gerenciada pelo diretório).
 */
struct Membro criar_membro(const char *caminho_arquivo, int ordem);

/**
 * Atualiza os dados do membro com base em um novo arquivo (substituição).
 */
void atualizar_membro(struct Membro *m, const char *caminho_arquivo);

/**
 * Imprime os dados de um membro de forma formatada.
 * Pode ser usado no comando -c (listar conteúdo do archive).
 */
void imprimir_membro(const struct Membro *m);

#endif
