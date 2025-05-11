# Nome do executável
EXEC = vina

# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Arquivos-fonte e objetos
SRCS = main.c archive.c diretorio.c lz.c membro.c operacoes.c
OBJS = $(SRCS:.c=.o)

# Regra principal
all: $(EXEC)

# Como construir o executável
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS)

# Regra para compilar arquivos .c em .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra de limpeza
clean:
	rm -f $(OBJS) $(EXEC)

# Evita que arquivos como "clean" e "all" sejam interpretados como arquivos
.PHONY: all clean
