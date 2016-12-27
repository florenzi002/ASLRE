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
	// Apertura in lettura del file con l'Acode
	FILE *file = fopen("./my_program.txt", "r");
	char str[LINEDIM];
	// Controllo se il puntatore a file e' nullo
	if(file != NULL){
		// Recupero la prima riga del file, che serve a sapere il numero di linee del file con l'Acode
		fgets(str, sizeof(str), file);
		char *last = strrchr(str, ' ');
		if(last==NULL){
			printf("Error loading code.");
			exit(1);
		}
		code_size = atoi(last+1);
		// Alloco la memoria necessaria a 'program', che e' un array di Acode, dove l'Acode e' la singola riga del file
		program = malloc(code_size*sizeof(Acode));
		char* line;
		int p = 0;
		// Leggo il file riga per riga finche' non arrivo all'EOF
		while(!feof(file)){
			// Leggo una riga del file
			fgets(str, sizeof(str), file);
			int str_len = strlen(str);
			// Sostituisco l'eventuale newline con il carattere di terminazione della stringa
			str[str_len-1] = (str[str_len-1]=='\n')?'\0':str[str_len-1];
			// Tokenizzo la stringa usando come separatore lo spazio
			line = strtok(str, " ");
			if(line == NULL){
				printf("Error loading code.");
				exit(1);
			}
			int i;
			for(i=0; i<(sizeof(s_op_code)/sizeof(char*)); i++){
				if(strcmp(line,s_op_code[i])==0){
					Acode *instruction = malloc(sizeof(Acode));
					instruction->operator = i;
					int j;
					/**
					for(j=0; j<NUMOPERANDS; j++) {
						instruction -> operands[j] = NULL;
					}*/

					//TODO load operandi
					j=0;
					line = strtok(NULL, " ");
					while (line!=NULL) {

					    //printf("Dentro\n");
					    Value lexval = instruction->operands[j];
					    if(((i==LOCS || i==WRIT) && (j==0)) || (i==READ && j==2)) {
					    	//lexval.sval = line;
					    	instruction -> operands[j].sval = line;
					    }
					    else {
					    	//lexval.ival = atoi(line);
					    	instruction -> operands[j].ival = atoi(line);
					    	//printf("%d\n", instruction -> operands[j].ival);
					    }
					    line = strtok(NULL, " ");
					    j++;
					    //printf("%d\n", lexval.ival);

					  //  instruction -> operands[j] = *lexval;
					}
					program[p++] = *instruction;
					break;
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
		printf("%d\n", program[i].operands[0].ival);
		/**
		if(program[i].operands[0] == NULL) {
			printf("Primo operando nullo\n");
		}
		else {

		} */
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
	case LOAD: execute_load(); break;
	case PACK: execute_pack(); break;
	case LODA: execute_loda(); break;
	case IXAD: execute_ixad(); break;
	case AIND: execute_aind(); break;
	case SIND: execute_sind(); break;
	case STOR: execute_store(); break;
	case ISTO: execute_isto(); break;
	case SKIP: execute_skip(); break;
	case SKPF: execute_skpf(); break;
	case EQUA: execute_equa(); break;
	case NEQU: execute_nequ(); break;
	case IGRT: execute_igrt(); break;
	case IGEQ: execute_igeq(); break;
	case ILET: execute_ilet(); break;
	case ILEQ: execute_ileq(); break;
	case SGRT: execute_sgrt(); break;
	case SGEQ: execute_sgeeq(); break;
	case SLET: execute_slet(); break;
	case SLEQ: execute_sleq(); break;
	case ADDI: execute_addi(); break;
	case SUBI: execute_subbi(); break;
	case MULI: execute_muli(); break;
	case DIVI: execute_divi(); break;
	case UMIN: execute_umin(); break;
	case NEGA: execute_nega(); break;
	case READ: execute_read(); break;
	case MODL: execute_modl(): break;
	case RETN: execute_retn(); break;
	case NOOP: execute_noop(); break;
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
