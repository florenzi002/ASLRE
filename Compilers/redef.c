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
void execute_subi();
void execute_muli();
void execute_divi();
void execute_igrt();
void execute_igeq();
void execute_ilet();
void execute_ileq();
void execute_sgrt();
void execute_sgeq();
void execute_slet();
void execute_sleq();
void execute_loci(int);
void execute_locs(char*);
void execute_store(int,int);
void execute_adef(int);
void execute_sdef(int);
void execute_nega();
void execute_umin();
void execute_nequ();
void execute_equa();
unsigned char *pop_n_istack(int);
void pop_ostack();
int pop_int();
int endian();
void push_int(int);
Arecord *top_astack();
Orecord *top_ostack();
char *top_istack();
void print_ostack();
void print_astack();
void print_istack();
int endian();
void abstract_machine_Error(char *);

#define ENDIANESS endian()

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
		if(fgets(str, sizeof(str), file)==NULL)
			abstract_machine_Error("Error loading code");
		char *last = strrchr(str, ' ');
		if(last==NULL)
			abstract_machine_Error("Error loading code");
		code_size = atoi(last+1);
		// Alloco la memoria necessaria a 'program', che e' un array di Acode, dove l'Acode e' la singola riga del file
		program = malloc(code_size*sizeof(Acode));
		char* line;
		int p = 0;
		// Leggo il file riga per riga finche' non arrivo all'EOF
		while(!feof(file)){
			// Leggo una riga del file
			if(fgets(str, sizeof(str), file)==NULL)
				abstract_machine_Error("Error loading code");
			int str_len = strlen(str);
			// Sostituisco l'eventuale newline con il carattere di terminazione della stringa
			str[str_len-1] = (str[str_len-1]=='\n')?'\0':str[str_len-1];
			// Tokenizzo la stringa usando come separatore lo spazio
			line = strtok(str, " ");
			if(line == NULL)
				abstract_machine_Error("Error loading code");
			int i;
			for(i=0; i<(sizeof(s_op_code)/sizeof(char*)); i++){
				if(strcmp(line,s_op_code[i])==0){
					Acode *instruction = malloc(sizeof(Acode));
					instruction->operator = i;
					int j = 0;
					line = strtok(NULL, " ");
					while (line!=NULL) {
						if(((i==LOCS || i==WRIT) && (j==0)) || (i==READ && j==2)) {
							char *string = (char*)malloc(sizeof(char)*(strlen(line)-1));
							int counter, pos;
							counter = pos = 0;
							char c = *line;
							while(c!='\0'){
								if(c!='"'){
									string[pos++]=c;
								}
								c=*(line+(++counter));
							}
							string[++counter]='\0';
							instruction -> operands[j].sval = string;
				
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
	/*printf("ENDIANESS: %d\n",ENDIANESS);
	//char c[] = "abc";
	char *pc = (char*)1000;
	int q = 1000;
	unsigned char *pb_p = (unsigned char *)&pc;
	unsigned char *pb_i = (unsigned char *)&q;
	int i=0;
	for(;i<sizeof(char*);i++)
		printf("[%X]",pb_p[i]);
	printf("\n");
	Value *v = malloc(sizeof(Value));
	//v->sval=(char*)1000;
	v->ival=1000;
	unsigned char *pb_v = (unsigned char *)v;
	for(i=0;i<sizeof(Value);i++)
		printf("[%X]",pb_v[i]);
	printf("\n");*/
	load_acode();
	pc = 0; ap = 0; op = 0; ip = 0;
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
	printf("Program executed without Errors\n");
	printf("Allocation: %ld bytes\n", allocated);
	printf("Deallocation: %ld bytes\n", deallocated);
	printf("Residue: %ld bytes\n", allocated - deallocated);
}

void *newmem(int size)
{
	void *p;
	if((p = malloc(size)) == NULL)
		abstract_machine_Error("Failure in memory allocation");
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

Arecord *top_astack(){
	if (ap==0) {
		abstract_machine_Error("Failure accessing top of Activation Stack");
	}
	return astack[ap-1];
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

Orecord *top_ostack(){
	if (op==0) {
		abstract_machine_Error("Failure accessing top of Activation Stack");
	}
	return ostack[op-1];
}

char *push_istack(){
	char *full_istack;
	int i;
	if(ip == isize)
	{
		full_istack = istack;
		istack = (char*) newmem(sizeof(char*) * (isize + ISEGMENT));
		for(i = 0; i < isize; i++)
			istack[i] = full_istack[i];
		freemem((char*) full_istack, sizeof(char*) * isize);
		isize += ISEGMENT;
	}
	// Ritorno l'indirizzo all'ultimo elemento
	return &(istack[ip++]);
}

char *top_istack(){
	if (ip==0) {
		abstract_machine_Error("Failure accessing top of Instance Stack");
	}
	return &(istack[ip-1]);
}

int endian(){
	int i=1;
	char *p = (char *)&i;
	if(p[0]==1)
		return 0;
	return 1;
}

/**
 * Alloca memoria per un nuovo elemento nell'istack e gli assegna un valore
 * @param i il valore da assegnare
 */
void push_int(int i){
	execute_adef(INTSIZE);
	top_ostack()->instance.sval=&(istack[ip]);
	unsigned char *i_bytes=(unsigned char*)&i;
	int p;
	if(ENDIANESS){
		p=0;
		for(;p<INTSIZE;p++){
			push_istack();
			*top_istack()=i_bytes[p];
		}
	}else{
		p=INTSIZE-1;
		for(;p>=0;p--){
			push_istack();
			*top_istack()=i_bytes[p];
		}
	}
}

void push_string(char* s){
	execute_adef(PTRSIZE);
	top_ostack()->instance.sval=&(istack[ip]);
	unsigned char *s_bytes=(unsigned char*)&s;
	int p;
	if(ENDIANESS){
		p=0;
		for(;p<PTRSIZE;p++){
			push_istack();
			*top_istack()=s_bytes[p];
		}
	}else{
		p=PTRSIZE-1;
		for(;p>=0;p--){
			push_istack();
			*top_istack()=s_bytes[p];
		}
	}
}

/**
 * Alloca memoria per un nuovo elemento nell'istack e gli assegna un valore
 * @paramm i il valore da assegnare (0:falso, 1:true)
 */
void push_bool(int i) {
	push_istack();
	istack[ip-1] = i;
}

void pop_activation_record()
{
	if(ap == 0)
		abstract_machine_Error("Failure in popping activation record");
	freemem((char*) astack[--ap],
			sizeof(Arecord));
}

void pop_ostack()
{
	if(op == 0)
		abstract_machine_Error("Failure in popping object stack record");
	freemem((char*) ostack[--op],
			sizeof(Orecord));
}

void pop_istack()
{
	if(ip == 0)
		abstract_machine_Error("Failure in popping instance stack record");
	ip--;
}

unsigned char *pop_n_istack(int n){
	unsigned char *i_bytes = malloc(n*sizeof(char));
	int p = 0;
	char *q = top_istack()+1-n;
	for(;p<n;p++){
		i_bytes[p]=*(q++);
		pop_istack();
	}
	if(!ENDIANESS){
		int i=-1;
		while((++i)<(--p)){
			unsigned char temp = i_bytes[i];
			i_bytes[i]=i_bytes[p];
			i_bytes[p]=temp;
		}
	}
	return i_bytes;
}

int pop_int(){
	unsigned char *i_bytes = pop_n_istack(INTSIZE);
	pop_ostack();
	int i=0,res=0;
	for(;i<INTSIZE;i++){
		res = res|(i_bytes[i])<<(ENDIANESS?(24-8*i):(8*i));
	}
	freemem((char *)i_bytes, INTSIZE);
	printf("INTEGER: %d\n", res);
	return res;
}

char* pop_string(){
	unsigned char *i_bytes = pop_n_istack(PTRSIZE);
	pop_ostack();
	return (char*) i_bytes;
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
	case LOCS: execute_locs(instruction->operands[0].sval); break;
	/*case LOAD: execute_load(); break;
	case PACK: execute_pack(); break;
	case LODA: execute_loda(); break;
	case IXAD: execute_ixad(); break;
	case AIND: execute_aind(); break;
	case SIND: execute_sind(); break; */
	case STOR: execute_store(instruction->operands[0].ival, instruction->operands[1].ival); break;
	case IGRT: execute_igrt(); break;
	case IGEQ: execute_igeq(); break;
	case ILET: execute_ilet(); break;
	case ILEQ: execute_ileq(); break;
	case SGRT: execute_sgrt(); break;
	case SGEQ: execute_sgeq(); break;
	case SLET: execute_slet(); break;
	case SLEQ: execute_sleq(); break;
	/*case ISTO: execute_isto(); break;
	case SKIP: execute_skip(); break;
	case SKPF: execute_skpf(); break;
	case EQUA: execute_equa(); break;
	case NEQU: execute_nequ(); break;
	 */
	case ADDI: execute_addi(); break;
	case SUBI: execute_subi(); break;
	case MULI: execute_muli(); break;
	case DIVI: execute_divi(); break;

	case UMIN: execute_umin(); break;
	case NEGA: execute_nega(); break;
	/**case READ: execute_read(); break;
	case MODL: execute_modl(): break;
	case NOOP: execute_noop(); break;
	// TODO
	case RETN: execute_retn(); break;*/
	default: abstract_machine_Error("Unknown operator"); break;
	}
}

void execute_store(int chain, int oid) {
	Arecord* target_ar = top_astack();
	int i;
	for(i=0; i<chain; i++) {
		target_ar = target_ar->al;
	}
	Orecord* p_obj = *(target_ar->head + oid);
	int size = top_ostack()->size;
	unsigned char *bytes = pop_n_istack(size);
	pop_ostack();
	memcpy(&(p_obj->instance),bytes, size);

}

void execute_push(int num_formals_aux, int num_loc, int chain){
	Arecord *ar = push_activation_record();
	ar->head = &ostack[op-num_formals_aux];
	ar->objects = num_loc+num_formals_aux;
	ar->retad = pc+1;
	if(chain<=0)
		ar->al=NULL;
}

void execute_jump(int address){
	pc = address;
}

void execute_loci(int const_val){
	push_int(const_val);
}

void execute_locs(char* const_val){
	push_string(const_val);
}

void execute_umin(){
	int m = pop_int();
	push_int(-m);
}

void execute_nega() {
	int m = pop_int();
	if(m==1) {
		push_int(0);
	}
	else {
		push_int(1);
	}
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
	n = pop_int();
	m = pop_int();
	push_int(m+n);
	print_ostack();
	print_istack();
}

void execute_subi()
{
	int n, m;
	n = pop_int();
	m = pop_int();
	push_int(m-n);
}

void execute_muli()
{
	int n, m;
	n = pop_int();
	m = pop_int();
	push_int(m*n);
}

void execute_divi()
{
	int n, m;
	n = pop_int();
	if(n==0)
		abstract_machine_Error("Error: Divide by 0");
	m = pop_int();
	push_int(m/n);
}

/**
 * Confronta due interi recuperati dall'Instance stack e stabilisce se il primo e' maggiore del secondo.
 * In caso affermativo viene inserito nell'Instance stack il valore 1, altrimenti il valore 0
 */
void execute_igrt()
{
	int n, m;
	n = pop_int();
	m = pop_int();
	push_bool(m>n);
}

/**
 * Confronta due interi recuperati dall'Instance stack e stabilisce se il primo e' maggiore o uguale al secondo.
 * In caso affermativo viene inserito nell'Instance stack il valore 1, altrimenti il valore 0
 */
void execute_igeq()
{
	int n, m;
	n = pop_int();
	m = pop_int();
	push_bool(m>=n);
}

/**
 * Confronta due interi recuperati dall'Instance stack e stabilisce se il primo e' minore del secondo.
 * In caso affermativo viene inserito nell'Instance stack il valore 1, altrimenti il valore 0
 */
void execute_ilet()
{
	int n, m;
	n = pop_int();
	m = pop_int();
	push_bool(m<n);
}

/**
 * Confronta due interi recuperati dall'Instance stack e stabilisce se il primo e' minore o uguale al secondo.
 * In caso affermativo viene inserito nell'Instance stack il valore 1, altrimenti il valore 0
 */
void execute_ileq() {
	int n, m;
	n = pop_int();
	m = pop_int();
	push_bool(m<=n);
}

/**
 * Confronta due stringhe ottenute recuperando il puntatore dall'Instance Stack e stabilisce se la prima è maggiore della
 * seconda. In caso affermativo viene inserito nell'Instance stack il valore 1, altrimenti il valore 0.
 */
void execute_sgrt() {
	const char* s2 = pop_string();
	const char* s1 = pop_string();
	if(strcmp(s1,s2) == 1) { // s1 > s2
		push_bool(1);
	}
	else { // s1 <= s2
		push_bool(0);
	}
}

/**
 * Confronta due stringhe ottenute recuperando il puntatore dall'Instance Stack e stabilisce se la prima è maggiore o
 * uguale alla seconda. In caso affermativo viene inserito nell'Instance Stack il valore 1, altrimenti il valore 0.
 */
void execute_sgeq() {
	const char* s2 = pop_string();
	const char* s1 = pop_string();
	if(strcmp(s1,s2) == -1) { // s1 < s2
		push_bool(0);
	}
	else { // s1 >= s2
		push_bool(1);
	}
	printf("SGEQ");
	print_istack();
}

/**
 * Confronta due stringhe ottenute recuperando il puntatore dall'Instance Stack e stabilisce se la prima è minore della
 * seconda. In caso affermativo viene inserito nell'Instance Stack il valore 1, altrimenti il valore 0.
 */
void execute_slet() {
	const char* s2 = pop_string();
	const char* s1 = pop_string();
	if(strcmp(s1,s2) == -1) { // s1 < s2
		push_bool(1);
	}
	else { // s1 >= s2
		push_bool(0);
	}
}

/**
 * Confronta due stringhe ottenute recuperando il puntatore dall'Instance Stack e stabilisce se la prima è minore della
 * seconda. In caso affermativo viene inserito nell'Instance Stack il valore 1, altrimenti il valore 0.
 */
void execute_sleq() {
	const char* s2 = pop_string();
	const char* s1 = pop_string();
	if(strcmp(s1,s2) == 1) { // s1 > s2
		push_bool(0);
	}
	else { // s1 <= s2
		push_bool(1);
	}
}

void execute_adef(int size)
{
	Orecord *po;
	po = push_ostack();
	po->type = ATOM;
	po->size = size;
}

void execute_sdef(int size)
{
	Orecord *po;
	po = push_ostack();
	po->type = VECTOR;
	po->size = size;
}

/**
 * Confronta due oggetti (operatore ==): se questi sono uguali, carica nell'instance stack il valore 1, altrimenti carica
 * il valore 0
 */
void execute_equa()
{
	int n,m;
	n = pop_int();
	m = pop_int();
	push_bool(m==n);
}

/**
 * Confronta due oggetti (operatore !=): se questi sono diversi, carica nell'instance stack il valore 1, altrimenti carica
 * il valore 0
 */
void execute_nequ()
{
	int n,m;
	n = pop_int();
	m = pop_int();
	push_bool(m!=n);
}

void print_ostack(){
	int i;
	printf("# objects on ostack: %d\n", op);
	for(i=0; i<op; i++){
		printf("size of object: %d\n", ostack[i]->size);
		printf("value of object: %s\n", ostack[i]->instance.sval);
	}
}

void print_astack() {
	int i;
	for(i=0; i<ap;i++) {
		printf("Activation record n° %d\n",i);
		printf("    ");
		printf("Number of objects: %d\n", astack[i]->objects);
		printf("    ");
		printf("First object_size of this AR: %d\n", (*astack[i]->head)->size);
		printf("    ");
		printf("First object_ival of this AR: %d\n", (*astack[i]->head)->instance.ival);
		printf("    ");
		printf("Return address: %d\n", astack[i]->retad);
	}
}

void print_istack() {
	int i=0;
	for(i=0; i<ip; i++) {
		printf("Instance stack record n° %d\n", i);
		printf("    ");
		printf("Valore: %02X\n", istack[i]);
	}
}

void abstract_machine_Error(char* Error){
	printf("%s\n", Error);
	exit(1);
}
