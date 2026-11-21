#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//funcao de verificacao
//essa operacao e colocada dentro de uma fucnao porque ela e usada para verificar dois argumentos, o 2 e o 3
int algoritmo_valido(const char *s) {
    return strcmp(s, "fifo") == 0 || strcmp(s, "lru") == 0;
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
    return 0;
}