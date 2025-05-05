#include "membro.h"
#include "diretorio.h"
#include "archive.h"
#include "operacoes.h"

#define CAPACIDADE_INICIAL 10

struct Diretorio *criar_diretorio(){
    
    struct Diretorio *dir;

    if(!(dir = malloc(sizeof(struct Diretorio)))){
        perror("Erro ao alocar diretório");
        return NULL;
    }

    dir->membros = NULL;
    dir->quantidade = 0;
    dir->capacidade = CAPACIDADE_INICIAL;

    if(!(dir->membros = malloc(dir->capacidade * sizeof(struct Membro)))){
        perror("Erro ao alocar membros do diretório");
        return NULL;
    }
    
    return dir;
}

void destruir_diretorio(struct Diretorio *dir){
    
    if(dir){
        free(dir->membros);
        free(dir);
    }
}

int garantir_capacidade_diretorio(struct Diretorio *dir) {

    if(dir->quantidade > dir->capacidade) {
        dir->capacidade = dir->quantidade * 2;

        if(!(dir->membros = realloc(dir->membros, dir->capacidade * sizeof(struct Membro)))) {
            perror("Erro ao alocar memória para membros");
            return 0;
        }
    }

    return 1;
}

int encontrar_indice(const struct Diretorio *dir, const char *nome) {
    
    for(int i = 0; i < dir->quantidade; i++) {
        if((strcmp(dir->membros[i].nome, nome) == 0)) {
            return i;
        }
    }
    
    return -1;
}

int adicionar_membro(struct Diretorio *dir, struct Membro novo){

    (dir->quantidade)++;

    if(!garantir_capacidade_diretorio(dir)){
        return 0;
    }

    dir->membros[dir->quantidade - 1] = novo;

    return 1;
}

int remover_membro(struct Diretorio *dir, const char *nome){

    int id = encontrar_indice(dir, nome);
    
    if(id == -1) {
        return 0;
    }

    (dir->quantidade)--;

    for(int i = id; i < dir->quantidade; i++) {
        dir->membros[i] = dir->membros[i + 1];
        dir->membros[i].ordem = i;
    }

    return 1;
}

int mover_membro(struct Diretorio *dir, const char *membro, const char *target){
    return 0;
}

struct Membro *buscar_membro(struct Diretorio *dir, const char *nome){

    int id = encontrar_indice(dir, nome);
    
    if(id == -1){
        return NULL;
    }

    return &dir->membros[id];
}