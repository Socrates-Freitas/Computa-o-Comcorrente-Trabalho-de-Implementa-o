#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> 
#include <string.h>
#include <math.h>
#include <pthread.h>
#include "Parser.h"
#include "timer.h"


long long int BUFFER = 100 * (1024 * 1024);

long long int quantidadeOcorrenciaEsperada = 1019260;



long long int TAMANHO_VETOR_PALAVRAS; // variavel que vai ter o tamanho do vetor de palavras

long long int *posicoesFinais; // Esta variavel vai guardar as posioes em bytes de cada fim de bloco de texto


void imprimirVetor(char **vetorDePalavras, long int quantidadeVetor){
  printf("[");
    for(int i =0; i < quantidadeVetor; i++){
        if(vetorDePalavras[i] != NULL){
            if(i < quantidadeVetor - 2){
                printf("\"%s\" -> tamanho: %ld, ", vetorDePalavras[i], strlen(vetorDePalavras[i]));
            }
            else{
                printf("\"%s\" -> tamanho: %ld]\n", vetorDePalavras[i],strlen(vetorDePalavras[i]) );
            }
        }
    }
}

void imiprimirPosicoesFinais(long long int *vetor,long long int quantidadePosicoes){
    for(long long int i =0; i < quantidadePosicoes; i++){
        printf("Posicao final: %lld: %lld\n", i+1, vetor[i]);
    }
}

long long int getTamanhoArquivo(FILE *arquivo){
    fseek(arquivo,SEEK_SET,SEEK_END); // manda o ponteiro para arquivo apontar pro fim do arquivo
    long long int tamanho = ftell(arquivo); // indica quantos bytes foram "pulados". No caso, fala a "posicao" apontada pelo ponteiro
    rewind(arquivo); // reseta o ponteiro para arquivo de volta pro inicio.

    return tamanho;
}


void encontrarPosicoesFinais(FILE *arquivo, long long int quantidadePosicoes){
    long long int tamanhoBloco, fim;
    long long int tamanhoArquivo;

    tamanhoArquivo = getTamanhoArquivo(arquivo);
    tamanhoBloco = tamanhoArquivo / quantidadePosicoes;

    for(long long int i = 0; i < quantidadePosicoes - 1; i++){ // aqui eu vou fazer os calculos e ver as paradas ae

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

    posicoesFinais[quantidadePosicoes - 1] = tamanhoArquivo; // coloca o ultimo fim na ultima posicao do vetor
    rewind(arquivo);
}


long long int getQuantidadeDeOcorrencias(char **vetorDePalavras, long long int tamanhoVetorPalavras, char *palavraProcurada){

   long long int tamanhoPalavra = 0, numeroDeOcorrencias = 0; 

    for(int i =0; i < tamanhoVetorPalavras; i++){
        /* Lê cada palavra do arquivo e armazena na variável. */
        if(vetorDePalavras[i] == NULL) /* Encerra a verificação no final do arquivo. */
            break;

        /* Formata todos os carcteres da palavra em minúsculo e define início e fim. */
        tamanhoPalavra = strlen(vetorDePalavras[i]);
        for(int j = 0; j < tamanhoPalavra; j++){ // para cada palavra..
            vetorDePalavras[i][j] = tolower(vetorDePalavras[i][j]);

        }

        /* Verifica se a palavra em questão é a procurada. */
        if(strcmp(palavraProcurada, vetorDePalavras[i]) == 0){
            numeroDeOcorrencias++;
        }
    }

    return numeroDeOcorrencias;
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

    char *nomeArquivo, *palavraProcurada;
    FILE *arquivo;
    char *palavra;
    char **vetorDePalavras;
    long long int tamanhoArquivo = 0,quantidadePosicoesFinais = 0, numeroDeOcorrencias = 0;
    double calculoPosicoesFinais = 0;
    double inicio, fim, tempo;


    if(argc < 3){
        puts("Digite: ./mainSequencial <nome do arquivo> <palavra procurada>");
        return EXIT_FAILURE;
    }
    nomeArquivo = argv[1];
    palavraProcurada = argv[2];
    palavraProcurada = strToLower(palavraProcurada);
    arquivo =  fopen(nomeArquivo, "r");
  
    if(arquivo == NULL){
        puts("Arquivo inexistente!");
        return EXIT_FAILURE;
    }
   
    tamanhoArquivo = getTamanhoArquivo(arquivo);
    calculoPosicoesFinais = tamanhoArquivo / BUFFER;
    if(calculoPosicoesFinais <= 1){
        quantidadePosicoesFinais = 1;
    }
    else{
        quantidadePosicoesFinais = (long long int) round(calculoPosicoesFinais);
    }

    // alocando espaço para vetor de posições
    posicoesFinais = (long long int *) malloc(quantidadePosicoesFinais * sizeof(long long int));
    if(posicoesFinais == NULL){
        puts("Erro ao alocar espaço para as posições finais de leitura.");
        return EXIT_FAILURE;
    }

    GET_TIME(inicio);

    encontrarPosicoesFinais(arquivo, quantidadePosicoesFinais); // marca as posições finais(limites) de cada thread
     
    long long int quantidadeASerLida = 0;
    
    
    for(long long int i =0; i < quantidadePosicoesFinais; i++){ // para cada posição final
        if(i == 0){
            quantidadeASerLida = posicoesFinais[i];
        }else{
            quantidadeASerLida = posicoesFinais[i] - posicoesFinais[i - 1];
        }

       
        palavra = (char *) malloc(sizeof(char) * quantidadeASerLida);

        if(palavra == NULL){
            puts("Erro na alocação do buffer!");
            exit(EXIT_FAILURE);
        }
        
        fread(palavra,1,quantidadeASerLida,arquivo);
        
        vetorDePalavras = split(palavra,&TAMANHO_VETOR_PALAVRAS);
        
        numeroDeOcorrencias += getQuantidadeDeOcorrencias(vetorDePalavras,TAMANHO_VETOR_PALAVRAS, palavraProcurada);
        
        free(palavra);
        free(vetorDePalavras);
    }
    
   
   
    GET_TIME(fim);
    
    tempo = fim - inicio;


    

    printf("\nNome do arquivo utilizado: \"%s\"\n",nomeArquivo);
    printf("Número de ocorrencias da palavra \"%s\": %lld\n",palavraProcurada, numeroDeOcorrencias);
    printf("Tempo gasto na busca: %lf Segundos\n\n", tempo);

    return EXIT_SUCCESS;
}