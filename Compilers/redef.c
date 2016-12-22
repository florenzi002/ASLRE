/*
 * redef.c
 *
 *  Created on: 09/dic/2016
 *      Author: fabio & cristian
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "redef.h"

char* s_op_code[] = {
    "ACODE",
    "PUSH",
    "JUMP",
    "APOP",
    "HALT",
    "ADEF",
    "SDEF",
    "LOAD",
    "PACK",
    "LODA",
    "IXAD",
    "AIND",
    "SIND",
    "STOR",
    "ISTO",
    "SKIP",
    "SKPF",
    "EQUA",
    "NEQU",
    "IGRT",
    "IGEQ",
    "ILET",
    "ILEQ",
    "SGRT",
    "SGEQ",
    "SLET",
    "SLEQ",
    "ADDI",
    "SUBI",
    "MULI",
    "DIVI",
    "UMIN",
    "NEGA",
    "READ",
    "WRIT",
    "MODL",
    "RETN",
    "LOCS",
    "LOCI",
    "NOOP"
};


const int ASEGMENT = 10;
const int OSEGMENT = 10;
const int ISEGMENT = 10;

const int LINEDIM = 128;

void *newmem(int);
void freemem(char *, int);
Arecord *push_activation_record();
void pop_activation_record();
//void execute(Acode *);
void execute_jump(int);
void execute_skip(int);
void execute_skpf(int);
void execute_retn();
void execute_addi();
void execute_igrt();
void execute_adef(int);
Arecord *top_astack();

Acode *program;
int pc, code_size;
Arecord **astack;
Orecord **ostack;
char *istack;
int asize, osize, isize, code_size;

int ap, op, ip;

long allocated = 0, deallocated = 0;

/**
 * Funzione che apre il file contenente l'A-Code e lo legge riga per riga, mettendo ciascuna di queste in
 * un array
 */
void load_acode(){
	FILE *file = fopen("./my_program.txt", "r");
	char str[LINEDIM];
	if(file != NULL){
		fgets(str, sizeof(str), file);
		char *last = strrchr(str, ' ');
		if(last==NULL){
			printf("Error loading code.");
			exit(1);
		}
		code_size = atoi(last+1);
		program = malloc(code_size*sizeof(Acode));
		char* line;
		int p = 0;
		while(!feof(file)){
			fgets(str, sizeof(str), file);
			line = strtok(str, " ");
			if(line == NULL){
				printf("Error loading code.");
				exit(1);
			}
			for(int i=0; i<(sizeof(s_op_code)/sizeof(void*)); i++){
				if(strcmp(line,s_op_code[i])==0){
					Acode *instruction = malloc(sizeof(Acode));
					instruction->operator = i;
					program[p++] = *instruction;
				}
			}
		}
	}
	fclose(file);
}

void start_abstract_machine()
{
	load_acode();
	int i;
	for(i=0; i<code_size; i++){
		printf("%s\n", s_op_code[program[i].operator]);
	}
	/*pc = ap = op = ip = 0;
	astack = (Arecord**) newmem(sizeof(Arecord*) * ASEGMENT);
	asize = ASEGMENT;
	ostack = (Orecord**) newmem(sizeof(Orecord*) * OSEGMENT);
	osize = OSEGMENT;
	istack = (char*) newmem(ISEGMENT);
	isize = ISEGMENT;*/
}
/*
void stop_abstract_machine()
{
	freemem((char*) program, sizeof(Acode) * code_size);
	freemem((char*) astack, sizeof(Arecord*) * asize);
	freemem((char*) ostack, sizeof(Orecord*) * osize);
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

Arecord *push_activation_record()
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

//TODO check update variable 'ap' after return
Arecord* top_astack(){
	return astack[ap];
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
	po->type = ATOM;
	po->size = size;
}
 */
