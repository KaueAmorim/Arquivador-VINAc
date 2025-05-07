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
    for(int i = 0; i < dir->quantidade; i++){
        if(dir->membros[i].tamanho_original > buffer->tamanho){
            buffer->tamanho = dir->membros[i].tamanho_original;
        }
    }

    if(!(buffer->dados = malloc(buffer->tamanho))){
        perror("Erro ao alocar memória para buffer");
        return NULL;
    }

    return buffer;
}

void destruir_buffer(struct Buffer *buffer){
    
    if(buffer){
        free(buffer->dados);
        free(buffer);
    }
}

void atualizar_offsets(struct Diretorio *dir){

    long offset = sizeof(int) + dir->quantidade * sizeof(struct Membro);

    for(int i = 0; i < dir->quantidade; i++){
        dir->membros[i].offset = offset;
        offset += dir->membros[i].tamanho_armazenado;
    }
}

int deslocar_membro(FILE *vc, struct Buffer *buffer, struct Membro *m, long deslocamento){
    
    if(!vc || !buffer || !m){
        fprintf(stderr, "Parâmetro nulo em deslocar_membro.\n");
        return 0;
    }

    // Lê os dados do local atual
    if(fseek(vc, m->offset, SEEK_SET) != 0){
        perror("Erro ao posicionar para leitura do membro");
        return 0;
    }

    if(fread(buffer->dados, 1, m->tamanho_armazenado, vc) != m->tamanho_armazenado){
        perror("Erro ao escrever dados do membro deslocado");
        return 0;
    }

    if(fseek(vc, m->offset + deslocamento, SEEK_SET) != 0){
        perror("Erro ao posicionar para escrita do membro");
        return 0;
    }

    if(fwrite(buffer->dados, 1, m->tamanho_armazenado, vc) != m->tamanho_armazenado){
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

    fseek(vc, 0, SEEK_SET);

    if(fread(&(dir->quantidade), sizeof(int), 1, vc) != 1){
        perror("Erro ao ler quantidade de membros");
        return 0;
    }

    if(!garantir_capacidade_diretorio(dir)){
        fprintf(stderr, "Erro ao garantir capacidade do diretório\n");
        return 0;
    }

    if(fread(dir->membros, sizeof(struct Membro), dir->quantidade, vc) != (size_t)dir->quantidade){
        perror("Erro ao ler membros do diretório");
        return 0;
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
    if(fwrite(dir->membros, sizeof(struct Membro), dir->quantidade, vc) != (size_t)dir->quantidade){
        perror("Erro ao salvar membros");
        return 0;
    }

    return 1;
}
int escrever_membros(FILE *vc, struct Diretorio *dir){

    if(!vc || !dir){
        return 0;
    }

    size_t max_tamanho = 0;
    for(int i = 0; i < dir->quantidade; i++){
        if(dir->membros[i].tamanho_original > max_tamanho){
            max_tamanho = dir->membros[i].tamanho_original;
        }
    }

    char *buffer_original = malloc(max_tamanho);
    if(!buffer_original){
        perror("Erro ao alocar buffer de leitura");
        return 0;
    }

    // Buffer de saída para compressão (caso necessário)
    size_t max_saida = (size_t)(max_tamanho * 1.004) + 1;
    char *buffer_saida = malloc(max_saida);
    if (!buffer_saida) {
        perror("Erro ao alocar buffer de compressão");
        free(buffer_original);
        return 0;
    }
    
    for(int i = 0; i < dir->quantidade; i++){
        
        struct Membro *m = &dir->membros[i];

        FILE *file = fopen(m->nome, "r");
        if(!file){
            fprintf(stderr, "Erro ao abrir '%s' para leitura\n", m->nome);
            free(buffer_original);
            free(buffer_saida);
            return 0;
        }

        // Lê conteúdo original
        if(fread(buffer_original, 1, m->tamanho_original, file) != (size_t)m->tamanho_original){
            perror("Erro ao ler dados do arquivo");
            fclose(file);
            free(buffer_original);
            free(buffer_saida);
            return 0;
        }

        fclose(file);

        char *dados_para_escrever = buffer_original;
        int bytes_a_escrever = m->tamanho_original;

        if(m->tamanho_armazenado != m->tamanho_original){
            int comprimido = LZ_Compress((unsigned char *)buffer_original, (unsigned char *)buffer_saida, m->tamanho_original);

            if(comprimido <= 0){
                fprintf(stderr, "Erro ao comprimir membro '%s'\n", m->nome);
                free(buffer_original);
                free(buffer_saida);
                return 0;
            }

            dados_para_escrever = buffer_saida;
            bytes_a_escrever = comprimido;

            if(comprimido != m->tamanho_armazenado){
                fprintf(stderr, "Aviso: tamanho comprimido divergente do esperado para '%s'\n", m->nome);
            }
        }

        fseek(vc, m->offset, SEEK_SET);
        if (fwrite(dados_para_escrever, 1, bytes_a_escrever, vc) != (size_t)bytes_a_escrever) {
            perror("Erro ao escrever dados no archive");
            free(buffer_original);
            free(buffer_saida);
            return 0;
        }
    }

    free(buffer_original);
    free(buffer_saida);

    return 1;
}

int extrair_membros(FILE *vc, struct Diretorio *dir, char **nomes, int num_nomes) {
    return 0;
}
