/*
 * ARuntimeEnviroment.c
 *
 *  Created on: 09/dic/2016
 *      Authors: Fabio Lorenzi & Cristian Sampietri
 */

#include "redef.h"

extern int pc; // Il Program Counter
extern Acode *program; // Ciascun Acode rappresenta una riga di codice generato
extern char *program_file;
/**
 * Nel metodo main viene avviata la macchina virtuale e si eseguono le istruzioni (singole righe di codice generato)
 * finche' non si incontra l'operatore HALT
 */
int main(int argc, char *argv[]) {
	if(argc!=2)
		abstract_machine_Error("No file to execute");
	program_file = argv[1];
	Acode *instruction;
	start_abstract_machine();
	while((instruction = &program[pc++])->operator != HALT)
		execute(instruction);
	stop_abstract_machine(); // Esecuzione terminata, la macchina virtuale viene stoppata
	return 0;
}



