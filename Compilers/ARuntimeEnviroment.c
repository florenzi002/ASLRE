/*
 * ARuntimeEnviroment.c
 *
 *  Created on: 09/dic/2016
 *      Author: fabio & cristian
 */

#include "redef.h";

main(int argc, char *argv[]) {
	Acode *instruction;
	start_abstract_machine();
	while ((instruction = &program[pc++])­->operator != HALT)
		execute(instruction);
	stop_abstract_machine();
	return;
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


void stop_abstract_machine()
{
	freemem((char*) program, sizeof(Acode) * code_size);
	freemem((char*) astack, sizeof(Adescr*) * asize);
	freemem((char*) ostack, sizeof(Odescr*) * osize);
	freemem(istack, isize);
	printf("Program executed without errors\n");
	printf("Allocation: %ld bytes\n", allocated);
	printf("Deallocation: %ld bytes\n", deallocated);
	printf("Residue: %ld bytes\n", allocated - deallocated);
}

void *newmem(int size)
{
	void *p;
	if((p = malloc(size)) == NULL)
		abstract_machine_error("Failure in memory allocation");
	allocated += size;
	return p;
}

void freemem(char *p, int size)
{
	free(p);
	deallocated += size;
}

Adescr *push_activation_record()
{
	Arecord **full_astack;
	int i;
	if(ap == asize)
	{
		full_astack = astack;
		astack = (Arecord**) newmem(sizeof(Arecord*) * (asize + ASEGMENT));
		for(i = 0; i < asize; i++)
			astack[i] = full_astack[i];
		freemem((char*) full_astack, sizeof(Arecord*) * asize);
		asize += ASEGMENT;
	}
	return (astack[ap++] = (Arecord*) newmem(sizeof(Arecord)));
}

void pop_activation_record()
{
	if(ap == 0)
		abstract_machine_error("Failure in popping activation record");
	freemem((char*) astack[ap],
			sizeof(Arecord));
}

void execute(Acode *instruction)
{
	switch(instruction->operator)
	{
	case PUSH: execute_push(instruction->operands[0].ival, instruction->operands[1].ival); break;
	case JUMP: execute_jump(instruction->operands[0].ival); break;
	case APOP: execute_apop(); break;
	case ADEF: execute_adef(instruction->operands[0].ival); break;
	case SDEF: execute_sdef(instruction->operands[0].ival); break;
	case LOCI: execute_loci(instruction->operands[0].ival); break;
	case LOCS: execute_locs(instruction->operands[0].sval); break;
	// TODO
	case RETN: execute_retn(); break;
	default: abstract_machine_error("Unknown operator"); break;
	}
}

void execute_jump(int address)
{
	pc = address;
}
void execute_skip(int offset)
{
	pc += offset-1;
}
void execute_skpf(int offset)
{
	if(!pop_bool())
		pc += offset-1;
}
void execute_retn()
{
	pc = top_astack()->retad;
}

void execute_addi()
{
	int n, m;
	n = pop_int();
	m = pop_int();
	push_int(m+n);
}

void execute_igrt()
{
	int n, m;
	n = pop_int();
	m = pop_int();
	push_bool(m>n);
}

void execute_adef(int size)
{
	Orecord *po;
	po = push_ostack();
	po>
	type = ATOM;
	po>
	size = size;
}


