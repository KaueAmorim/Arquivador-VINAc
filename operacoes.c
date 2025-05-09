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

        struct Membro *novo = criar_membro(cmd->membros[i], dir->quantidade);

        if(!redimensionar_buffer(buffer, novo->tamanho_armazenado)){
            fprintf(stderr, "Erro ao garantir espaço no buffer para %s\n", novo->nome);
            free(novo);
            continue;
        }

        if(existente){
            long diff = novo->tamanho_armazenado - existente->tamanho_armazenado;

            free(novo);

            if(diff > 0){
                for(int j = dir->quantidade - 1; j > existente->ordem; j--){
                    deslocar_membro(vc, buffer, dir->membros[j], diff);
                }
            }
            else if(diff < 0){
                for(int j = existente->ordem + 1; j < dir->quantidade; j++){
                    deslocar_membro(vc, buffer, dir->membros[j], diff);
                }
            }

            atualizar_membro(existente, cmd->membros[i]);
            atualizar_offsets(dir);
            fseek(vc, 0, SEEK_SET);
            escrever_diretorio(vc, dir);

            FILE *fp = fopen(existente->nome, "rb");
            if(fp){
                if(fread(buffer->dados, existente->tamanho_armazenado, 1, fp) != 1){
                    perror("Erro ao ler conteúdo do membro substituído");
                }

                fclose(fp);

                fseek(vc, existente->offset, SEEK_SET);
                if(fwrite(buffer->dados, existente->tamanho_armazenado, 1, vc) != 1){
                    perror("Erro ao escrever conteúdo do membro substituído");
                }
            } 
            else{
                fprintf(stderr, "Erro ao abrir %s para leitura\n", novo->nome);
            }

            if(diff < 0){
                ftruncate(fileno(vc), dir->membros[dir->quantidade - 1]->offset + dir->membros[dir->quantidade - 1]->tamanho_armazenado);
            }
        } 
        else{
            if(!adicionar_membro(dir, novo)){
                fprintf(stderr, "Erro ao adicionar membro: %s\n", cmd->membros[i]);
                free(novo);
                continue;
            }

            // Desloca membros para abrir espaço para nova struct Membro no diretório
            for(int j = dir->quantidade - 2; j >= 0; j--){
                deslocar_membro(vc, buffer, dir->membros[j], sizeof(struct Membro));
            }

            atualizar_offsets(dir);
            fseek(vc, 0, SEEK_SET);
            escrever_diretorio(vc, dir);

            // Escreve novo membro no final do arquivo
            FILE *fp = fopen(novo->nome, "rb");
            if(fp){
                if(fread(buffer->dados, novo->tamanho_armazenado, 1, fp) != 1){
                    perror("Erro ao ler conteúdo do novo membro");
                }

                fclose(fp);

                fseek(vc, novo->offset, SEEK_SET);
                if(fwrite(buffer->dados, novo->tamanho_armazenado, 1, vc) != 1){
                    perror("Erro ao escrever conteúdo do novo membro");
                }
            } 
            else{
                fprintf(stderr, "Erro ao abrir %s para leitura\n", novo->nome);
            }
        }
    }
}

void executar_insercao_comprimida(FILE *vc, struct Diretorio *dir, struct Comando *cmd, struct Buffer *buffer){
    
    for(int i = 0; i < cmd->num_membros; i++) {

        struct Membro *existente = buscar_membro(dir, cmd->membros[i]);

        struct Membro *novo = criar_membro(cmd->membros[i], dir->quantidade);

        if(!redimensionar_buffer(buffer, novo->tamanho_original)){
            fprintf(stderr, "Erro ao garantir espaço no buffer para %s\n", novo->nome);
            free(novo);
            continue;
        }

        // Ler o conteúdo original do arquivo
        FILE *fp = fopen(novo->nome, "rb");
        if(!fp){
            fprintf(stderr, "Erro ao abrir %s para leitura\n", novo->nome);
            free(novo);
            continue;
        }

        if(fread(buffer->dados, novo->tamanho_original, 1, fp) != 1){
            perror("Erro ao ler conteúdo do arquivo original");
            fclose(fp);
            free(novo);
            continue;
        }
        fclose(fp);

        // Aloca espaço de saída para compressão
        unsigned char *output;
        if(!(output = malloc((novo->tamanho_original * 1.004) + 1))){
            perror("Erro ao alocar memória para compressão");
            free(novo);
            continue;
        }

        // Comprime os dados
        int tamanho_comprimido = LZ_Compress(buffer->dados, output, novo->tamanho_original);
        
        // Compressão foi ineficiente — armazenar plano
        if(tamanho_comprimido >= novo->tamanho_original){
            free(novo);
            free(output);

            struct Comando unico = {OP_INSERIR_PLANO, cmd->arquivo_vc, &cmd->membros[i], 1, NULL};
            executar_insercao_plana(vc, dir, &unico, buffer);

            continue;
        }
        
        // Compressão foi eficiente — armazenar comprimido
        novo->tamanho_armazenado = tamanho_comprimido;

        if(existente){
            long diff = novo->tamanho_armazenado - existente->tamanho_armazenado;

            if(diff > 0){
                for(int j = dir->quantidade - 1; j > existente->ordem; j--){
                    deslocar_membro(vc, buffer, dir->membros[j], diff);
                }
            }
            else if(diff < 0){
                for(int j = existente->ordem + 1; j < dir->quantidade; j++){
                    deslocar_membro(vc, buffer, dir->membros[j], diff);
                }
            }

            atualizar_membro(existente, cmd->membros[i]);
            existente->tamanho_armazenado = tamanho_comprimido;

            atualizar_offsets(dir);
            fseek(vc, 0, SEEK_SET);
            escrever_diretorio(vc, dir);
            
            fseek(vc, existente->offset, SEEK_SET);
            if(fwrite(output, existente->tamanho_armazenado, 1, vc) != 1){
                perror("Erro ao escrever conteúdo do membro substituído");
            }

            if(diff < 0){
                ftruncate(fileno(vc), dir->membros[dir->quantidade - 1]->offset + dir->membros[dir->quantidade - 1]->tamanho_armazenado);
            }

            free(novo); // porque não foi adicionado ao diretório
        } 
        else{
            if(!adicionar_membro(dir, novo)){
                fprintf(stderr, "Erro ao adicionar membro: %s\n", cmd->membros[i]);
                free(output);
                free(novo);
                continue;
            }

            // Desloca membros para abrir espaço para nova struct Membro no diretório
            for(int j = dir->quantidade - 2; j >= 0; j--){
                deslocar_membro(vc, buffer, dir->membros[j], sizeof(struct Membro));
            }

            atualizar_offsets(dir);
            fseek(vc, 0, SEEK_SET);
            escrever_diretorio(vc, dir);

            fseek(vc, novo->offset, SEEK_SET);
            if(fwrite(output, novo->tamanho_armazenado, 1, vc) != 1){
                perror("Erro ao escrever conteúdo do novo membro");
            }
        }

        free(output);
    }
}

void executar_movimentacao(FILE *vc, struct Diretorio *dir, struct Comando *cmd, struct Buffer *buffer){
    return;
}

void executar_extracao(FILE *vc, struct Diretorio *dir, struct Comando *cmd, struct Buffer *buffer){
    return;
}

void executar_remocao(FILE *vc, struct Diretorio *dir, struct Comando *cmd, struct Buffer *buffer){
    
    for(int i = 0; i < cmd->num_membros; i++){
        
        struct Membro *m = buscar_membro(dir, cmd->membros[i]);

        if(!m){
            fprintf(stderr, "Membro %s não encontrado\n", cmd->membros[i]);
            continue;
        }

        for(int j = 0; j < m->ordem; j++){
            deslocar_membro(vc, buffer, dir->membros[j], -(sizeof(struct Membro)));
        }

        for(int j = m->ordem + 1; j < dir->quantidade; j++){
            deslocar_membro(vc, buffer, dir->membros[j], -(m->tamanho_armazenado + sizeof(struct Membro)));
        }

        if(!remover_membro(dir, m->nome)){
            fprintf(stderr, "Erro ao remover membro %s\n", m->nome);
            continue;
        }

        atualizar_offsets(dir);
        fseek(vc, 0, SEEK_SET);
        escrever_diretorio(vc, dir);

        if(dir->quantidade > 0){
            ftruncate(fileno(vc), dir->membros[dir->quantidade - 1]->offset + dir->membros[dir->quantidade - 1]->tamanho_armazenado);
        }
        else{
            ftruncate(fileno(vc), 0);
        }
    }
}

void executar_listagem(struct Diretorio *dir){
    
    printf("=== Conteúdo do Diretório ===\n");

    if(dir->quantidade == 0){
        printf("[Diretório vazio]\n");
        return;
    }
    
    for(int i = 0; i < dir->quantidade; i++){
        imprimir_membro(dir->membros[i]);
    }
}