
# VINAc – Arquivador Binário com Compressão em C

**VINAc** (VINA Compactado) é um arquivador binário desenvolvido em C puro, que permite armazenar múltiplos arquivos em um único arquivo `.vc` com suporte a compressão, manipulação por comandos e gerenciamento de diretório interno.

---

## 🎯 Objetivo do Projeto

O objetivo do VINAc é simular um utilitário semelhante ao `tar`, com uma estrutura binária de diretório personalizada, operações diversas por linha de comando e suporte opcional à compressão via algoritmo LZ77 simplificado.

Este projeto foi pensado para exercitar habilidades como:

- Manipulação de arquivos binários
- Implementação de estruturas de dados dinâmicas
- Escrita de código modular e reutilizável
- Gerenciamento de memória e buffers
- Interpretação de comandos e parsing de argumentos

---

## 🚀 Funcionalidades Suportadas

| Operação          | Descrição                                                                 |
|-------------------|---------------------------------------------------------------------------|
| `-ip`             | Insere arquivos no archive sem compressão                                 |
| `-ic`             | Insere arquivos no archive com compressão via LZ77                        |
| `-x`              | Extrai arquivos do archive para o sistema de arquivos                     |
| `-l`              | Lista todos os membros presentes no archive                               |
| `-r`              | Remove membros específicos do archive                                     |
| `-m`              | Move um ou mais membros para uma nova posição dentro do archive           |
| `-z`              | Deriva um novo arquivo `.vc` contendo apenas um subconjunto dos membros   |

---

## 🧱 Como o VINAc Funciona

### Estruturas de Dados

- `struct Membro`: representa os metadados de um arquivo (nome, UID, tamanhos, data de modificação, offset, ordem).
- `struct Diretorio`: representa o diretório que armazena os membros do archive.
- `struct Buffer`: área de memória para leitura e escrita de membros no arquivo `.vc`, redimensionada conforme o maior membro.
- `struct Comando`: representa e organiza os argumentos fornecidos via linha de comando.

### 🔹 Estrutura Interna

O arquivo `.vc` é composto por duas regiões principais:

1. **Diretório Inicial** – Contém metadados de cada membro (nome, tamanho original, tamanho armazenado, compressão, ordem e offset).
2. **Dados dos Membros** – Conteúdo real dos arquivos, armazenado de forma sequencial, possivelmente comprimido.

A manipulação dos membros depende de operações como `fseek`, `fread`, `fwrite` e `ftruncate`. Os offsets são recalculados dinamicamente conforme membros são adicionados, removidos ou deslocados.

### 🔹 Diretório Dinâmico

O diretório é representado em memória como um vetor de ponteiros para structs `Membro`. Ele é alocado dinamicamente e pode crescer via `realloc`, conforme arquivos são inseridos.

---

## 📁 Arquivos e Diretórios do Projeto

| Arquivo           | Descrição breve |
|-------------------|------------------|
| `main.c`          | Função principal `main()`, responsável por abrir o arquivo `.vc`, interpretar comandos e chamar as operações adequadas. |
| `archive.c/.h`    | Manipulação do arquivo `.vc` como um todo: leitura/escrita do diretório, deslocamento de membros, atualização dos offsets, criação e ajuste de buffers. |
| `diretorio.c/.h`  | Gerenciamento lógico do diretório de membros, incluindo adição, remoção, busca e movimentação dos membros. |
| `membro.c/.h`     | Criação, atualização e exibição de metadados de um membro individual (arquivo). |
| `lz.c/.h`         | Implementação da compressão e descompressão usando algoritmo LZ. |
| `operacoes.c/.h`  | Processamento dos comandos passados via linha de comando: inserção, extração, remoção, movimentação e listagem. |
| `Makefile`        | Regras de compilação para gerar o executável `vina`, com suporte às regras `all` e `clean`. |

---

### Decisões Tomadas

- ✅ **Diretório no início do arquivo:** facilita acesso direto e leitura inicial dos metadados.
- ✅ **Struct Diretorio:** Utiliza um vetor dinâmico de ponteiros para `struct Membro`, com controle de tamanho atual (`quantidade`) e tamanho total alocado (`capacidade`). O campo `capacidade` permite redimensionar o vetor com eficiência (via `realloc`), dobrando a capacidade conforme o crescimento, evitando alocações frequentes.
- ✅ **Movimentação lógica e física dos membros:** o vetor de ponteiros é reorganizado, e os dados no arquivo `.vc` são deslocados fisicamente com `fseek` e `fread`/`fwrite`.
- ✅ **Compressão condicional com LZ:** utilizada apenas se for mais eficiente que o tamanho original.
- ✅ **O campo `ordem` dos membros começa em `0`:** coincide com o índice do vetor `dir->membros[]`. Essa decisão evita a necessidade de traduções ou compensações ao acessar ou reordenar membros, além de simplificar os laços e as comparações de posição durante inserção e movimentação.

## ⚙️ Compilação

Para compilar:

```bash
make
```

Para limpar os arquivos gerados:

```bash
make clean
```

---

## ✍️ Exemplos de Uso

```bash
# Inserir arquivos sem compressão
./vina -ip arquivo.vc arquivo1.txt arquivo2.txt

# Listar conteúdo
./vina -l arquivo.vc

# Extrair arquivos
./vina -x arquivo.vc arquivo1.txt

# Remover arquivos
./vina -r arquivo.vc arquivo2.txt

# Mover arquivo1.txt para antes de arquivo2.txt
./vina -m arquivo2.txt arquivo.vc arquivo1.txt

# Criar novo archive com subconjunto
./vina -z arquivo.vc arquivo1.txt arquivo2.txt
```
