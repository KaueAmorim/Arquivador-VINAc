#ifndef OPERACOES_H
#define OPERACOES_H

#include "membro.h"
#include "diretorio.h"
#include "archive.h"

// Constantes para as operações
#define OP_INVALIDA 0
#define OP_INSERIR_PLANO 1
#define OP_INSERIR_COMPRIMIDO 2
#define OP_MOVER 3 
#define OP_EXTRAIR 4
#define OP_REMOVER 5
#define OP_LISTAR 6

// Struct Comando (reconhece e armazena os argumentos da linha de comando)
struct Comando {
    int op;
    char *arquivo_vc;
    char **membros;
    int num_membros;
    char *target; // usado apenas na operação -m
};

struct Buffer{
    char *dados;
    size_t tamanho;
};

/**
 * Identifica a operação a ser realizada com base na string de argumento (ex: "-ip").
 */
int identificar_operacao(const char *arg);

/**
 * Faz o parsing dos argumentos da linha de comando e preenche uma struct comando.
 */
struct Comando parse_comando(int argc, char **argv);

/**
 * Executa a operação de inserção sem compressão (-ip).
 */
void executar_insercao_plana(FILE *vc, struct Diretorio *dir, struct Comando *cmd, struct Buffer *buffer);

/**
 * Executa a operação de inserção com compressão (-ic).
 */
void executar_insercao_comprimida(FILE *vc, struct Diretorio *dir, struct Comando *cmd, struct Buffer *buffer);

/**
 * Executa a movimentação de membros dentro do diretório (-m).
 */
void executar_movimentacao(FILE *vc, struct Diretorio *dir, struct Comando *cmd, struct Buffer *buffer);

/**
 * Executa a extração de membros (-x).
 */
void executar_extracao(FILE *vc, struct Diretorio *dir, struct Comando *cmd, struct Buffer *buffer);

/**
 * Executa a operação de remoção de membros (-r).
 */
void executar_remocao(FILE *vc, struct Diretorio *dir, struct Comando *cmd, struct Buffer *buffer);

/**
 * Executa a listagem dos membros do diretório (-c).
 */
void executar_listagem(struct Diretorio *dir);

#endif