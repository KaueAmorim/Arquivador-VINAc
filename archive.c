#include "membro.h"
#include "diretorio.h"
#include "archive.h"
#include "operacoes.h"
#include "lz.h"

struct Buffer *criar_buffer(struct Diretorio *dir){
    
    struct Buffer *buffer;

    if(!(buffer = malloc(sizeof(struct Buffer)))){
        perror("Erro ao alocar memória para buffer");
        return NULL;
    }

    buffer->tamanho = 1;

    // Encontra o maior tamanho_armazenado entre os membros
    for(int i = 0; i < dir->quantidade; i++){
        if(dir->membros[i] && dir->membros[i]->tamanho_armazenado > buffer->tamanho){
            buffer->tamanho = dir->membros[i]->tamanho_armazenado;
        }
    }

    if(!(buffer->dados = malloc(buffer->tamanho))){
        perror("Erro ao alocar dados do buffer");
        free(buffer);
        return NULL;
    }

    return buffer;
}

int redimensionar_buffer(struct Buffer *buffer, int novo_tamanho){
    
    if(!buffer || novo_tamanho <= buffer->tamanho){
        return 1;
    }

    void *novo_dados;
    
    if(!(novo_dados = realloc(buffer->dados, novo_tamanho))){
        perror("Erro ao redimensionar buffer");
        return 0;
    }

    buffer->dados = novo_dados;
    buffer->tamanho = novo_tamanho;

    return 1;
}

void destruir_buffer(struct Buffer *buffer){
    
    if(buffer){
        free(buffer->dados);
        free(buffer);
    }
}

void atualizar_offsets(struct Diretorio *dir){

    if(!dir || !dir->membros){
        return;
    }

    long offset = sizeof(int) + dir->quantidade * sizeof(struct Membro);

    for(int i = 0; i < dir->quantidade; i++){
        if(dir->membros[i]){
            dir->membros[i]->offset = offset;
            offset += dir->membros[i]->tamanho_armazenado;
        }
    }
}

int deslocar_membro(FILE *vc, struct Buffer *buffer, struct Membro *m, long deslocamento){
    
    if(!vc || !buffer || !buffer->dados || !m){
        fprintf(stderr, "Parâmetro nulo em deslocar_membro.\n");
        return 0;
    }

    // Lê os dados do local atual
    if(fseek(vc, m->offset, SEEK_SET) != 0){
        perror("Erro ao posicionar para leitura do membro");
        return 0;
    }

    if(fread(buffer->dados, m->tamanho_armazenado, 1, vc) != 1){
        perror("Erro ao escrever dados do membro deslocado");
        return 0;
    }

    if(fseek(vc, m->offset + deslocamento, SEEK_SET) != 0){
        perror("Erro ao posicionar para escrita do membro");
        return 0;
    }

    if(fwrite(buffer->dados, m->tamanho_armazenado, 1, vc) != 1){
        perror("Erro ao escrever dados do membro deslocado");
        return 0;
    }

    // Atualiza o offset do membro na struct
    m->offset += deslocamento;

    return 1;
}

int ler_diretorio(FILE *vc, struct Diretorio *dir){

    if(!vc || !dir){
        return 0;
    }

    if(fseek(vc, 0, SEEK_SET) != 0){
        perror("Erro ao posicionar ponteiro no início do arquivo");
        return 0;
    }

    if(fread(&(dir->quantidade), sizeof(int), 1, vc) != 1){
        return 0;
    }

    if(!garantir_capacidade_diretorio(dir)){
        fprintf(stderr, "Erro ao garantir capacidade do diretório\n");
        return 0;
    }

    // Aloca e lê cada membro individualmente
    for(int i = 0; i < dir->quantidade; i++){

        if(!(dir->membros[i] = malloc(sizeof(struct Membro)))){
            perror("Erro ao alocar membro");
            return 0;
        }

        if(fread(dir->membros[i], sizeof(struct Membro), 1, vc) != 1){
            perror("Erro ao ler dados do membro");
            return 0;
        }
    }

    return 1;
}

int escrever_diretorio(FILE *vc, const struct Diretorio *dir){

    if (!vc || !dir) {
        fprintf(stderr, "Arquivo ou diretório nulo na escrita.\n");
        return 0;
    }

    // Posiciona no início do arquivo
    if(fseek(vc, 0, SEEK_SET) != 0){
        perror("Erro ao posicionar ponteiro no início do arquivo");
        return 0;
    }

    // Escreve quantidade de membros
    if(fwrite(&(dir->quantidade), sizeof(int), 1, vc) != 1){
        perror("Erro ao salvar quantidade de membros");
        return 0;
    }

    // Escreve os membros
    for(int i = 0; i < dir->quantidade; i++){
        if(fwrite(dir->membros[i], sizeof(struct Membro), 1, vc) != 1){
            perror("Erro ao salvar membros");
            return 0;
        }
    }

    return 1;
}

int extrair_membros(FILE *vc, struct Diretorio *dir, char **nomes, int num_nomes) {
    return 0;
}
