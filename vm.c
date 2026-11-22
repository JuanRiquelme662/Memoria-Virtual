#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define ENDERECO_MAX 65535
#define TAM_BUFFER   64

//bites por pagina e por frame
#define TAM_PAGINA   256
//paginas logicas
#define NUM_PAGINAS  256
//frames de memoria fisica
#define NUM_FRAMES   128

#define ARQ_BACKING  "BACKING_STORE.bin"
#define ARQ_SAIDA    "correct.txt"

//memoria fisica num_frames x tam_pagina
unsigned char memoria_fisica[NUM_FRAMES][TAM_PAGINA];
//tabela de paginas, cada entrada contem o numero do frame correspondente a pagina logica, ou -1 se a pagina nao estiver na memoria fisica
int tabela_paginas[NUM_PAGINAS];
//proximo frmae livre
int proximo_frame_livre = 0;    
//funcao de verificacao
//essa operacao e colocada dentro de uma fucnao porque ela e usada para verificar dois argumentos, o 2 e o 3
int algoritmo_valido(const char *s) {
    return strcmp(s, "fifo") == 0 || strcmp(s, "lru") == 0;
}

int converter_endereco(const char *linha, long *endereco) {
    char *fim;

    if (linha[0] < '0' || linha[0] > '9') {
        return -1;
    }

    errno = 0;
    long valor = strtol(linha, &fim, 10);

    if (errno != 0 || fim == linha) {
        return -1;
    }

    while (*fim == ' ' || *fim == '\t') {
        fim++;
    }

    if (*fim != '\0' && *fim != '\n' && *fim != '\r') {
        return -1;
    }
    if (*fim != '\0' && *fim != '\n' && *fim != '\r') {
        return -1;
    }

    if (valor < 0 || valor > ENDERECO_MAX) {
        return -1;
    }

    *endereco = valor;
    return 0;
}

int main(int argc, char **argv) {
    //verificacoes de argumentos
    if (argc != 4) {
        fprintf(stderr, "Erro: quantidade de argumentos invalida.\n");
        fprintf(stderr, "Uso: %s <arquivo_enderecos> <fifo|lru> <fifo|lru>\n", argv[0]);
        return 1;
    }

    if (!algoritmo_valido(argv[2])) {
        fprintf(stderr, "Erro: algoritmo de substituicao de pagina invalido '%s' (use fifo ou lru).\n", argv[2]);
        return 1;
    }

    if (!algoritmo_valido(argv[3])) {
        fprintf(stderr, "Erro: algoritmo de substituicao da TLB invalido '%s' (use fifo ou lru).\n", argv[3]);
        return 1;
    }

    //guarda os argumentos em variaveis para melhor legibilidadeq
    const char *arquivo_enderecos = argv[1];
    const char *algoritmo_pagina  = argv[2];
    const char *algoritmo_tlb     = argv[3];

    //abri o arquivo
    FILE *arquivo = fopen(arquivo_enderecos, "r");
    if (arquivo == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s': %s\n", arquivo_enderecos, strerror(errno));
        return 1;
    }

    char buffer[TAM_BUFFER];
    int numero_linha = 0;

    while (fgets(buffer, sizeof(buffer), arquivo) != NULL) {
        numero_linha++;

        //linha maior que o buffer ou mal formatada
        if (strchr(buffer, '\n') == NULL && !feof(arquivo)) {
            fprintf(stderr, "Erro: linha %d de '%s' muito longa ou mal formatada.\n", numero_linha, arquivo_enderecos);
            fclose(arquivo);
            return 1;
        }

        long endereco;
        if (converter_endereco(buffer, &endereco) != 0) {
            fprintf(stderr, "Erro: formato invalido na linha %d de '%s' (esperado um inteiro de 0 a %d por linha).\n",numero_linha, arquivo_enderecos, ENDERECO_MAX);
            fclose(arquivo);
            return 1;
        }

        long pagina = endereco >> 8;
        long offset = endereco & 0xFF;

        //linha temporaria para teste
        printf("linha %d: endereco=%ld pagina=%ld offset=%ld\n", numero_linha, endereco, pagina, offset);
    }

    if (ferror(arquivo)) {
        fprintf(stderr, "Erro: falha ao ler '%s'.\n", arquivo_enderecos);
        fclose(arquivo);
        return 1;
    }

    fclose(arquivo);
    return 0;
}