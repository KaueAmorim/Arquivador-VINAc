# Nome do executável
EXEC = vinac

# Compilador e flags
CC = gcc

# Arquivos-fonte
SRC = main.c membro.c diretorio.c archive.c operacoes.c lz.c
OBJ = $(SRC:.c=.o)

# Regra principal
all: $(EXEC)

# Regra para gerar o executável
$(EXEC): $(OBJ)
	$(CC) -o $@ $^

# Compilação de .c para .o
%.o: %.c
	$(CC) -c $< -o $@

# Limpeza dos arquivos gerados
clean:
	rm -f $(EXEC) *.o arquivo.vc

.PHONY: all clean

