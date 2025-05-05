#include "membro.h"
#include "diretorio.h"
#include "archive.h"
#include "operacoes.h"


struct Membro criar_membro(const char *caminho_arquivo, int ordem){
    
    struct Membro novo;
    struct stat info;

    if(stat(caminho_arquivo, &info) == -1){
        perror("Erro ao acessar o arquivo");
        exit(1);
    }

    novo.uid = info.st_uid;
    novo.tamanho_original = info.st_size;
    novo.tamanho_armazenado = info.st_size;
    novo.data_modificacao = info.st_mtime;
    novo.ordem = ordem;
    novo.offset = -1;

    if(strncpy(novo.nome, caminho_arquivo, sizeof(novo.nome)) == NULL){
        perror("Erro ao copiar nome do arquivo");
        exit(1);
    }
    novo.nome[sizeof(novo.nome) - 1] = '\0';
    
    return novo;
}

void atualizar_membro(struct Membro *m, const char *caminho_arquivo){
    
    struct stat info;

    if(stat(caminho_arquivo, &info) == -1){
        perror("Erro ao acessar o arquivo para atualização");
        exit(1);
    }

    m->uid = info.st_uid;
    m->tamanho_original = info.st_size;
    m->tamanho_armazenado = info.st_size;
    m->data_modificacao = info.st_mtime;
}


void imprimir_membro(const struct Membro *m){
    
    char data_formatada[64];
    struct tm *tm_info = localtime(&(m->data_modificacao));
    
    strftime(data_formatada, sizeof(data_formatada), "%Y-%m-%d %H:%M:%S", tm_info);

    printf("Nome: %s\n", m->nome);
    printf("UID: %d\n", m->uid);
    printf("Tamanho Original: %zu bytes\n", m->tamanho_original);
    printf("Tamanho no Archive: %zu bytes\n", m->tamanho_armazenado);
    printf("Data de Modificação: %s\n", data_formatada);
    printf("Ordem: %d\n", m->ordem);
    printf("Offset: %ld\n", m->offset);
    printf("----------------------------------\n");
}