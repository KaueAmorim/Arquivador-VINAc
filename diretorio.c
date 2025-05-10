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

    dir->quantidade = 0;
    dir->capacidade = CAPACIDADE_INICIAL;

    if(!(dir->membros = malloc(dir->capacidade * sizeof(struct Membro *)))){
        perror("Erro ao alocar membros do diretório");
        free(dir);
        return NULL;
    }
    
    return dir;
}

void destruir_diretorio(struct Diretorio *dir){
    
    if(dir){
        for(int i = 0; i < dir->quantidade; i++){
            free(dir->membros[i]);
        }
        free(dir->membros);
        free(dir);
    }
}

int garantir_capacidade_diretorio(struct Diretorio *dir) {

    if(dir->quantidade >= dir->capacidade){
        dir->capacidade = dir->quantidade * 2;

        if(!(dir->membros = realloc(dir->membros, dir->capacidade * sizeof(struct Membro *)))) {
            perror("Erro ao realocar vetor de membros");
            return 0;
        }
    }

    return 1;
}

int encontrar_indice(const struct Diretorio *dir, const char *nome) {
    
    for(int i = 0; i < dir->quantidade; i++) {
        if((strcmp(dir->membros[i]->nome, nome) == 0)) {
            return i;
        }
    }
    
    return -1;
}

int adicionar_membro(struct Diretorio *dir, struct Membro *novo){

    if(!garantir_capacidade_diretorio(dir)){
        return 0;
    }

    dir->membros[dir->quantidade] = novo;
    (dir->quantidade)++;

    return 1;
}

int remover_membro(struct Diretorio *dir, const char *nome){

    int id = encontrar_indice(dir, nome);
    
    if(id == -1){
        return 0;
    }

    // Libera o membro que será removido
    free(dir->membros[id]);

    for(int i = id; i < dir->quantidade - 1; i++){
        dir->membros[i] = dir->membros[i + 1];
        dir->membros[i]->ordem = i;
    }

    (dir->quantidade)--;

    return 1;
}

void mover_membro(struct Diretorio *dir, int i_mover, int i_target){

    struct Membro *m = dir->membros[i_mover];

    if(i_mover > i_target){
        for(int i = i_mover; i > i_target + 1; i--){
            dir->membros[i] = dir->membros[i - 1];
            dir->membros[i]->ordem = i;
        }
        
        dir->membros[i_target + 1] = m;
        m->ordem = i_target + 1;
    } 
    else{
        for(int i = i_mover; i < i_target; i++){
            dir->membros[i] = dir->membros[i + 1];
            dir->membros[i]->ordem = i;
        }

        dir->membros[i_target] = m;
        m->ordem = i_target;
    }
}

struct Membro *buscar_membro(struct Diretorio *dir, const char *nome){

    int id = encontrar_indice(dir, nome);
    
    if(id == -1){
        return NULL;
    }

    return dir->membros[id];
}