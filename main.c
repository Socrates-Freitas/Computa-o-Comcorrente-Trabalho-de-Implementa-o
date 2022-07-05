#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include "timer.h"




typedef struct {
    char *nomeArquivo;
    char *palavraProcurada;
    long int idThread;
}InformacaoTarefa;

int NUMTHREADS_LEITORAS = 4;
long long int *posicoesFinais; // Esta variavel vai guardar as posioes em bytes de cada fim de bloco de texto
long long int *posicoesIniciais;

long long int getTamanhoArquivo(FILE *arquivo){
    fseek(arquivo,SEEK_SET,SEEK_END); // manda o ponteiro para arquivo apontar pro fim do arquivo
    long long int tamanho = ftell(arquivo); // indica quantos bytes foram "pulados". No caso, fala a "posicao" apontada pelo ponteiro
    rewind(arquivo); // reseta o ponteiro para arquivo de volta pro inicio.

    return tamanho;
}



long long int procurarPalavra(FILE *arquivo,long long int pontoInicial, long long int pontoFinal, char *nomeArquivo,char *palavraProcurada){

    int tamanhoPalavra = 25;
    char palavra[tamanhoPalavra];
    int numeroDeCaracteres = 0, controle = 0, inicio = 0;
    long long int numeroDeOcorrencias = 0;


    while(ftell(arquivo) < pontoFinal){ // enquanto eu não passar do limite de leitura.

        numeroDeCaracteres = fscanf(arquivo,"%s", palavra);
        if(numeroDeCaracteres != 1) break; /* Encerra a verificação no final do arquivo. */

        controle = 0;
        for(int j = 0; j < tamanhoPalavra; j++){
            if(isalpha(palavra[j]) && controle == 0){
                inicio = j;
                controle = 1;
            }
            if(!isalpha(palavra[j]) && controle == 1){
                palavra[j] = '\0';
                break;
            }
            palavra[j] = tolower(palavra[j]);
        }

        /* Verifica se a palavra em questão é a procurada. */
        if(!strcmp(palavraProcurada, &palavra[inicio])){
            numeroDeOcorrencias++;
        }
        else{
        }

    }
   
    return numeroDeOcorrencias;

}

/* Aqui, cada as 4 threads processaram os arquivos*/ 
void *tarefa(void *arg){
    InformacaoTarefa *informacaoTarefa = (InformacaoTarefa * ) arg;

    long long int pontoInicial, pontoFinal, retorno= 0;
    FILE *arquivo;
   
    pontoInicial = posicoesIniciais[informacaoTarefa->idThread];
    pontoFinal = posicoesFinais[informacaoTarefa->idThread];

    arquivo = fopen(informacaoTarefa->nomeArquivo, "r");
    if(arquivo == NULL){
        puts("Arquivo inexistente!");
        pthread_exit(NULL);
    }
  
   fseek(arquivo,pontoInicial,SEEK_SET);
    
    retorno = procurarPalavra(arquivo, pontoInicial, pontoFinal, informacaoTarefa->nomeArquivo,informacaoTarefa->palavraProcurada);

    fclose(arquivo);
    pthread_exit((void *) retorno);

}

/*
    Esta função garante que cada thread começa no próximo espaço em branco a partir do fim da anterior.
    Assim, sobreposições são evitadas
*/
void encontrarPosicoesIniciais(FILE *arquivo, long long int tamanhoArquivo){
  
    posicoesIniciais[0] = 0; // a primeira posição...
    for (int i = 1; i < NUMTHREADS_LEITORAS; i++){ // para cada thread

        fseek(arquivo, posicoesFinais[i - 1], SEEK_SET); // Pula pra x posicao final anterior
        long long int offset = 0;
        while(isalnum(getc(arquivo)) ){ // se o ponto inicial não for espaço em branco
            offset++; // vai contando a quantidade de bytes pulados pra frente pra encontrar um espaço em branco
        }

        posicoesIniciais[i] = posicoesFinais[i-1] + offset;

    }

    rewind(arquivo); // reseta o ponteiro de arquivo

}


void encontrarPosicoesFinais(FILE *arquivo, long long int tamanhoArquivo){
    long long int tamanhoBloco, fim;
    tamanhoBloco = tamanhoArquivo / NUMTHREADS_LEITORAS;

    for(int i = 0; i < NUMTHREADS_LEITORAS - 1; i++){ // aqui eu vou fazer os calculos e ver as paradas ae

        long long int fimSugerido;
        
        if(i == 0){ // se for o primeiro fim...
            fimSugerido = tamanhoBloco;
        }else{ // senao
            fimSugerido = posicoesFinais[i - 1] + tamanhoBloco;
        }

        if(fimSugerido > tamanhoArquivo){ // se extrapolar o tamanho total do arquivo
            fimSugerido = tamanhoArquivo; // corrige e coloca de volta pro fim do arquivo
            break;
        }

        fim = fimSugerido;
        
        fseek(arquivo, fim, SEEK_SET); // coloco o ponteiro pra arquivo "apontar pra aquele fim"
        
        long long int offset = 0;
        while(isalnum(getc(arquivo)) ){ // se o ponto de parada não for espaço em branco
            offset++; // vai contando a quantidade de bytes pulados pra frente pra encontrar um espaço em branco
        }

        posicoesFinais[i] = fim + offset + 1;
    }

    posicoesFinais[NUMTHREADS_LEITORAS - 1] = tamanhoArquivo; // coloca o ultimo fim na ultima posicao do vetor
    rewind(arquivo);
}


char *strToLower(char *str)
{
  unsigned char *charAtual = (unsigned char *)str;

  while (*charAtual) { // enquanto nao chegou no fim do arquivo
     *charAtual = tolower((unsigned char)*charAtual);
      charAtual++; // avança
  }

  return str;
}

int main(int argc, char *argv[]){

    pthread_t *identificadores;
    InformacaoTarefa *argumentos;

    long long int retorno, tamanhoArquivo = 0, somaQuantidadeOcorrencias = 0;
    double inicio, fim, tempo;
    char *nomeArquivo, *palavraProcurada;
    if(argc < 3){
        puts("Digite: ./mainConcorrentePonteiro <NomeArquivo> <Palavra Procurada>");
        puts("Ou");
        puts("Digite: ./mainConcorrentePonteiro <NomeArquivo> <Palavra Procurada> <Numthreds>");
        return EXIT_FAILURE;
    }

    nomeArquivo = argv[1];
    palavraProcurada = argv[2];
    if(argc > 4){ // Se um número de threads for fornecido...
        NUMTHREADS_LEITORAS = atoi(argv[3]);
    }
    
    // transforma todos os caracteres da palavra procurada em minusculo...
    palavraProcurada = strToLower(palavraProcurada);

    FILE *arquivo = fopen(nomeArquivo, "r");


    if(arquivo == NULL){
        puts("Arquivo não existe!");
        return EXIT_FAILURE;
    }

    // alocando espaço para identificadores de threads
    identificadores = (pthread_t *) malloc(NUMTHREADS_LEITORAS * sizeof(pthread_t));
    if(identificadores == NULL){
        puts("Erro ao alocar espaço para os identificadores das threads.");
        return EXIT_FAILURE;
    }

    // alocando espaço dos arguentos para as threads
    argumentos = (InformacaoTarefa *) malloc(NUMTHREADS_LEITORAS * sizeof(InformacaoTarefa));
    if(argumentos == NULL){
        puts("Erro ao alocar espaço para os argumentos das threads.");
        return EXIT_FAILURE;
    }

    // alocando espaço para vetor de posições iniciais

    posicoesIniciais = (long long int *) malloc(NUMTHREADS_LEITORAS * sizeof(long long int));
    if(posicoesIniciais == NULL){
        puts("Erro ao alocar espaço para as posições iniciais de leitura.");
        return EXIT_FAILURE;
    }

    // alocando espaço para vetor de posições finais
    posicoesFinais = (long long int *) malloc(NUMTHREADS_LEITORAS * sizeof(long long int));
    if(posicoesFinais == NULL){
        puts("Erro ao alocar espaço para as posições finais de leitura.");
        return EXIT_FAILURE;
    }
    tamanhoArquivo = getTamanhoArquivo(arquivo);
    encontrarPosicoesFinais(arquivo, tamanhoArquivo); // marca as posições finais(limites) de cada thread 
    encontrarPosicoesIniciais(arquivo,tamanhoArquivo); // marca as posições iniciais de atuação de cada thread

    fclose(arquivo); // fechando o arquivo, ja que cada thread vai abrir uma instancia propria do arquivo

    GET_TIME(inicio);

    for(long int i = 0; i < NUMTHREADS_LEITORAS; i++){

        argumentos[i].nomeArquivo = nomeArquivo;
        argumentos[i].palavraProcurada = palavraProcurada;
        argumentos[i].idThread = i;


        if(pthread_create(&identificadores[i],NULL,tarefa,(void *) &argumentos[i] )){
            puts("Erro no pthread_create!");
            exit(EXIT_FAILURE);
        }
    } 


    for(int i = 0; i < NUMTHREADS_LEITORAS; i++){
        if(pthread_join(identificadores[i],(void **) &retorno)){
            puts("Erro no pthread_join!");
            exit(EXIT_FAILURE);
        }

        somaQuantidadeOcorrencias += retorno;
    }

    GET_TIME(fim);

    tempo = fim - inicio;

    printf("\nNome do arquivo utilizado: \"%s\"\n",nomeArquivo);
    printf("Número de ocorrencias da palavra \"%s\": %lld\n",palavraProcurada, somaQuantidadeOcorrencias);
    printf("Tempo gasto na busca: %lf Segundos\n\n", tempo);
  

    free(posicoesIniciais);
    free(posicoesFinais);

    pthread_exit(NULL);

    return EXIT_SUCCESS;
}
