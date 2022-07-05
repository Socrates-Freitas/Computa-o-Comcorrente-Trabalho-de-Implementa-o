#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "Parser.h"

/*
* Esta função divide o texto do comando em um array de strings
* Esse array de strings vai ser o argv para o execve 
* Lembrar: Dar free() quando usar essa função 
*/
char  **split(char *comando,long long int *recebedorQuantidade){
    /* TODO: Colocar um parametro do tipo ponteiro para inteiro para ter a informação a respeito do número de elementos
        Isso vai ser importante para acessar o último item(avaliar se é ou não é "&") */
     
    char **argv;
    char *tokenDosElementos;
    int quantidadeElementos = 0;
    // Isso é necessário, porque quando eu usar strtok, o resultado vai ser char*, mas eu preciso de **char
    // eu também preciso de um vetor de strings que possua o ultimo argumento como null

    argv = malloc(sizeof(char *));
    if(argv){ // se a alocação deu certo
        tokenDosElementos = strtok(comando," \n\t"); // "separa" elementos com delimitação baseada em espaço

        while(tokenDosElementos != NULL){
            argv = realloc(argv,sizeof(char *) * (quantidadeElementos+1));
            if(argv){ // caso a alocação tiver sucesso
                argv[quantidadeElementos] = tokenDosElementos;
                tokenDosElementos = strtok(NULL," .,;+=-_?:[]{}!@#$'\"*()\\/|`\n\t<>"); // passa pro próximo pedaço da string separado por espaço
                quantidadeElementos++;
            }
            else{ // senão
                printf("Erro na alocação :(\n");
                exit(1);
            }
        }
        // Colocar NULL como último elemento do vetor de string é importante para o uso
        // da função execve,que exige que o último elemento seja NULL
        argv = realloc(argv,sizeof(char *) * (quantidadeElementos+1));
        if(argv == NULL){ // se a alocação deu problema 
            printf("erro na alocação :(\n");
            exit(1);
        }
        else{ // se a alocação deu certo
            argv[quantidadeElementos] = NULL;
            // Aqui eu atualizo o valor do ponteiro pra inteiro que vai ter a quantidade de elementos em argv
            // isso vai ser necessário para acessar o último item
            
            if(recebedorQuantidade != NULL){
                quantidadeElementos++;
                *recebedorQuantidade = quantidadeElementos;
            }
        }


    }else{
        printf("erro na alcalçao :(\n");
    }

    return argv;
}


