#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include "timer.h"


#define NUMTHREADS_LEITORAS 4
#define PROCURADA "teste"
#define NOMEARQUIVO "texto.txt"


typedef struct {
    char *nomeArquivo;
    long int idThread;
}InformacaoTarefa;

long long int *posicoesFinais; // Esta variavel vai guardar as posioes em bytes de cada fim de bloco de texto


long long int getTamanhoArquivo(FILE *arquivo){
    fseek(arquivo,SEEK_SET,SEEK_END); // manda o ponteiro para arquivo apontar pro fim do arquivo
    long long int tamanho = ftell(arquivo); // indica quantos bytes foram "pulados". No caso, fala a "posicao" apontada pelo ponteiro
    rewind(arquivo); // reseta o ponteiro para arquivo de volta pro inicio.

    return tamanho;
}




/*
    O que faz: Calcula Vai pro proximo bloco,

*/

long long int procurarPalavra(FILE *arquivo,long long int pontoInicial, long long int pontoFinal, char *nomeArquivo){

    char palavra[30];
    int numeroDeCaracteres = 0, controle = 0, inicio = 0;
    long long int numeroDeOcorrencias = 0;

    fseek(arquivo,pontoInicial,SEEK_SET); // pula o ponto de arquivo para o ponto inicial de processamento

    while(ftell(arquivo) < pontoFinal -1){ // enquanto eu não passar do limite de leitura.

        numeroDeCaracteres = fscanf(arquivo,"%s", palavra);
        if(numeroDeCaracteres != 1) break; /* Encerra a verificação no final do arquivo. */

        controle = 0;
        for(int j = 0; j < 30; j++){
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
        if(!strcmp(&palavra[inicio], PROCURADA)){
            numeroDeOcorrencias++;
        }
      
    }
   
    return numeroDeOcorrencias;

}

void *tarefa(void *arg){
    //long int id = (long int)  arg;
    InformacaoTarefa *informacaoTarefa = (InformacaoTarefa * ) arg;

    long long int pontoInicial, pontoFinal, retorno= 0;
    FILE *arquivo;
    // thread se situando pra saber por onde ela deve atuar
    if(informacaoTarefa->idThread == 0){
        pontoInicial = 0;
    }else{

        pontoInicial = posicoesFinais[informacaoTarefa->idThread - 1];
    }
    pontoFinal = posicoesFinais[informacaoTarefa->idThread];

    arquivo = fopen(informacaoTarefa->nomeArquivo, "r");
    if(arquivo == NULL){
        puts("Arquivo inexistente!");
        pthread_exit(NULL);
    }
      

    
    retorno = procurarPalavra(arquivo, pontoInicial, pontoFinal, informacaoTarefa->nomeArquivo);

    fclose(arquivo);
    pthread_exit((void *) retorno);

}


void encontrarPosicoesFinais(FILE *arquivo){
    long long int tamanhoBloco, fim;
    long long int tamanhoArquivo;

    tamanhoArquivo = getTamanhoArquivo(arquivo);
    tamanhoBloco = tamanhoArquivo / NUMTHREADS_LEITORAS;

    for(int i = 0; i < NUMTHREADS_LEITORAS - 1; i++){ // aqui eu vou fazer os calculos e ver as paradas ae

        long long int fimSugerido;
        
        if(i == 0){ // se for o primeiro fim...
            fimSugerido = (tamanhoBloco * i) + tamanhoBloco;
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
        while(isalnum(getc(arquivo)) ){ // se nao for espaço em branco
            offset++; // vai contando a quantidade de bytes pulados pra frente pra encontrar um espaço em branco
        }

        posicoesFinais[i] = fim + offset;
    }

    posicoesFinais[NUMTHREADS_LEITORAS - 1] = tamanhoArquivo; // coloca o ultimo fim na ultima posicao do vetor
    rewind(arquivo);
}



int main(int argc, char *argv[]){

    char *nomeArquivo = NOMEARQUIVO;
    FILE *arquivo = fopen(nomeArquivo, "r");

    pthread_t *identificadores;
    InformacaoTarefa *argumentos;

    long long int retorno, somaQuantidadeOcorrencias = 0;
    double inicio, fim, tempo;

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

    // alocando espaço para vetor de posições
    posicoesFinais = (long long int *) malloc(NUMTHREADS_LEITORAS * sizeof(long long int));
    if(posicoesFinais == NULL){
        puts("Erro ao alocar espaço para as posições finais de leitura.");
        return EXIT_FAILURE;
    }

    encontrarPosicoesFinais(arquivo); // marca as posições finais(limites) de cada thread 
    fclose(arquivo); // fechando o arquivo, ja que cada thread vai abrir uma instancia propria do arquivo

    GET_TIME(inicio);

    for(long int i = 0; i < NUMTHREADS_LEITORAS; i++){

        argumentos[i].nomeArquivo = nomeArquivo;
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

    printf("\n\nA palavra '%s' aparece %lld vezes em '%s'\n", PROCURADA, somaQuantidadeOcorrencias, nomeArquivo);


    GET_TIME(fim);

    tempo = fim - inicio;

    printf("\nTempo de execução: %lf Segundos\n", tempo);
   

    free(posicoesFinais);
    pthread_exit(NULL);

    return EXIT_SUCCESS;
}
