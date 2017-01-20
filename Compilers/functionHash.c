/*
 * Authors: Fabio Lorenzi & Cristian Sampietri
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "functionHash.h"


Nodo *ht[TOT];

int hash(char *id){
    int h = 0;
    int i= 0;
    for(i = 0; id[i] != '\0'; i++){
        h = ((h << SHIFT)+id[i])%TOT;
    }
    return h;
}

Nodo *creaNodo(char *id){
    Nodo *nuovoNodo = malloc(sizeof(Nodo));
    nuovoNodo->stringa = malloc(strlen(id)+1);
    nuovoNodo->fratello = NULL;
    strcpy(nuovoNodo->stringa, id);
    return nuovoNodo;
}


char* insertFind(int h, char *id){
	Nodo* node;
    if((node=ht[h])==NULL){
    	ht[h] = node = creaNodo(id);
    	return node->stringa;
    }
    while(strcmp(node->stringa, id)!=0){
    	Nodo* next = node->fratello;
    	if(next==NULL){
    		node->fratello = creaNodo(id);
    		return node->fratello->stringa;
    	}
    	node=next;
    }
    return node->stringa;
}

void print(){
	int i=0;
	for(;i<TOT;i++){
		Nodo* node;
		if((node=ht[i])!=NULL){
			printf("%d\n\t%s ", i, node->stringa);
			while((node=node->fratello)!=NULL){
				printf("%s ", node->stringa);
			}
			printf("\n");
		}
	}
}
