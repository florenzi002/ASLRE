/*
 * ARuntimeEnviroment.c
 *
 *  Created on: 09/dic/2016
 *      Author: fabio & cristian
 */

extern Acode *program;
extern int pc;
Arecord **astack;
Orecord **ostack;
char *istack;
int asize, osize, isize;
int ap, op, ip;
long allocated = 0,
		deallocated = 0;



main(int  argc,  char *argv[]) {
	Acode *instruction;
	start_abstract_machine();
	while ((instruction = &program[pc++])­>opertor != HALT)
		execute(instruction);
	stop_abstract_machine();
}

void start_abstract_machine()
{
	load_acode();
	pc = ap = op = ip = 0;
	astack = (Arecord**) newmem(sizeof(Arecord*) * ASEGMENT);
	asize = ASEGMENT;
	ostack = (Orecord**) newmem(sizeof(Orecord*) * OSEGMENT);
	osize = OSEGMENT;
	istack = (char*) newmem(ISEGMENT);
	isize = ISEGMENT;
}
