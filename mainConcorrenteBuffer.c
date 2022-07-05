#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> 
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "Parser.h"
#include "timer.h"


typedef struct{
    int threadId;
    char *palavraProcurada; // aqui eu so passo a palavra que eu quero achar
} InformacaoTarefa;


int NUMERO_LEITORAS = 4;

long long int BUFFER = 100 * (1024 * 1024);

long long int quantidadeOcorrenciaEsperada = 1019260;

char **vetorDePalavras;
char *palavra;
long long int TAMANHO_VETOR_PALAVRAS; // variavel que vai ter o tamanho do vetor de palavras

long long int *posicoesFinais; // Esta variavel vai guardar as posioes em bytes de cada fim de bloco de texto
int NUMERO_ESCRITORAS = 1;

int escrevendo = 0, lendo = 0,leitoresBloqueados = 0, escritorAtivo = 1, processamentoConcluido = 0;

int FIM_DE_PROCESSAMENTO = 0; // Uma variavel que indica quando não tem mais nada para consumir.

pthread_cond_t condLeitor, condEscritor;
pthread_mutex_t lock;


long long int getTamanhoArquivo(FILE *arquivo){
    fseek(arquivo,SEEK_SET,SEEK_END); // manda o ponteiro para arquivo apontar pro fim do arquivo
    long long int tamanho = ftell(arquivo); // indica quantos bytes foram "pulados". No caso, fala a "posicao" apontada pelo ponteiro
    rewind(arquivo); // reseta o ponteiro para arquivo de volta pro inicio.

    return tamanho;
}


long long int getQuantidadeDeOcorrencias(long long int pontoInicial, long long int PontoParada, char *palavraProcurada, int threadId){
   long long int tamanhoPalavra = 0, numeroDeOcorrencias = 0; 

    for(long long int i = pontoInicial; i < PontoParada; i++){
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
        }else{
        }
        
    }

    return numeroDeOcorrencias;
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

/*
    A thread que gera o buffer executa esse codigo quando ja rodou pela primeira vez.
    A thread se bloqueia e espera as threads processadoras terminarem 
*/
void barreiraEscritor(){
    // quando ja termina de rodar...
    pthread_mutex_lock(&lock); 
    escritorAtivo = 0; // indica para as threads "leitoras" que não está mais gerando buffer
    pthread_cond_broadcast(&condLeitor); // libera as demais threads

    while(escritorAtivo == 0){
        pthread_cond_wait(&condEscritor,&lock); // se bloqueia e espera as demais threads acabarem
    }

    pthread_mutex_unlock(&lock);

}



void *escritora(void *arg){

    char *nomeArquivo = (char *) arg;
  
    FILE *arquivo = fopen(nomeArquivo, "r");
    
    long long int tamanhoArquivo = 0,quantidadePosicoesFinais = 0;
    double calculoPosicoesFinais = 0;

    if(arquivo == NULL){
        puts("Arquivo inexistente!");
        exit(EXIT_FAILURE);
    }
   
    tamanhoArquivo = getTamanhoArquivo(arquivo); // retorna tamanho do arquivo em bytes
     // Calcula o tamanho de cada bloco do arquivo a ser lido em bytes
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
        pthread_exit(NULL);
    }

    encontrarPosicoesFinais(arquivo, quantidadePosicoesFinais); // Demarca cada bloco que vai ser lido por esta thread
    
 
    long long int quantidadeASerLida = 0;


    for(long long int i = 0; i < quantidadePosicoesFinais; i++){ // para cada posição final / para cada fragmento de texto
        
         if(i == 0){ // calculando o tamanho do bloco a ser lido...
            quantidadeASerLida = posicoesFinais[i];
            
        }else{
            quantidadeASerLida = posicoesFinais[i] - posicoesFinais[i - 1];
        }

       palavra = (char *) malloc(sizeof(char) * quantidadeASerLida);

        if(palavra == NULL){
            puts("Erro na alocação do buffer!");
            exit(EXIT_FAILURE);
        }
        
        fread(palavra,1,quantidadeASerLida,arquivo); // Lê/bufferiza a quantidade a ser lida dentro da variavel palavra
        
        // da um  split()/Vetor de "string". Esse vetor vai ser processado depois
        vetorDePalavras = split(palavra,&TAMANHO_VETOR_PALAVRAS); 
        
     
        barreiraEscritor();

        free(palavra);
        free(vetorDePalavras);


    } // fim do for


    fclose(arquivo);

   pthread_mutex_lock(&lock);
    FIM_DE_PROCESSAMENTO = 1; // Indica pra thread leitora que não precisa mais trabalhar
    escritorAtivo = 0;
    pthread_cond_broadcast(&condLeitor);

    pthread_mutex_unlock(&lock);
    

    pthread_exit(NULL);
}

void barreira_leitor(){

    pthread_mutex_lock(&lock);


    if(leitoresBloqueados == (NUMERO_LEITORAS - 1)){ // Se todo mundo processou...
        leitoresBloqueados=0; 
        // reativa a thread geradora de buffer
        escritorAtivo = 1; 
        pthread_cond_signal(&condEscritor);
        
    }else{ // senão
        leitoresBloqueados++;
        pthread_cond_wait(&condLeitor,&lock); 
    }
    pthread_mutex_unlock(&lock);

}


void *leitora(void *arg){
    /*
        Aqui pega o id, faz a divisão por bloco e avisa quando acabou de processar - OK
        Ter uma forma de sair quando não tem mais nada pra processar
    */

   InformacaoTarefa *argumento = (InformacaoTarefa *) arg;
   long long int tamanhoBloco = 0, pontoInicial =0, pontoParada = 0, retorno = 0;

   while(1){

        pthread_mutex_lock(&lock); 
        // verifica se a thread geradora de buffer está executando...
        while(escritorAtivo != 0) {
            pthread_cond_wait(&condLeitor,&lock);
        }

        // verifica se não tem mais nada para processar
        if(FIM_DE_PROCESSAMENTO != 0){
            pthread_mutex_unlock(&lock);
            break;
        }
        pthread_mutex_unlock(&lock);

        
        tamanhoBloco = (TAMANHO_VETOR_PALAVRAS - 1) / NUMERO_LEITORAS; // divide o tamanho do bloco pelo numero de threads leitoras
        pontoInicial = tamanhoBloco * argumento->threadId;

        if(argumento->threadId != NUMERO_LEITORAS - 1){ // se a thread leitora nao for a ultima
            pontoParada = pontoInicial + tamanhoBloco;
        }else{
            pontoParada = TAMANHO_VETOR_PALAVRAS;
        }

        retorno += getQuantidadeDeOcorrencias(pontoInicial,pontoParada, argumento->palavraProcurada,argumento->threadId);

        barreira_leitor();

   }


    pthread_exit((void *) retorno);

}

// converte todos os caracteres de uma "string" em minusculo 
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

    pthread_t identificadorProdutora;
    pthread_t *identificadoresleitora;

    InformacaoTarefa *argumentos;

    long long int retorno = 0, somaTotalOcorrencias = 0;
    double inicio, fim, tempo;

    char *palavraProcurada, *nomeArquivo;

    if(argc < 3){
        puts("Digite: ./mainConcorrente <nome do arquivo> <palavra procurada>");
        puts("Ou");
        puts("Digite: ./mainConcorrentePonteiro <NomeArquivo> <Palavra Procurada> <Numthreds>");
        return EXIT_FAILURE;
    }

    nomeArquivo = argv[1];
    palavraProcurada = argv[2];

    if(argc > 4){ // Se um número de threads for fornecido...
        NUMERO_LEITORAS = atoi(argv[3]);
    }

    // transforma todos os caracteres da palavra procurada em minusculo...
    palavraProcurada = strToLower(palavraProcurada);

    pthread_mutex_init(&lock,NULL);
    pthread_cond_init(&condLeitor,NULL);
    pthread_cond_init(&condEscritor,NULL);


    identificadoresleitora = (pthread_t *)  malloc(sizeof(pthread_t) * NUMERO_LEITORAS);
    if(identificadoresleitora == NULL){
        puts("Erro na alocação do identificados da thread leitora");
        exit(EXIT_FAILURE);
    }

    argumentos = (InformacaoTarefa *) malloc(NUMERO_LEITORAS * sizeof(InformacaoTarefa));
    if(argumentos == NULL){
        puts("Erro ao alocar espaço para os argumentos das threads.");
        return EXIT_FAILURE;
    }



    GET_TIME(inicio);
    if(pthread_create(&identificadorProdutora,NULL,escritora,(void *) nomeArquivo)){
        puts("Erro no pthread_create da thread produtora");
        exit(EXIT_FAILURE);
    }




    for(int i = 0; i < NUMERO_LEITORAS; i++){

        argumentos[i].palavraProcurada = palavraProcurada;
        argumentos[i].threadId = i;

        if(pthread_create(&identificadoresleitora[i],NULL,leitora,(void*) &argumentos[i])){
            puts("Erro no pthread_create da thread leitora");
            exit(EXIT_FAILURE);
        }
    }

    pthread_join(identificadorProdutora,NULL);

    for(int i = 0; i < NUMERO_LEITORAS; i++ ){
        pthread_join(identificadoresleitora[i],(void **) &retorno);
        somaTotalOcorrencias += retorno;
    }

    GET_TIME(fim);

    tempo = fim - inicio;
    printf("\nNome do arquivo utilizado: \"%s\"\n",nomeArquivo);
    printf("Número de ocorrencias da palavra \"%s\": %lld\n",palavraProcurada, somaTotalOcorrencias);
    printf("Tempo gasto na busca: %lf Segundos\n\n", tempo);


   
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&condLeitor);
    pthread_cond_destroy(&condEscritor);


    pthread_exit(NULL);
    return EXIT_SUCCESS;
}