/*
 * ARuntimeEnviroment.c
 *
 *  Created on: 09/dic/2016
 *      Authors: Fabio Lorenzi & Cristian Sampietri
 */

#include "redef.h"

extern int pc; // Il Program Counter
extern Acode *program; // Ciascun Acode rappresenta una riga di codice generato

/**
 * Nel metodo main viene avviata la macchina virtuale e si eseguono le istruzioni (singole righe di codice generato)
 * finche' non si incontra l'operatore HALT
 */
int main(int argc, char *argv[]) {
	Acode *instruction;
	start_abstract_machine();
	while((instruction = &program[pc++])->operator != HALT)
		execute(instruction);
	stop_abstract_machine(); // Esecuzione terminata, la macchina virtuale viene stoppata
	return 0;
}



