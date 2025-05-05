#include "membro.h"
#include "diretorio.h"
#include "archive.h"
#include "operacoes.h"
#include "lz.h"

int identificar_operacao(const char *arg) {
    if(strcmp(arg, "-ip") == 0) 
        return OP_INSERIR_PLANO;
    if(strcmp(arg, "-ic") == 0) 
        return OP_INSERIR_COMPRIMIDO;
    if(strcmp(arg, "-m") == 0) 
        return OP_MOVER;
    if(strcmp(arg, "-x") == 0) 
        return OP_EXTRAIR;
    if(strcmp(arg, "-r") == 0) 
        return OP_REMOVER;
    if(strcmp(arg, "-c") == 0) 
        return OP_LISTAR;
    return OP_INVALIDA;
}

struct Comando parse_comando(int argc, char **argv) {
    
    struct Comando cmd;
    
    cmd.op = OP_INVALIDA;
    cmd.membros = NULL;
    cmd.num_membros = 0;
    cmd.target = NULL;

    if(argc < 3) {
        fprintf(stderr, "Uso: vinac <opção> <arquivo.vc> [membros...]\n");
        return cmd;
    }

    cmd.op = identificar_operacao(argv[1]);
    cmd.arquivo_vc = argv[2];

    if(cmd.op != OP_LISTAR) {
        
        cmd.num_membros = argc - 3;
        
        if(cmd.num_membros > 0) {
            cmd.membros = malloc(sizeof(char *) * cmd.num_membros);
            
            for(int i = 0; i < cmd.num_membros; i++) {
                cmd.membros[i] = argv[i + 3];
            }
        }
    }

    return cmd;
}

void executar_insercao_plana(FILE *vc, struct Diretorio *dir, struct Comando *cmd, struct Buffer *buffer){
    
    for(int i = 0; i < cmd->num_membros; i++) {

        struct Membro *existente = buscar_membro(dir, cmd->membros[i]);

        struct Membro novo = criar_membro(cmd->membros[i], dir->quantidade);

        if(novo.tamanho_armazenado > buffer->tamanho){
            buffer->tamanho = novo.tamanho_armazenado;

            if(!(buffer->dados = realloc(buffer->dados, buffer->tamanho))) {
                perror("Erro ao alocar memória para buffer");
                return;
            }
        }

        if(existente){
            long diff = novo.tamanho_armazenado - existente->tamanho_armazenado;

            if(diff > 0){
                for(int j = dir->quantidade - 1; j > existente->ordem; j--){
                    deslocar_membro(vc, buffer, &dir->membros[j], diff);
                }
            }
            else if (diff < 0){
                for(int j = existente->ordem + 1; j < dir->quantidade; j++){
                    deslocar_membro(vc, buffer, &dir->membros[j], diff);
                }
            }

            atualizar_membro(existente, cmd->membros[i]);
            atualizar_offsets(dir);
            fseek(vc, 0, SEEK_SET);
            escrever_diretorio(vc, dir);

            FILE *fp = fopen(existente->nome, "rb");
            if(fp){
                if(fread(buffer->dados, 1, existente->tamanho_armazenado, fp) != existente->tamanho_armazenado){
                    perror("Erro ao ler conteúdo do novo membro");
                }

                fclose(fp);

                fseek(vc, existente->offset, SEEK_SET);
                if(fwrite(buffer->dados, 1, existente->tamanho_armazenado, vc) != existente->tamanho_armazenado){
                    perror("Erro ao escrever conteúdo do novo membro");
                }
            } 
            else{
                fprintf(stderr, "Erro ao abrir %s para leitura\n", novo.nome);
            }

        } 
        else{
            if(!adicionar_membro(dir, novo)){
                fprintf(stderr, "Erro ao adicionar membro: %s\n", cmd->membros[i]);
                continue;
            }

            fseek(vc, 0, SEEK_SET);

            // Desloca membros para abrir espaço para nova struct Membro no diretório
            for(int j = dir->quantidade - 2; j >= 0; j--){
                fprintf(stderr, "Movendo membro: %s\n", dir->membros[j].nome);
                deslocar_membro(vc, buffer, &dir->membros[j], sizeof(struct Membro));
            }

            atualizar_offsets(dir);
            imprimir_membro(&(dir->membros[dir->quantidade - 1]));
            fseek(vc, 0, SEEK_SET);
            escrever_diretorio(vc, dir);
            novo = dir->membros[dir->quantidade - 1];

            // Escreve novo membro no final do arquivo
            FILE *fp = fopen(novo.nome, "rb");
            if(fp){
                if(fread(buffer->dados, 1, novo.tamanho_armazenado, fp) != novo.tamanho_armazenado){
                    perror("Erro ao ler conteúdo do novo membro");
                }

                fclose(fp);

                printf("Offset: %ld\n", novo.offset);

                fseek(vc, novo.offset, SEEK_SET);
                if(fwrite(buffer->dados, 1, novo.tamanho_armazenado, vc) != novo.tamanho_armazenado){
                    perror("Erro ao escrever conteúdo do novo membro");
                }
            } 
            else{
                fprintf(stderr, "Erro ao abrir %s para leitura\n", novo.nome);
            }
        }
    }
}

void executar_insercao_comprimida(FILE *vc, struct Diretorio *dir, struct Comando *cmd){
    
    for(int i = 0; i < cmd->num_membros; i++){

        struct Membro novo = criar_membro(cmd->membros[i], dir->quantidade);

        FILE *file = fopen(cmd->membros[i], "r");
        if(!file){
            fprintf(stderr, "Erro ao abrir '%s'\n", cmd->membros[i]);
            return;
        }

        unsigned char *input = malloc(novo.tamanho_original);
        if(!input){
            perror("Erro ao alocar buffer de entrada");
            return;
        }
        
        fread(input, novo.tamanho_original, 1, file);
        fclose(file);
        
        size_t max_saida = (size_t)(novo.tamanho_original * 1.004) + 1;
        unsigned char *output = malloc(max_saida);
        if(!output){
            perror("Erro ao alocar buffer de saída");
            free(input);
            return;
        }

        int tamanho_comprimido = LZ_Compress(input, output, novo.tamanho_original);
        free(input);

        if(tamanho_comprimido <= 0){
            fprintf(stderr, "Erro ao comprimir '%s'\n", cmd->membros[i]);
            free(output);
            return;
        }

        if(tamanho_comprimido >= novo.tamanho_original) {
            // Compressão não valeu a pena — armazenar plano
            novo.tamanho_armazenado = novo.tamanho_original;
        } 
        else{
            // Compressão foi eficiente — armazenar comprimido
            novo.tamanho_armazenado = tamanho_comprimido;
        }

        if(!adicionar_membro(dir, novo)) {
            fprintf(stderr, "Erro ao inserir membro '%s'\n", cmd->membros[i]);
            free(output);
            return;
        }

        free(output);
    }

    atualizar_offsets(dir);
    fseek(vc, 0, SEEK_SET);
    escrever_diretorio(vc, dir);
    escrever_membros(vc, dir);
}

void executar_movimentacao(FILE *vc, struct Diretorio *dir, struct Comando *cmd){
    return;
}

void executar_extracao(FILE *vc, struct Diretorio *dir, struct Comando *cmd){
    return;
}

void executar_remocao(FILE *vc, struct Diretorio *dir, struct Comando *cmd){
    
    int removidos = 0;

    for(int i = 0; i < cmd->num_membros; i++){
        if(remover_membro(dir, cmd->membros[i])){
            printf("Membro removido: %s\n", cmd->membros[i]);
            removidos++;
        } 
        else{
            printf("Membro não encontrado: %s\n", cmd->membros[i]);
        }
    }

    if(removidos > 0){
        atualizar_offsets(dir);

        fseek(vc, 0, SEEK_SET);
        if(!escrever_diretorio(vc, dir)){
            fprintf(stderr, "Erro ao salvar diretório após remoção.\n");
        }
    } 
    else{
        printf("Nenhum membro foi removido.\n");
    }
}

void executar_listagem(struct Diretorio *dir){
    
    printf("=== Conteúdo do Diretório ===\n");

    if(dir->quantidade == 0){
        printf("[Diretório vazio]\n");
        return;
    }
    
    for(int i = 0; i < dir->quantidade; i++){
        imprimir_membro(&dir->membros[i]);
    }
}