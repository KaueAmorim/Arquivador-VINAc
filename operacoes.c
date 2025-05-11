#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
    
    struct Comando cmd = {OP_INVALIDA, NULL, NULL, 0, NULL};

    if(argc < 3){
        fprintf(stderr, "Uso: vina <opção> <arquivo.vc> [membros...]\n");
        exit(1);
    }

    cmd.op = identificar_operacao(argv[1]);

    if(cmd.op == OP_INVALIDA){
        fprintf(stderr, "Opção inválida: %s\n", argv[1]);
        exit(1);
    }

    if(cmd.op == OP_MOVER){
        if(argc < 5){
            fprintf(stderr, "Uso para mover: vina -m <membro_target> <arquivo.vc> <membro_mover>\n");
            exit(1);
        }

        cmd.target = argv[2];
        cmd.arquivo_vc = argv[3];
        cmd.num_membros = 1;
        cmd.membros = malloc(sizeof(char *));
        cmd.membros[0] = argv[4];

        return cmd;
    }

    cmd.arquivo_vc = argv[2];

    if(cmd.op == OP_LISTAR){
        return cmd;
    }

    cmd.num_membros = argc - 3;

    if(cmd.num_membros == 0 && cmd.op == OP_EXTRAIR){
        return cmd;
    }

    // Demais comandos: -ip, -ic, -x, -r
    if(argc < 4){
        fprintf(stderr, "Uso: vina <opção> <arquivo.vc> [membros...]\n");
        exit(1);
    }

    cmd.membros = malloc(sizeof(char *) * cmd.num_membros);
    for(int i = 0; i < cmd.num_membros; i++){
        cmd.membros[i] = argv[i + 3];
    }

    return cmd;
}

void executar_insercao_plana(FILE *vc, struct Diretorio *dir, struct Comando *cmd, struct Buffer *buffer){

    if(!dir || !cmd || !buffer){
        fprintf(stderr, "Parâmetro nulo em executar_insercao_plana.\n");
        return;
    }
    
    for(int i = 0; i < cmd->num_membros; i++) {

        struct Membro *existente = buscar_membro(dir, cmd->membros[i]);

        if(existente){

            size_t tamanho_antigo = existente->tamanho_armazenado;

            if(!(atualizar_membro(existente, cmd->membros[i]))){
                fprintf(stderr, "Erro ao atualizar membro: %s\n", cmd->membros[i]);
                continue;
            }

            if(!redimensionar_buffer(buffer, existente->tamanho_armazenado)){
                fprintf(stderr, "Erro ao garantir espaço no buffer para %s\n", existente->nome);
                continue;
            }

            long diff = existente->tamanho_armazenado - tamanho_antigo;

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
                fprintf(stderr, "Erro ao abrir %s para leitura\n", existente->nome);
            }

            if(diff < 0){
                ftruncate(fileno(vc), dir->membros[dir->quantidade - 1]->offset + dir->membros[dir->quantidade - 1]->tamanho_armazenado);
            }

            printf("Membro substituído plano: %s\n", existente->nome);
        } 
        else{

            struct Membro *novo = criar_membro(cmd->membros[i], dir->quantidade);
            if(!novo){
                fprintf(stderr, "Erro ao criar novo membro: %s\n", cmd->membros[i]);
                continue;
            }

            if(!redimensionar_buffer(buffer, novo->tamanho_armazenado)){
                fprintf(stderr, "Erro ao garantir espaço no buffer para %s\n", novo->nome);
                free(novo);
                continue;
            }

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

            printf("Membro inserido plano: %s\n", novo->nome);
        }
    }
}

void executar_insercao_comprimida(FILE *vc, struct Diretorio *dir, struct Comando *cmd, struct Buffer *buffer){
    
    if(!dir || !cmd || !buffer){
        fprintf(stderr, "Parâmetro nulo em executar_insercao_comprimida.\n");
        return;
    }
    
    for(int i = 0; i < cmd->num_membros; i++) {

        struct Membro *existente = buscar_membro(dir, cmd->membros[i]);

        struct Membro *novo = criar_membro(cmd->membros[i], dir->quantidade);
        if(!novo){
            fprintf(stderr, "Erro ao criar novo membro: %s\n", cmd->membros[i]);
            continue;
        }

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
        int tamanho_comprimido = LZ_Compress(buffer->dados, output, (unsigned int)novo->tamanho_original);
        
        // Compressão foi ineficiente — armazenar plano
        if((size_t)tamanho_comprimido >= novo->tamanho_original){
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

            printf("Membro substituído comprimido: %s\n", existente->nome);
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

            printf("Membro inserido comprimido: %s\n", novo->nome);
        }

        free(output);
    }
}

void executar_movimentacao(FILE *vc, struct Diretorio *dir, struct Comando *cmd, struct Buffer *buffer){
    
    if(!dir || !cmd || !buffer){
        fprintf(stderr, "Parâmetro nulo em executar_movimentacao.\n");
        return;
    }
    
    struct Membro *mover = buscar_membro(dir, cmd->membros[0]);
    struct Membro *target;

    if(!mover){
        fprintf(stderr, "Erro: membro %s a mover não encontrado\n", cmd->membros[0]);
        return;
    }

    int ordem_target = -1; // padrão: mover para o início

    if(strcmp(cmd->target, "NULL") != 0){

        target = buscar_membro(dir, cmd->target);
        if(!target){
            fprintf(stderr, "Erro: membro target %s não encontrado\n", cmd->target);
            return;
        }

        ordem_target = target->ordem;

        if(mover->ordem == ordem_target || mover->ordem == ordem_target + 1){
            printf("Membro %s já está na posição desejada\n", mover->nome);
            return;
        }
    } 
    else{
        if(mover->ordem == 0){
            printf("Membro %s já está no início\n", mover->nome);
            return;
        }
    }

    unsigned char *aux;
    if(!(aux = malloc(mover->tamanho_armazenado))){
        perror("Erro ao alocar memória para descompressão");
        return;
    }

    if(fseek(vc, mover->offset, SEEK_SET) != 0){
        perror("Erro ao posicionar no archive");
        free(aux);
        return;
    }

    if(fread(aux, mover->tamanho_armazenado, 1, vc) != 1){
        fprintf(stderr, "Erro ao ler dados do membro %s\n", mover->nome);
        free(aux);
        return;
    }

    // Desloca o membro para a nova posição
    if(mover->ordem > ordem_target){
        for(int i = mover->ordem - 1; i > ordem_target; i--){
            deslocar_membro(vc, buffer, dir->membros[i], mover->tamanho_armazenado);
        }
    } 
    else{
        for(int i = mover->ordem + 1; i <= ordem_target; i++){
            deslocar_membro(vc, buffer, dir->membros[i], -(mover->tamanho_armazenado));
        }
    }

    mover_membro(dir, mover->ordem, ordem_target);
    atualizar_offsets(dir);
    fseek(vc, 0, SEEK_SET);
    escrever_diretorio(vc, dir);

    // Escreve o membro na nova posição
    if(ordem_target == -1){
        if(fseek(vc, sizeof(int) + dir->quantidade * sizeof(struct Membro), SEEK_SET) != 0){
            perror("Erro ao posicionar para escrita do membro");
            free(aux);
            return;
        }

        printf("Membro %s movido para o início\n", mover->nome);
    } 
    else{
        if(fseek(vc, target->offset + target->tamanho_armazenado, SEEK_SET) != 0){
            perror("Erro ao posicionar para escrita do membro");
            free(aux);
            return;
        }

        printf("Membro %s movido para a frente do membro %s\n", mover->nome, target->nome);
    }

    if(fwrite(aux, mover->tamanho_armazenado, 1, vc) != 1){
        fprintf(stderr, "Erro ao escrever dados do membro '%s'\n", mover->nome);
        free(aux);
        return;
    }
    
    free(aux);
}

void executar_extracao(FILE *vc, struct Diretorio *dir, struct Comando *cmd, struct Buffer *buffer){
    
    if(!dir || !cmd || !buffer){
        fprintf(stderr, "Parâmetro nulo em executar_extracao.\n");
        return;
    }
    
    int extrair_todos = (cmd->num_membros == 0);

    for(int i = 0; i < dir->quantidade; i++){
        
        struct Membro *m = dir->membros[i];

        if(!extrair_todos){
            int encontrado = 0;
            for(int j = 0; j < cmd->num_membros; j++){
                if(strcmp(m->nome, cmd->membros[j]) == 0){
                    encontrado = 1;
                    break;
                }
            }
            if(!encontrado){
                continue;
            }
        }

        if(fseek(vc, m->offset, SEEK_SET) != 0){
            perror("Erro ao posicionar no archive");
            continue;
        }

        if(fread(buffer->dados, m->tamanho_armazenado, 1, vc) != 1){
            fprintf(stderr, "Erro ao ler dados do membro '%s'\n", m->nome);
            continue;
        }

        FILE *saida = fopen(m->nome, "wb");
        if(!saida){
            perror("Erro ao criar arquivo extraído");
            continue;
        }

        if(m->tamanho_armazenado != m->tamanho_original){
            
            // Membro comprimido → descomprimir
            unsigned char *saida_descomprimida;
            if(!(saida_descomprimida = malloc(m->tamanho_original))){
                perror("Erro ao alocar memória para descompressão");
                fclose(saida);
                continue;
            }

            LZ_Uncompress(buffer->dados, saida_descomprimida, m->tamanho_armazenado);
            
            if(fwrite(saida_descomprimida, m->tamanho_original, 1, saida) != 1){
                fprintf(stderr, "Erro ao escrever dados no arquivo '%s'\n", m->nome);
            }

            free(saida_descomprimida);

            printf("Membro descomprimido e extraído: %s\n", m->nome);
        } 
        else{
            
            // Membro plano → escrever direto
            if(fwrite(buffer->dados, m->tamanho_armazenado, 1, saida) != 1){
                fprintf(stderr, "Erro ao escrever dados no arquivo '%s'\n", m->nome);
            }

            printf("Membro extraído: %s\n", m->nome);
        }

        fclose(saida);
    }
}

void executar_remocao(FILE *vc, struct Diretorio *dir, struct Comando *cmd, struct Buffer *buffer){
    
    if(!dir || !cmd || !buffer){
        fprintf(stderr, "Parâmetro nulo em executar_remocao.\n");
        return;
    }
    
    for(int i = 0; i < cmd->num_membros; i++){
        
        struct Membro *m = buscar_membro(dir, cmd->membros[i]);
        if(!m){
            fprintf(stderr, "Membro %s não encontrado\n", cmd->membros[i]);
            continue;
        }

        size_t tamanho_remocao = m->tamanho_armazenado;

        char nome_copia[1025];
        strncpy(nome_copia, m->nome, sizeof(nome_copia));
        nome_copia[1024] = '\0';

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
            ftruncate(fileno(vc), sizeof(int));
        }

        // Alocação de um buffer para o maior membro presente no archive
        if(tamanho_remocao == buffer->tamanho){

            buffer->tamanho = 1;

            for(int i = 0; i < dir->quantidade; i++){
                if(dir->membros[i] && dir->membros[i]->tamanho_armazenado > buffer->tamanho){
                    buffer->tamanho = dir->membros[i]->tamanho_armazenado;
                }
            }

            if(!redimensionar_buffer(buffer, buffer->tamanho)){
                fprintf(stderr, "Erro ao redimensionar buffer para %zu bytes\n", buffer->tamanho);
            }
        }

        printf("Membro removido: %s\n", nome_copia);
    }
}

void executar_listagem(struct Diretorio *dir){
    
    if(!dir){
        fprintf(stderr, "Parâmetro nulo em executar_listagem.\n");
        return;
    }
    
    printf("=== Conteúdo do Diretório ===\n");

    if(dir->quantidade == 0){
        printf("[Diretório vazio]\n");
        return;
    }
    
    for(int i = 0; i < dir->quantidade; i++){
        imprimir_membro(dir->membros[i]);
    }
}