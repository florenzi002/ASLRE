/*
 * ARuntimeEnviroment.c
 *
 *  Created on: 09/dic/2016
 *      Author: fabio & cristian
 */

#include "redef.h"

extern int pc;
extern Acode *program;

int main(int argc, char *argv[]) {
	Acode *instruction;
	start_abstract_machine();
	while((instruction = &program[pc++])->operator != HALT)
		execute(instruction);
	stop_abstract_machine();
	return 0;
}




