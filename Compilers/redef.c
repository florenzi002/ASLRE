/*
 * redef.c
 *
 *  Created on: 09/dic/2016
 *      Author: Fabio Lorenzi & Cristian Sampietri
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "redef.h"
#include "functionHash.h"

// Prototipi delle funzioni
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
int pop_bool();
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

#define ENDIANESS endian()  // Costante per tenere traccia dell'Endianess della macchina

// Array di stringhe contenente tutti i codici corrispondenti ai vari operatori
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

// Costanti che indicano per quanti elementi bisogna allocare memoria, rispettivamente, per Astack, Ostack e Istack
const int ASEGMENT = 10;
const int OSEGMENT = 10;
const int ISEGMENT = 10;

const int LINEDIM = 128;

Acode *program;
int pc;
Arecord **astack;        // Activation Record
Orecord **ostack;        // Object Record
char *istack;            // Singola cella dell'Instance Stack (cioè una singola cella di memoria
int asize, osize, isize; // Dimensioni (numero di elementi) dei vari stack
int code_size;           // Numero di righe che costituiscono il file con l'Acode

int ap, op, ip;          // Variabili che tengono traccia della posizione nei vari stack (prima cella libera)

long allocated = 0, deallocated = 0; // Variabili che tengono traccia della memoria allocata e di quella deallocata

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
		// Recupero la prima riga del file (ACODE size), che serve per sapere il numero di linee del file con l'Acode
		if(fgets(str, sizeof(str), file)==NULL)
			abstract_machine_Error("Error loading code");
		// Cerco la prima occorrenza dello spazio nella riga
		char *last = strrchr(str, ' '); // Last punta alla prima occorrenza di ' ' (se lo trova)
		if(last==NULL)
			abstract_machine_Error("Error loading code");
		// Recupero size dalla prima riga
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
			// Tokenizzo la stringa usando come separatore lo spazio. In questo modo posso recuperare l'operatore e gli operandi
			line = strtok(str, " "); // line contiene ora il codice dell'operatore
			if(line == NULL)
				abstract_machine_Error("Error loading code");
			int i;
			//
			for(i=0; i<(sizeof(s_op_code)/sizeof(char*)); i++){
				if(strcmp(line,s_op_code[i])==0) { // line coincide con uno dei codici
					Acode *instruction = malloc(sizeof(Acode)); // Alloco memoria per una nuova struct Acode
					instruction->operator = i;  // Assegno il campo operatore della struct con la stringa corrispondente (p.e. LOCI, LOCS, ecc.)

					// Procedo a recuperare gli eventuali operatori associati all'Acode
					int j = 0;
					line = strtok(NULL, " ");
					while (line!=NULL) {
						// Verifico se devo recuperare una stringa (primo operando di LOCS o WRITE, oppure secondo operando della READ
						if(((i==LOCS || i==WRIT) && (j==0)) || (i==READ && j==2)) {      // L'operando è una stringa
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
							string[pos]='\0';
							// Assegno il sval del j-esimo operando della riga di Acode.
							instruction -> operands[j].sval = insertFind(hash(string), string);
						}
						else {  // L'operando è un intero
							instruction -> operands[j].ival = atoi(line); // Assegno l'ival del j-esimo operando della riga di Acode
						}
						line = strtok(NULL, " ");
						j++;
					}
					program[p++] = *instruction; // Aggiungo l'istruzione (una struct Acode) al programma (array di Acode)
					break;
				}
			}
		}
	}
	fclose(file);  // Chiusura del file
}

/**
 * Funzione che avvia la macchina virtuale effettuando le necessarie inizializzazioni e allocazioni di memoria
 */
void start_abstract_machine()
{
	load_acode(); // Funzione per la lettura del file con l'Acode
	pc = 0; ap = 0; op = 0; ip = 0; // Inizializzazione dei contatori

	// Allocazione della memoria per i vari stack
	astack = (Arecord**) newmem(sizeof(Arecord*) * ASEGMENT);
	asize = ASEGMENT;
	ostack = (Orecord**) newmem(sizeof(Orecord*) * OSEGMENT);
	osize = OSEGMENT;
	istack = (char*) newmem(ISEGMENT);
	isize = ISEGMENT;
}

/**
 * Funzione che arresta la macchina virtuale, liberando la memoria allocata per gli stack e per l'array di Acode.
 * Vengono inoltre stampate informazioni sulla memoria allocata, deallocata e residua
 */
void stop_abstract_machine()
{
	// Viene liberata la memoria allocata
	freemem((char*) program, sizeof(Acode) * code_size);
	freemem((char*) astack, sizeof(Arecord*) * asize);
	freemem((char*) ostack, sizeof(Orecord*) * osize);
	freemem(istack, isize);

	// Stampa a video delle informazioni sull'esecuzione del programma
	printf("Program executed without Errors\n");
	printf("Allocation: %ld bytes\n", allocated);
	printf("Deallocation: %ld bytes\n", deallocated);
	printf("Residue: %ld bytes\n", allocated - deallocated);
}

/**
 * Funzione che effettua l'allocazione della memoria chiamando la funzione malloc
 *
 * @param size --> la quantita' di memoria da allocare
 *
 * @return p --> Puntatore alla memoria allocata
 */
void *newmem(int size)
{
	void *p;
	if((p = malloc(size)) == NULL)
		abstract_machine_Error("Failure in memory allocation");
	allocated += size; // Viene aggiornata la variabile che tiene traccia della memoria allocata
	return p;
}

/**
 * Funzione che effettua la deallocazione della memoria chiamando la funzione free
 *
 * @param *p --> Puntatore alla memoria da deallocare
 * @param size --> Quantita' di memoria da deallocare
 */
void freemem(char *p, int size)
{
	free(p);
	deallocated += size;  // Aggiorno la variabile che tiene traccia della memoria deallocata
}

/**
 * Funzione che esegue il push di un Activation Record, aggiungendolo all'astack
 *
 * @return l'Activation Record creato
 */
Arecord *push_activation_record()
{
	Arecord **full_astack;
	int i;
	if(ap == asize) // Verifico se l'astack e' pieno
	{
		// Devo allocare nuovamente la memoria, quindi copio l'astack nella variabile temporanea
		full_astack = astack;
		// Alloco nuova memoria per l'astack
		astack = (Arecord**) newmem(sizeof(Arecord*) * (asize + ASEGMENT));
		for(i = 0; i < asize; i++)
			astack[i] = full_astack[i];
		// Libero la memoria allocata per la variabile di supporto
		freemem((char*) full_astack, sizeof(Arecord*) * asize);
		// Aggiorno la dimensione (numero di Arecord) dell'Activation Stack
		asize += ASEGMENT;
	}
	// Restituisco l'Activation Record creato
	return (astack[ap++] = (Arecord*) newmem(sizeof(Arecord)));
}

/**
 * Funzione che recupera l'Activation Record in cima allo stack (cioe' quello presente nella posizione appena sopra a quella
 * puntata da ap, che indica la prima posizione libera)
 *
 * @return l'Activation Record in cima all'Activation Stack
 */
Arecord *top_astack(){
	if (ap==0) { // Errore: Astack vuoto
		abstract_machine_Error("Failure accessing top of Activation Stack");
	}
	return astack[ap-1];
}

/**
 * Funzione che esegue il push di un Object Record, aggiungendolo all'Obect Stack
 *
 * @return l'Object Record appena creato
 */
Orecord *push_ostack(){
	Orecord **full_ostack;
	int i;
	if(op == osize) // Verifico se l'Obect Stack e' pieno
	{
		// Devo allocare nuovamente la memoria, quindi copio l'astack nella variabile temporanea
		full_ostack = ostack;
		// Alloco nuova memoria per l'ostack
		ostack = (Orecord**) newmem(sizeof(Orecord*) * (osize + OSEGMENT));
		for(i = 0; i < osize; i++)
			ostack[i] = full_ostack[i];
		// Libero la memoria allocata per la variabile di supporto
		freemem((char*) full_ostack, sizeof(Orecord*) * osize);
		osize += OSEGMENT;
	}
	// Restituisco l'Object Record creato
	return (ostack[op++] = (Orecord*) newmem(sizeof(Orecord)));
}

/**
 * Funzione che recupera l'Object Record in cima allo stack (cioe' quello presente nella posizione appena sopra a quella
 * puntata da op, che indica la prima posizione libera)
 *
 * @return l'Object Record in cima all'Activation Stack
 */
Orecord *top_ostack(){
	if (op==0) { // Errore: Object Stack vuoto
		abstract_machine_Error("Failure accessing top of Activation Stack");
	}
	return ostack[op-1];
}

/**
 * Funzione che alloca un nuovo elemento all'Instance Stack,
 *
 * @return l'indirizzo della nuova cella creata nell'istack
 */
char *push_istack(){
	char *full_istack;
	int i;
	if(ip == isize) // Verifico se l'Instance Stack e' pieno
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

/**
 * Funzione che recupera la cella dell'Instance Stack che si trova in cima allo stack (cioe' quella presente nella
 * posizione appena sopra a quella puntata da ip, che indica la prima posizione libera)
 *
 * @return l'Object Record in cima all'Activation Stack
 */
char *top_istack(){
	if (ip==0) { // Errore: Instance Stack vuoto
		abstract_machine_Error("Failure accessing top of Instance Stack");
	}
	return &(istack[ip-1]);
}

/**
 * Funzione che verifica l'Endianess della macchina
 */
int endian(){
	int i=1;
	char *p = (char *)&i;
	if(p[0]==1)
		return 0;  // Little Endian
	return 1;      // Big Endian
}

/**
 * Alloca memoria per un nuovo intero nell'istack e gli assegna un valore
 * @param i --> L'intero da mettere nell'Instance Stack
 */
void push_int(int i){
	// Chiamo ADEF per creare l'oggetto nell'Object Stack, passandogli come parametro la dimensione di un intero
	execute_adef(INTSIZE);
	top_ostack()->instance.sval=&(istack[ip]);
	// Trasformo l'intero in sequenza di bytes
	unsigned char *i_bytes=(unsigned char*)&i;
	int p;
	// A seconda dell'Endianess cambia l'ordine con cui i vari byte che formano l'intero vengono memorizzati
	if(ENDIANESS){ // Big Endian (la memorizzazione inizia dal byte piu' significativo)
		p=0;
		// Memorizzo byte per byte nell'istack
		for(;p<INTSIZE;p++){
			push_istack();
			*top_istack()=i_bytes[p];
		}
	}else{ // Little Endian (la memorizzazione inizia dal byte meno significativo)
		p=INTSIZE-1;
		for(;p>=0;p--){
			push_istack();
			*top_istack()=i_bytes[p];
		}
	}
}

/**
 * Funzione che memorizza nell'Instance stack il puntatore alla stringa
 *
 * @param s --> Il puntatore alla stringa che deve essere messo nell'Istack
 */
void push_string(char* s){
	// Chiamo l'ADEF per creare l'oggetto nell'Object Stack, passandogli come parametro la dimensione in bytes di un puntatore
	execute_adef(PTRSIZE);
	top_ostack()->instance.sval=&(istack[ip]);
	// Converto in una sequenza di bytes
	unsigned char *s_bytes=(unsigned char*)&s;
	int p;
	// A seconda dell'Endianess cambia l'ordine con cui i vari byte che formano il puntatore vengono memorizzati
	if(ENDIANESS){ // Big Endian (la memorizzazione inizia dal byte piu' significativo)
		p=0;
		// Memorizzo byte per byte nell'istack
		for(;p<PTRSIZE;p++){
			push_istack();
			*top_istack()=s_bytes[p];
		}
	}else{ // Little Endian (la memorizzazione inizia dal byte meno significativo)
		p=PTRSIZE-1;
		// Memorizzo byte per byte nell'istack
		for(;p>=0;p--){
			push_istack();
			*top_istack()=s_bytes[p];
		}
	}
}

/**
 * Funzione che memorizza nell'Instance Stack uno zero o un uno
 *
 * @paramm i --> il valore da assegnare (0:falso, 1:true)
 */
void push_bool(int i) {
	push_int(i);
}

/**
 * Funzione che rimuove un Activation Record dalla cima dell'Activation Stack
 */
void pop_activation_record()
{
	if(ap == 0) // Errore: Activation Stack vuoto
		abstract_machine_Error("Failure in popping activation record");
	// Libera la memoria allocata per l'Activation Record che si trova in cima all'astack
	freemem((char*) astack[--ap],
			sizeof(Arecord));
}

/**
 * Funzione che rimuove un Obect Record dalla cima dell'Object Stack
 */
void pop_ostack()
{
	if(op == 0) // Errore: Object Stack vuoto
		abstract_machine_Error("Failure in popping object stack record");
	// Libera la memoria allocata per l'Object Record che si trova in cima all'ostack
	freemem((char*) ostack[--op],
			sizeof(Orecord));
}

/**
 * Funzione che decrementa l'Instance Pointer di una unita'
 */
void pop_istack()
{
	if(ip == 0) // Errore: Instance Stack vuoto
		abstract_machine_Error("Failure in popping instance stack record");
	ip--;
}

/**
 * Funzione che recupera n bytes a partire dalla cima dell'Instance Stack
 *
 * @param n --> Specifica il numero di bytes da recuperare
 *
 * @return --> Puntatore ai bytes presi dall'Instance Stack
 */
unsigned char *pop_n_istack(int n){
	unsigned char *i_bytes = malloc(n*sizeof(char));
	int p = 0;
	// Risalgo nell'Instance Stack del numero di posizioni necessario per recuperare la prima
	char *q = top_istack()+1-n;
	for(;p<n;p++){
		// A ciascun byte di i_bytes assegno il valore di ciò che punta q
		i_bytes[p]=*(q++);
		// Decremento l'Instance Pointer
		pop_istack();
	}
	if(!ENDIANESS){ // Little Endian
		int i=-1;
		// Inverto il contenuto dell'array
		while((++i)<(--p)){
			unsigned char temp = i_bytes[i];
			i_bytes[i]=i_bytes[p];
			i_bytes[p]=temp;
		}
	}
	return i_bytes;
}

/**
 * Funzione che recupera un intero a partire dalla cima dell'Instance Stack
 *
 * @return --> L'intero recuperato dall'Instance Stack
 */
int pop_int(){
	// Chiamo la funzione per recuperare dalla cima dell'Instance Stack un numero di bytes pari a INTSIZE (che in genere e' 4)
	unsigned char *i_bytes = pop_n_istack(INTSIZE);
	// Rimuovo l'Object Record associato all'intero recuperato
	pop_ostack();
	int i=0,res=0;
	for(;i<INTSIZE;i++){
		res = res|(i_bytes[i])<<(ENDIANESS?(24-8*i):(8*i));
	}
	// TODO freemem o free?
	freemem((char *)i_bytes, INTSIZE);
	printf("INTEGER: %d\n", res);
	return res;
}

/**
 * Funzione che recupera una stringa dalla cima dell'Instance Stack
 *
 * @return --> Puntatore alla stringa recuperata
 */
char* pop_string(){
	// Chiamo la funzione per recuperare dalla cima dell'Instance Stack un numero di bytes pari a PTRSIZE
	unsigned char *i_bytes = pop_n_istack(PTRSIZE);
	// Rimuovo l'Object Record associato al puntatore
	pop_ostack();
	// Alloco la memoria per il puntatore alla stringa da restituire
	char* s = malloc(PTRSIZE);
	// Copio PTRSIZE bytes da i_bytes all'indirizzo di s
	memcpy(&s,i_bytes,PTRSIZE);
	freemem((char *)i_bytes, PTRSIZE);
	return s;
}

int pop_bool(){
	return pop_int();
}

/**
 * Funzione che esegue una singola istruzione (cioe' una singola riga del file con l'ACode
 *
 * @param instruction --> Puntatore alla struttura Acode contenente l'operatore e gli operandi dell'istruzione da eseguire
 */
void execute(Acode *instruction)
{
	// printf("%d - %s\n", instruction->operator, s_op_code[instruction->operator]);
	// A seconda dell'operatore, chiamo la funzione che esegue la corrispondente istruzione
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
	 */
	case EQUA: execute_equa(); break;
	case NEQU: execute_nequ(); break;
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

/**
 * Funzione che esegue l'istruzione STORE per l'assegnamento di un identificatore
 *
 * @param chain --> Distanza nell'ambiente (indica di quanti passi bisogna risalire nella catena statica)
 * @param oid --> L'oid dell'oggetto assegnato
 */
void execute_store(int chain, int oid) {
	Arecord* target_ar = top_astack();
	int i;
	// Tramite l'access link, risalgo di un numero di Activation Record pari a quello specificato nel parametro chain
	for(i=0; i<chain; i++) {
		target_ar = target_ar->al;
	}
	// Recupero dall'Object Stack l'oggetto relativo all'identificatore assegnato, sfruttando il campo head dell'Activation Record e l'oid dell'oggetto
	Orecord* p_obj = *(target_ar->head + oid);
	int size = top_ostack()->size;
	// Recupero dall'istack un numero di bytes specificato dal campo size dell'Orecord
	unsigned char *bytes = pop_n_istack(size);
	// Rimuovo l'Object Record dalla cima dell'Object Stack
	pop_ostack();
	memcpy(&(p_obj->instance),bytes, size);
}

/**
 * Funzione che esegue l'istruzione PUSH per creare l'Activation Record all'atto della chiamata di funzione
 *
 * @param num_formals_aux --> Numero di parametri formali + ausiliari
 * @param num_loc --> Numero di costanti locali
 * @param chain --> Distanza tra l'ambiente del chiamante e l'ambiente in cui la funzione e' definita
 */
void execute_push(int num_formals_aux, int num_loc, int chain){
	// Chiamo la funzione per allocare un nuovo Activation Record e aggiungerlo all'Activation Stack
	Arecord *ar = push_activation_record();
	//
	ar->head = &ostack[op-num_formals_aux];
	// Il numero di oggetti associati all'Activation Record e' dato dal numero dei locali + il numero dei parametri formali e ausiliari
	ar->objects = num_loc+num_formals_aux;
	// Il return address viene settato come program-counter+1
	ar->retad = pc+1;
	if(chain<=0)
		ar->al=NULL;
}

/**
 * Funzione che esegue il salto incondizionato (operatore JUMP)
 *
 * @param address --> L'indirizzo a cui saltare
 */
void execute_jump(int address){
	pc = address;
}

/**
 * Funzione che esegue l'istruzione per l'operatore LOCI per le costanti intere
 *
 * @param const_val --> L'intero da mettere nell'Instance stack
 */
void execute_loci(int const_val){
	push_int(const_val);
}

/**
 * Funzione che esegue l'istruzione per l'operatore LOCS per le costanti stringa
 *
 * @param const_val --> Puntatore da aggiungere nell'Instance Stack
 */
void execute_locs(char* const_val){
	push_string(const_val);
}

/**
 * Funzione che esegue l'istruzione per l'operatore UMIN, che fa l'opposto di un intero
 */
void execute_umin(){
	int m = pop_int();
	push_int(-m);
}

/**
 * Funzione che esegue l'istruzione per l'operatore NEGA, che fa la negazione di un'espressione
 */
void execute_nega() {
	int m = pop_int();
	// Se m non e' zero (vero), carico sull'Instance stack il valore 0 (falso), altrimenti carico il valore 1 (vero)
	push_bool(m ? 0 : 1);
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

/**
 * Funzione che esegue la somma tra due interi (operatore ADDI)
 */
void execute_addi()
{
	// Tolgo i due interi dall'Instance Stack e ci inserisco la somma
	int n, m;
	n = pop_int();
	m = pop_int();
	push_int(m+n);
	print_ostack();
	print_istack();
}

/**
 * Funzione che esegue la sottrazione tra due interi (operatore SUBI)
 */
void execute_subi()
{
	// Tolgo i due interi dall'Instance Stack (prima il sottraendo, poi il minuendo) e ci inserisco la differenza
	int n, m;
	n = pop_int();
	m = pop_int();
	push_int(m-n);
}

/**
 * Funzione che esegue la moltiplicazione tra due interi (operatore MULI)
 */
void execute_muli()
{
	// Tolgo i due interi dall'Instance Stack e ci inserisco il prodotto
	int n, m;
	n = pop_int();
	m = pop_int();
	push_int(m*n);
}

/**
 * Funzione che esegue la divisione tra due interi (operatore DIVI)
 */
void execute_divi()
{
	// Tolgo i due interi dall'Instance Stack (prima il divisore e poi il dividendo) e ci inserisco il quoziente
	int n, m;
	n = pop_int();
	if(n==0) // Controllo che non venga eseguita una divisione per zero (errore a runtime)
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
	if(strcmp(s1,s2) >= 1) { // s1 > s2
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
	if(strcmp(s1,s2) <= -1) { // s1 < s2
		push_bool(0);
	}
	else { // s1 >= s2
		push_bool(1);
	}
}

/**
 * Confronta due stringhe ottenute recuperando il puntatore dall'Instance Stack e stabilisce se la prima è minore della
 * seconda. In caso affermativo viene inserito nell'Instance Stack il valore 1, altrimenti il valore 0.
 */
void execute_slet() {
	const char* s2 = pop_string();
	const char* s1 = pop_string();
	if(strcmp(s1,s2) <= -1) { // s1 < s2
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
	if(strcmp(s1,s2) >= 1) { // s1 > s2
		push_bool(0);
	}
	else { // s1 <= s2
		push_bool(1);
	}
}

/**
 * Funzione che implementa l'operatore ADEF per la definizione di tipi atomici
 */
void execute_adef(int size)
{
	// Creo l'Object Record da inserire nella prima posizione libera dell'Object stack
	Orecord *po;
	po = push_ostack();
	po->type = ATOM;
	po->size = size;
}

/**
 * Funzione che implementa l'operatore SDEF per la definizione di tipi strutturati
 */
void execute_sdef(int size)
{
	// Creo l'Object Record da inserire nella prima posizione libera dell'Object stack
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
	unsigned char *n, *m;
	int s1 = top_ostack()->size;
	// Copio in n s1 bytes presi dalla cima dell'Instance Stack
	memcpy(&n, pop_n_istack(s1), s1);
	pop_ostack();
	int s2 = top_ostack()->size;
	// Copio in m s2 bytes presi dalla cima dell'Instance Stack
	memcpy(&m, pop_n_istack(s2), s2);
	pop_ostack();
	// Sull'Instance stack verra' messo il risultato dell'operatore == applicato ai due puntatori
	push_bool(n==m);
}

/**
 * Confronta due oggetti (operatore !=): se questi sono diversi, carica nell'instance stack il valore 1, altrimenti carica
 * il valore 0
 */
void execute_nequ()
{
	unsigned char *n, *m;
	int s1 = top_ostack()->size;
	// Copio in n s1 bytes presi dalla cima dell'Instance Stack
	memcpy(&n, pop_n_istack(s1), s1);
	pop_ostack();
	int s2 = top_ostack()->size;
	// Copio in n s1 bytes presi dalla cima dell'Instance Stack
	memcpy(&m, pop_n_istack(s2), s2);
	pop_ostack();
	// Sull'Instance stack verra' messo il risultato dell'operatore == applicato ai due puntatori
	push_bool(n!=m);
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
