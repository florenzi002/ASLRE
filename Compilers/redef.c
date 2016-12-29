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

void *newmem(int);
void freemem(char *, int);
Arecord *push_activation_record();
void pop_activation_record();
void execute_push(int, int, int);
void execute_jump(int);
void execute_skip(int);
void execute_skpf(int);
void execute_retn();
void execute_addi();
void execute_igrt();
void execute_loci(int);
void execute_store(int,int);
void execute_adef(int);
Arecord *top_astack();
void print_ostack();
void print_astack();
void abstract_machine_error(char *);

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
					int j = 0;
					line = strtok(NULL, " ");
					while (line!=NULL) {
						Value lexval = instruction->operands[j];
						if(((i==LOCS || i==WRIT) && (j==0)) || (i==READ && j==2)) {
							instruction -> operands[j].sval = line;
						}
						else {
							instruction -> operands[j].ival = atoi(line);
						}
						line = strtok(NULL, " ");
						j++;
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

Orecord *push_ostack(){
	Orecord **full_ostack;
	int i;
	if(op == osize)
	{
		full_ostack = ostack;
		ostack = (Orecord**) newmem(sizeof(Orecord*) * (osize + OSEGMENT));
		for(i = 0; i < osize; i++)
			ostack[i] = full_ostack[i];
		freemem((char*) full_ostack, sizeof(Orecord*) * osize);
		osize += OSEGMENT;
	}
	return (ostack[op++] = (Orecord*) newmem(sizeof(Orecord)));
}

char *push_istack(){
	char **full_istack;
	int i;
	if(ip == isize)
	{
		full_istack = istack;
		istack = (char**) newmem(sizeof(char*) * (isize + ISEGMENT));
		for(i = 0; i < isize; i++)
			istack[i] = full_istack[i];
		freemem((char*) full_istack, sizeof(char*) * isize);
		isize += ISEGMENT;
	}
	return (ostack[ip++] = (char*) newmem(sizeof(char*)));
}


void pop_activation_record()
{
	if(ap == 0)
		abstract_machine_error("Failure in popping activation record");
	freemem((char*) astack[--ap],
			sizeof(Arecord));
}

void pop_ostack()
{
	if(op == 0)
		abstract_machine_error("Failure in popping object stack record");
	freemem((char*) ostack[--op],
			sizeof(Orecord));
}

void pop_istack()
{
	if(ip == 0)
		abstract_machine_error("Failure in popping instance stack record");
	freemem((char*) istack[--ip],
			sizeof(char*));
}

void execute(Acode *instruction)
{
	printf("%d - %s\n", instruction->operator, s_op_code[instruction->operator]);
	switch(instruction->operator)
	{
	case PUSH: execute_push(instruction->operands[0].ival, instruction->operands[1].ival, instruction->operands[2].ival); break;
	case JUMP: execute_jump(instruction->operands[0].ival); break;
	case APOP: pop_activation_record(); break;
	case ADEF: execute_adef(instruction->operands[0].ival); break;
	//case SDEF: execute_sdef(instruction->operands[0].ival); break;
	case LOCI: execute_loci(instruction->operands[0].ival); break;
	/*case LOCS: execute_locs(instruction->operands[0].sval); break;
	case LOAD: execute_load(); break;
	case PACK: execute_pack(); break;
	case LODA: execute_loda(); break;
	case IXAD: execute_ixad(); break;
	case AIND: execute_aind(); break;
	case SIND: execute_sind(); break; */
	case STOR: execute_store(instruction->operands[0].ival, instruction->operands[1].ival); break;
	/*case ISTO: execute_isto(); break;
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
	case SLEQ: execute_sleq(); break;*/
	case ADDI: execute_addi(); break;
	/*case SUBI: execute_subbi(); break;
	case MULI: execute_muli(); break;
	case DIVI: execute_divi(); break;
	case UMIN: execute_umin(); break;
	case NEGA: execute_nega(); break;
	case READ: execute_read(); break;
	case MODL: execute_modl(): break;
	case NOOP: execute_noop(); break;
	// TODO
	case RETN: execute_retn(); break;*/
	default: abstract_machine_error("Unknown operator"); break;
	}
}

void execute_store(int chain, int oid) {
	print_ostack();
	Arecord* target_ar = astack[ap-1];
	int i;
	for(i=0; i<chain; i++) {
		target_ar = target_ar->al;
	}
	Orecord *object = target_ar -> head + oid;
}

void execute_push(int num_formals_aux, int num_loc, int chain){
	Arecord *ar = push_activation_record();
	ar->head = ostack[op-num_formals_aux];
	ar->objects = num_loc;
	ar->retad = pc+1;
	int i=0;
	if(chain<=0)
		ar->al=NULL;
}

void execute_jump(int address)
{
	pc = address;
}

void execute_loci(int const_val){
	push_istack();
	istack[ip-1] = const_val;
}
/*
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
}*/

void execute_addi()
{
	int n, m;
	/*n = pop_int();
	m = pop_int();
	push_int(m+n);*/
}
/*
void execute_igrt()
{
	int n, m;
	n = pop_int();
	m = pop_int();
	push_bool(m>n);
}*/

void execute_adef(int size)
{
	Orecord *po;
	po = push_ostack();
	po->type = ATOM;
	po->size = size;
}

void print_ostack(){
	int i;
	for(i=0; i<op; i++){
		printf("%d\n", ostack[i]->type);
	}
}

void print_astack() {
    int i;
    for(i=0; i<asize;i++) {
     printf("Activation record n° %d\n",i);
     printf("    ");
     printf("Number of objects: %d\n", astack[i]->objects);
     printf("    ");
     printf("First object of this AR: %d\n", astack[i]->head->instance.ival);
     printf("    ");
     printf("Return address: %d\n", astack[i]->retad);
    }
}

void abstract_machine_error(char* error){
	printf("%s\n", error);
	exit(1);
}
