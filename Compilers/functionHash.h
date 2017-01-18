/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   functionsHash.h
 * Authors : Fabio Lorenzi & Cristian Sampietri
 *
 */

#ifndef FUNCTIONSHASH_H
#define FUNCTIONSHASH_H

#define TOT 20
#define SHIFT 4


typedef struct _nodo{
	char* stringa;
	struct _nodo* fratello;
}Nodo;

int hash(char *id);

Nodo* creaNodo(char *id);

char* insertFind(int h, char *id);

void print();


#endif /* FUNCTIONSHASH_H */
