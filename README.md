# Simulador de Memória Virtual

Simulador em C de uma Unidade de Gerenciamento de Memória (**MMU**) com paginação sob demanda, **TLB** (*Translation Lookaside Buffer*) de 16 entradas consultada em paralelo via **POSIX Threads (`pthreads`)** e políticas de substituição **FIFO** e **LRU** configuráveis de forma independente para a memória física e para a TLB.

---

## Arquitetura do Sistema Simulado

| Componente | Especificação |
| :--- | :--- |
| **Espaço de Endereçamento Lógico** | 16 bits (`0` a `65535` — 256 páginas de 256 bytes) |
| **Memória Física (RAM)** | 32 KB (128 *frames* de 256 bytes) |
| **Tamanho da Página / Frame** | 256 bytes (`8` bits de *offset*) |
| **TLB** | 16 entradas com busca concorrente (1 thread por entrada) |
| **Armazenamento Secundário** | `BACKING_STORE.bin` (65.536 bytes) |
| **Políticas de Substituição** | `fifo` (*First-In, First-Out*) e `lru` (*Least Recently Used*) |



## Estrutura do Projeto

```text
.
├── vm.c               # Código-fonte principal (MMU, TLB multithread, FIFO/LRU e parser)
├── Makefile           # Automação da compilação
├── BACKING_STORE.bin  # Arquivo binário simulando o disco (256 páginas x 256 bytes)
├── addresses.txt      # Arquivo de entrada com endereços virtuais de 16 bits 
funcionais e casos de erro
└── correct.txt        # Arquivo de saída gerado após cada execução
```

---

## Compilação e Execução

### 1. Compilar o projeto
Utilize o `Makefile` na raiz do repositório:

```bash
make
```

*(Opcional) Para remover o binário compilado e arquivos temporários:*

```bash
make clean
```

### 2. Executar o simulador
O programa recebe três argumentos obrigatórios: o arquivo de endereços, o algoritmo de substituição de páginas na RAM e o algoritmo de substituição da TLB:

```bash
./vm <arquivo_enderecos> <algoritmo_pagina> <algoritmo_tlb>
```

**Exemplos de execução:**

```bash
# FIFO na RAM e FIFO na TLB
./vm addresses.txt fifo fifo

# LRU na RAM e LRU na TLB
./vm addresses.txt lru lru

# LRU na RAM e FIFO na TLB
./vm addresses.txt lru fifo

# FIFO na RAM e LRU na TLB
./vm addresses.txt fifo lru
```

---


## Formato da Saída (`correct.txt`)

Para cada endereço virtual lido, o programa grava a tradução correspondente e o valor do byte assinado (`-128` a `127`) armazenado na memória física, seguido das estatísticas finais da execução:

```text
Virtual address: 16916 TLB: 0 Physical address: 20 Value: 0
...
Number of Translated Addresses = 1000
Page Faults = 539
Page Fault Rate = 0.539
TLB Hits = 54
TLB Hit Rate = 0.054
```

---

## Autor

**Juan Riquelme**
## Ambiente de Desenvolvimento

* **Sistema Operacional:** Windows com **WSL** (*Windows Subsystem for Linux* — Ubuntu)
* **Compilador:** GCC com suporte a POSIX Threads (`-pthread`)
* **Build e Automação:** GNU Make e Bash

---