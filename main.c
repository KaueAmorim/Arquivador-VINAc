#include "membro.h"
#include "diretorio.h"
#include "archive.h"
#include "operacoes.h"

int main(int argc, char **argv){
    
    struct Comando cmd = parse_comando(argc, argv);
    FILE *vc = fopen(cmd.arquivo_vc, "rb+");
    
    if(!vc) {
        // Se não existe, cria novo arquivo
        vc = fopen(cmd.arquivo_vc, "wb+");
        if(!vc) {
            perror("Erro ao abrir/criar arquivo .vc");
            return 1;
        }
    }

    struct Diretorio *dir = criar_diretorio();
    ler_diretorio(vc, dir);

    struct Buffer *buffer = criar_buffer(dir);

    switch(cmd.op){
        case OP_INVALIDA:
            fprintf(stderr, "Opção inválida: %s\n", argv[1]);
            return 1;
        case OP_INSERIR_PLANO:
            printf("Inserção sem compressão\n");
            executar_insercao_plana(vc, dir, &cmd, buffer);
            break;
        case OP_INSERIR_COMPRIMIDO:
            printf("Inserção com compressão\n");
            executar_insercao_comprimida(vc, dir, &cmd, buffer);
            break;
        case OP_MOVER:
            printf("Movimentação\n");
            executar_movimentacao(vc, dir, &cmd, buffer);
            break;
        case OP_EXTRAIR:
            printf("Extração\n");
            executar_extracao(vc, dir, &cmd, buffer);
            break;
        case OP_REMOVER:
            printf("Remoção\n");
            executar_remocao(vc, dir, &cmd, buffer);
            break;
        case OP_LISTAR:
            printf("Listagem\n");
            executar_listagem(dir);
            break;
    }

    fclose(vc);
    destruir_diretorio(dir);
    destruir_buffer(buffer);
    free(cmd.membros);

    return 0;
}