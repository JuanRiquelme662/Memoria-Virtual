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
#define TAM_TLB 16

typedef struct {
    int  pagina;
    int  frame;
    int  valida;
    long carregada_em;
    long ultimo_uso;
} EntradaTLB;

EntradaTLB tlb[TAM_TLB];


//memoria fisica num_frames x tam_pagina
unsigned char memoria_fisica[NUM_FRAMES][TAM_PAGINA];
//tabela de paginas, cada entrada contem o numero do frame correspondente a pagina logica, ou -1 se a pagina nao estiver na memoria fisica
int tabela_paginas[NUM_PAGINAS];
//proximo frmae livre
int proximo_frame_livre = 0;    

//metadados de cada frame
//qual pagina esta cada frame
int  frame_para_pagina[NUM_FRAMES];
//quando o frame foi carregado
long frame_carregado_em[NUM_FRAMES];
//quando o frame foi usado pela ultima vez
long frame_ultimo_uso[NUM_FRAMES];    

//relogio logico
long relogio = 0;
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

    if (valor < 0 || valor > ENDERECO_MAX) {
        return -1;
    }

    *endereco = valor;
    return 0;
}

int escolher_vitima(const char *algoritmo) {
    const long *criterio;

    if (strcmp(algoritmo, "fifo") == 0) {
        criterio = frame_carregado_em;
    } else {
        criterio = frame_ultimo_uso;
    }

    int vitima = 0;
    for (int i = 1; i < NUM_FRAMES; i++) {
        if (criterio[i] < criterio[vitima]) {
            vitima = i;
        }
    }

    return vitima;
}

//funcoes de manipulacao da TLB
int buscar_tlb(int pagina) {
    for (int i = 0; i < TAM_TLB; i++) {
        if (tlb[i].valida && tlb[i].pagina == pagina) {
            return i;
        }
    }
    return -1;
}

//Insere a página na TLB e retorna a posição usada
int inserir_tlb(int pagina, int frame, const char *algoritmo) {
    int posicao = -1;

    for (int i = 0; i < TAM_TLB; i++) {
        if (!tlb[i].valida) {
            posicao = i;
            break;
        }
    }

    if (posicao == -1) {
        int usa_fifo = (strcmp(algoritmo, "fifo") == 0);

        posicao = 0;
        for (int i = 1; i < TAM_TLB; i++) {
            long atual = usa_fifo ? tlb[i].carregada_em       : tlb[i].ultimo_uso;
            long menor = usa_fifo ? tlb[posicao].carregada_em : tlb[posicao].ultimo_uso;
            if (atual < menor) {
                posicao = i;
            }
        }
    }

    tlb[posicao].pagina       = pagina;
    tlb[posicao].frame        = frame;
    tlb[posicao].valida       = 1;
    tlb[posicao].carregada_em = relogio;
    tlb[posicao].ultimo_uso   = relogio;

    return posicao;
}

void invalidar_tlb(int pagina) {
    int posicao = buscar_tlb(pagina);

    if (posicao != -1) {
        tlb[posicao].valida = 0;
    }
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

    //recursos declarados aqui para o goto fim libera-;los
    FILE *arquivo = NULL; 
    FILE *backing = NULL;
    FILE *saida   = NULL;   
    char buffer[TAM_BUFFER];
    int numero_linha = 0;
    int total_enderecos = 0;
    int page_faults = 0;
    int status = 0;
    int tlb_hits = 0;

    arquivo = fopen(arquivo_enderecos, "r");
    if (arquivo == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s': %s\n", arquivo_enderecos, strerror(errno));
        status = 1;
        goto fim;
    }

    backing = fopen(ARQ_BACKING, "rb");
    if (backing == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s': %s\n", ARQ_BACKING, strerror(errno));
        status = 1;
        goto fim;
    }

    saida = fopen(ARQ_SAIDA, "w");
    if (saida == NULL) {
        fprintf(stderr, "Erro: nao foi possivel criar '%s': %s\n", ARQ_SAIDA, strerror(errno));
        status = 1;
        goto fim;
    }

    for (int i = 0; i < NUM_PAGINAS; i++) {
        tabela_paginas[i] = -1;
    }
    
    for (int i = 0; i < TAM_TLB; i++) {
        tlb[i].valida = 0;
    }

    while (fgets(buffer, sizeof(buffer), arquivo) != NULL) {
        numero_linha++;
        relogio++;   
        if (strchr(buffer, '\n') == NULL && !feof(arquivo)) {
            fprintf(stderr, "Erro: linha %d de '%s' muito longa ou mal formatada.\n", numero_linha, arquivo_enderecos);
            status = 1;
            goto fim;
        }

        long endereco;
        if (converter_endereco(buffer, &endereco) != 0) {
            fprintf(stderr, "Erro: formato invalido na linha %d de '%s' (esperado um inteiro de 0 a %d por linha).\n",
                    numero_linha, arquivo_enderecos, ENDERECO_MAX);
            status = 1;
            goto fim;
        }

        long pagina = endereco >> 8;
        long offset = endereco & 0xFF;

        int frame;
        int posicao_tlb = buscar_tlb((int)pagina);

        if (posicao_tlb != -1) {

            frame = tlb[posicao_tlb].frame;
            tlb[posicao_tlb].ultimo_uso = relogio;
            tlb_hits++;
        } else {

            if (tabela_paginas[pagina] == -1) {

                int frame_destino;

                if (proximo_frame_livre < NUM_FRAMES) {
                    frame_destino = proximo_frame_livre;
                    proximo_frame_livre++;
                } else {
                    frame_destino = escolher_vitima(algoritmo_pagina);

                    int pagina_vitima = frame_para_pagina[frame_destino];
                    tabela_paginas[pagina_vitima] = -1;
                    invalidar_tlb(pagina_vitima);
                }

                if (fseek(backing, pagina * TAM_PAGINA, SEEK_SET) != 0) {
                    fprintf(stderr, "Erro: falha ao posicionar em '%s' (pagina %ld).\n", ARQ_BACKING, pagina);
                    status = 1;
                    goto fim;
                }

                if (fread(memoria_fisica[frame_destino], 1, TAM_PAGINA, backing) != TAM_PAGINA) {
                    fprintf(stderr, "Erro: falha ao ler a pagina %ld de '%s' (arquivo menor que o esperado?).\n",
                            pagina, ARQ_BACKING);
                    status = 1;
                    goto fim;
                }

                tabela_paginas[pagina] = frame_destino;
                frame_para_pagina[frame_destino] = (int)pagina;
                frame_carregado_em[frame_destino] = relogio;
                page_faults++;
            }

            frame = tabela_paginas[pagina];

            posicao_tlb = inserir_tlb((int)pagina, frame, algoritmo_tlb);
        }

      
        frame_ultimo_uso[frame] = relogio;

        long endereco_fisico = (long)frame * TAM_PAGINA + offset;
        signed char valor = (signed char)memoria_fisica[frame][offset];

        fprintf(saida, "Virtual address: %ld TLB: %d Physical address: %ld Value: %d\n",
                endereco, posicao_tlb, endereco_fisico, valor);

        total_enderecos++;
    }

    if (ferror(arquivo)) {
        fprintf(stderr, "Erro: falha ao ler '%s'.\n", arquivo_enderecos);
        status = 1;
        goto fim;
    }

    //temporario
    printf("Enderecos traduzidos: %d | Page faults: %d | TLB hits: %d\n",total_enderecos, page_faults, tlb_hits);

fim:
    if (arquivo != NULL) fclose(arquivo);
    if (backing != NULL) fclose(backing);
    if (saida   != NULL) fclose(saida);
    return status;
}