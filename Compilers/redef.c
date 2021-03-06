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
void execute_load(int, int);
void execute_loda(int, int);
void execute_aind(int);
void execute_sind(int);
void execute_ixad(int);
void execute_isto();
void execute_read(int, int, char*);
void execute_writ(char*);
void execute_adef(int);
void execute_sdef(int);
void execute_pack(int, int);
void execute_nega();
void execute_umin();
void execute_nequ();
void execute_equa();
void execute_apop();
unsigned char *pop_n_istack(int);
void pop_ostack();
int pop_int();
int pop_bool();
int endian();
void push_n_istack(unsigned char*, int);
void push_int(int);
Arecord *top_astack();
Orecord *top_ostack();
char *top_istack();
void print_ostack();
void print_astack();
void print_istack();
int endian();
void print_array(char, int*, int, int);
void abstract_machine_Error(char *);
void read_from_stream(Orecord*, Type);
int bytes_to_int(unsigned char*);

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
const int ASEGMENT = 128;
const int OSEGMENT = 1024;
const int ISEGMENT = 128;

const int LINEDIM = 128;

char *program_file;
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
	FILE *file = fopen(program_file, "r");
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
		program = newmem(code_size*sizeof(Acode));
		char* line;
		int p = 0;
		// Leggo il file riga per riga finche' non arrivo all'EOF
		while(!feof(file)){
			// Leggo una riga del file
			if(fgets(str, sizeof(str), file)==NULL)
				abstract_machine_Error("Error loading code");
			if(strlen(str)!=0) {
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
									if(counter>(strlen(line)-1)){
										if(realloc(string, counter)==NULL)
											abstract_machine_Error("Error allocating memory for given string");
									}
									if(c!='"'){
										string[pos++]=c;
									}
									char tmp = c;
									c=*(line+(++counter));
									if(c=='\0' && tmp!='"')
										c= ' ';
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
	}else{
		abstract_machine_Error("Error opening Acode file");
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
	print_ostack();
	printf("ObjectPointer: %d\n", op);

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
 * @return Puntatore all'Activation Record creato
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
 * @return Puntatore all'Activation Record in cima all'Activation Stack
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
 * @return Puntatore all'Object Record appena creato
 */
Orecord *push_ostack(){
	Orecord **full_ostack;
	int i;
	if(op == osize) // Verifico se l'Object Stack e' pieno
	{
		abstract_machine_Error("Failure allocating more space");
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
 * @return Puntatore all'Object Record in cima all'Activation Stack
 */
Orecord *top_ostack(){
	if (op==0) { // Errore: Object Stack vuoto
		abstract_machine_Error("Failure accessing top of Object Stack");
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
		istack = (char*) newmem(sizeof(char) * (isize + ISEGMENT));
		for(i = 0; i < isize; i++)
			istack[i] = full_istack[i];
		freemem((char*) full_istack, sizeof(char) * isize);
		isize += ISEGMENT;
	}
	// Ritorno l'indirizzo all'ultimo elemento
	return &(istack[ip++]);
}

/**
 * Funzione che recupera la cella dell'Instance Stack che si trova in cima allo stack (cioe' quella presente nella
 * posizione appena sopra a quella puntata da ip, che indica la prima posizione libera)
 *
 * @return l'indirizzo della cella in cima all'Activation Stack, cioe' quella sopra all'ip
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
 * Funzione che aggiunge un certo numero di bytes all'Instance stack a partire dalla prima posizione libera
 *
 * @param bytes --> I bytes da inserire
 * @param size --> Il numero di bytes da inserire
 */
void push_n_istack(unsigned char* bytes, int size) {
	int p;
	// A seconda dell'Endianess cambia l'ordine con cui i vari byte vengono memorizzati
	if (ENDIANESS) { // Big Endian (la memorizzazione inizia dal byte piu' significativo)
		p = 0;
		// Memorizzo byte per byte nell'istack
		for (; p < size; p++) {
			push_istack();
			*top_istack() = bytes[p];
		}
	} else { // Little Endian (la memorizzazione inizia dal byte meno significativo)
		p = size - 1;
		for (; p >= 0; p--) {
			push_istack();
			*top_istack() = bytes[p];
		}
	}
}

/**
 * Funzione che aggiunge un certo numero di bytes all'Instance stack a partire da una posizione specificata
 *
 * @param bytes --> I bytes da inserire
 * @param size --> Il numero di bytes da inserire
 * @param start_p --> La posizione iniziale da cui iniziare a scrivere i bytes
 */
void write_n_istack(unsigned char* bytes, int size, int start_p){
	int p, s=start_p;
	// A seconda dell'Endianess cambia l'ordine con cui i vari byte vengono memorizzati
	if (ENDIANESS) { // Big Endian (la memorizzazione inizia dal byte piu' significativo)
		p = 0;
		// Memorizzo byte per byte nell'istack a partire dalla posizione specificata da s
		for (; p < size; p++) {
			istack[s++] = *(bytes+p);
		}
	} else { // Little Endian (la memorizzazione inizia dal byte meno significativo)
		p = size - 1;
		for (; p >= 0; p--) {
			//printf("%d", bytes[p]);
			istack[s++] = bytes[p];
		}
	}
}

/**
 * Alloca memoria per un nuovo intero nell'istack e gli assegna un valore
 * @param i --> L'intero da mettere nell'Instance Stack
 */
void push_int(int i){
	// Chiamo ADEF per creare l'oggetto nell'Object Stack, passandogli come parametro la dimensione di un intero
	execute_adef(INTSIZE);
	top_ostack()->instance.ival=i;
}

/**
 * Funzione che memorizza nell'Instance stack il puntatore alla stringa
 *
 * @param s --> Il puntatore alla stringa che deve essere messo nell'Istack
 */
void push_string(char* s){
	// Chiamo l'ADEF per creare l'oggetto nell'Object Stack, passandogli come parametro la dimensione in bytes di un puntatore
	execute_adef(PTRSIZE);
	top_ostack()->instance.sval=s;
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
 * Recupera dall'Instance stack n bytes a partire da una certa posizione start
 *
 * @param n --> Numero di celle da recuperare dall'Instance Stack
 * @param start --> Posizione iniziale da cui recuperare gli elementi
 */
unsigned char *load_n_istack(int n, int start){
	unsigned char *i_bytes = malloc(n*sizeof(char));
	int p = 0;
	// Parto dalla posizione specificata dal parametro start e risalgo nell'Instance Stack del numero di posizioni necessario per recuperare la prima
	char *q = &istack[start];
	for(;p<n;p++){
		// A ciascun byte di i_bytes assegno il valore di ciò che punta q
		i_bytes[p]=*(q++);
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
 * Funzione che recupera il valore dell'oggetto in cima all'Object Stack
 *
 * @return --> L'intero recuperato dal campo ival di instance
 */
int pop_int(){
	int res = top_ostack()->instance.ival;
	pop_ostack();
	return res;
}

/**
 * Funzione che recupera una stringa dal campo sval dell'oggetto in cima all'Object Stack
 *
 * @return --> Puntatore alla stringa recuperata
 */
char* pop_string(){
	char* res = top_ostack()->instance.sval;
	pop_ostack();
	return res;
}

/**
 * Richiama la funzione pop_int (la macchina virtuale considera un boolano come un intero avente valore 0 o 1)
 *
 * @return --> L'intero recuperato dal campo ival dell'oggetto in cima all'Object Stack
 */
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
	//printf("istr: %d|%d - %s\n", pc+1,instruction->operator, s_op_code[instruction->operator]);
	// A seconda dell'operatore, chiamo la funzione che esegue la corrispondente istruzione
	switch(instruction->operator)
	{
	case PUSH: execute_push(instruction->operands[0].ival, instruction->operands[1].ival, instruction->operands[2].ival); break;
	case JUMP: execute_jump(instruction->operands[0].ival); break;
	case APOP: execute_apop(); break;
	case ADEF: execute_adef(instruction->operands[0].ival); break;
	case SDEF: execute_sdef(instruction->operands[0].ival); break;
	case LOCI: execute_loci(instruction->operands[0].ival); break;
	case LOCS: execute_locs(instruction->operands[0].sval); break;
	case LOAD: execute_load(instruction->operands[0].ival, instruction->operands[1].ival); break;
	case PACK: execute_pack(instruction->operands[0].ival, instruction->operands[1].ival); break;
	case LODA: execute_loda(instruction->operands[0].ival, instruction->operands[1].ival); break;
	case IXAD: execute_ixad(instruction->operands[0].ival); break;
	case AIND: execute_aind(instruction->operands[0].ival); break;
	case SIND: execute_sind(instruction->operands[0].ival); break;
	case STOR: execute_store(instruction->operands[0].ival, instruction->operands[1].ival); break;
	case IGRT: execute_igrt(); break;
	case IGEQ: execute_igeq(); break;
	case ILET: execute_ilet(); break;
	case ILEQ: execute_ileq(); break;
	case SGRT: execute_sgrt(); break;
	case SGEQ: execute_sgeq(); break;
	case SLET: execute_slet(); break;
	case SLEQ: execute_sleq(); break;
	case ISTO: execute_isto(); break;
	case SKIP: execute_skip(instruction->operands[0].ival); break;
	case SKPF: execute_skpf(instruction->operands[0].ival); break;
	case EQUA: execute_equa(); break;
	case NEQU: execute_nequ(); break;
	case ADDI: execute_addi(); break;
	case SUBI: execute_subi(); break;
	case MULI: execute_muli(); break;
	case DIVI: execute_divi(); break;
	case UMIN: execute_umin(); break;
	case NEGA: execute_nega(); break;
	case READ: execute_read(instruction->operands[0].ival, instruction->operands[1].ival, instruction->operands[2].sval); break;
	case WRIT: execute_writ(instruction->operands[0].sval); break;
	case MODL: break;
	case NOOP: break;
	case RETN: execute_retn(); break;
	default: abstract_machine_Error("Unknown operator"); break;
	}
	//print_ostack();
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

	if(p_obj->type==ATOM)
		// Per oggetti atomici, si copia l'indirizzo del campo instance dell'oggetto in cima allo stack nell'indirizzo del campo instance di p_obj
		memcpy(&(p_obj->instance),&(top_ostack()->instance), size);
	else{
		// Recupero dall'istack un numero di bytes specificato dal campo size dell'Orecord
		unsigned char *bytes = pop_n_istack(size);
		write_n_istack(bytes,size,p_obj->instance.ival);
	}
	// Rimuovo l'Object Record dalla cima dell'Object Stack
	pop_ostack();
}

/**
 * Funzione che esegue l'istruzione LOAD per referenziare un identificatore
 *
 * @param chain --> Indica la distanza, cioe' di quanti Activation Record dobbiamo risalire per arrivare all'ambiente in
 *                  cui l'identificatore e' definito (se e' 0, allora l'oggetto e' locale)
 * @param oid --> Identificatore dell'oggetto (al fine di recuperarlo dall'Object Stack)
 */
void execute_load(int chain, int oid){
	Arecord* target_ar = top_astack(); // Variabile che dovra' contenere l'Activation Record corrispondente all'ambiente in cui l'oggetto e' definito
	int i=0;
	// Tramite l'access link, risalgo di un numero di Activation Record pari a quello specificato nel parametro chain
	for(; i<chain; i++) {
		target_ar = target_ar->al;
	}
	// Si ottiene un riferimento all'oggetto relativo all'identificatore assegnato, sfruttando il campo head dell'Activation Record e l'oid
	Orecord* p_obj = *(target_ar->head + oid);
	if(p_obj->size == ADDR){ // Caricamento di un indirizzo
		// Creazione nuovo Object Record
		push_ostack();
		// Viene copiato p_obj nel puntatore al nuovo oggetto
		memcpy(top_ostack(),p_obj,sizeof(Orecord));
	}else{ // Caricamento di un oggetto atomico
		if(p_obj->type==ATOM){
			// Viene creato un nuovo oggetto di tipo atomico
			execute_adef(p_obj->size);
			// Viene copiato l'indirizzo del campo instance di p_obj nell'indirizzo del campo instance del nuovo oggetto
			memcpy(&(top_ostack()->instance), &(p_obj->instance), sizeof(Value));
		}else{ // Caricamento di un oggetto di tipo VECTOR
			execute_sdef(p_obj->size);
			// Recupero dall'Instance stack i valori degli elementi atomici di p_obj e li associo al nuovo oggetto.
			unsigned char *bytes = load_n_istack(p_obj->size, p_obj->instance.ival);
			write_n_istack(bytes,p_obj->size,top_ostack()->instance.ival);
		}
	}
}

/**
 * Funzione che esegue l'istruzione LODA per effettuare il caricamento di un indirizzo
 *
 * @param chain --> Indica la distanza, cioe' di quanti Activation Record dobbiamo risalire per arrivare all'ambiente in
 *                  cui l'array e' definito (se e' 0, allora l'oggetto e' locale)
 *
 * @param oid --> Identificatore dell'oggetto (al fine di recuperarlo dall'Object Stack)
 */
void execute_loda(int chain, int oid){
	Arecord* target_ar = top_astack();
	int i=0;
	// Tramite l'access link, risalgo di un numero di Activation Record pari a quello specificato nel parametro chain
	for(; i<chain; i++) {
		target_ar = target_ar->al;
	}
	// Recupero dall'Object Stack l'oggetto relativo all'identificatore assegnato, sfruttando il campo head dell'Activation Record e l'oid dell'oggetto
	Orecord* p_obj = *(target_ar->head + oid);
	push_ostack();
	// Viene copiato p_obj nel nuovo oggetto creato
	memcpy(top_ostack(),p_obj,sizeof(Orecord));
	if(p_obj->type==ATOM){ // Per oggetti atomici, copio p_obj nel campo sval
		top_ostack()->instance.sval = (char*)p_obj;
	}
	top_ostack()->size = ADDR;
}

/**
 * Funzione che esegue l'istruzione PUSH per creare l'Activation Record all'atto della chiamata di funzione
 *
 * @param num_formals_aux --> Numero di parametri formali + ausiliari
 * @param num_loc --> Numero di costanti locali
 * @param chain --> Distanza tra l'ambiente del chiamante e l'ambiente in cui la funzione e' definita
 */
void execute_push(int num_formals_aux, int num_loc, int chain){
	Arecord *dyn_ar = NULL;
	if(ap>0)
		dyn_ar = top_astack();
	// Chiamo la funzione per allocare un nuovo Activation Record e aggiungerlo all'Activation Stack
	Arecord *ar = push_activation_record();
	// In base al valore della catena, viene settato opportunamente l'access link
	ar->al=dyn_ar;
	while((chain--)>0){
		ar->al = (ar->al)->al;
	}
	// Il campo head del nuovo Activation Record punta al primo degli oggetti ad esso associati
	ar->head = &ostack[op-num_formals_aux];
	// Il numero di oggetti associati all'Activation Record e' dato dal numero dei locali + il numero dei parametri formali e ausiliari
	ar->objects = num_loc+num_formals_aux;
	// Il return address viene settato come program-counter+1
	ar->retad = pc+1;
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
 * @param const_val --> L'intero da mettere nel campo ival del relativo oggetto
 */
void execute_loci(int const_val){
	push_int(const_val);
}

/**
 * Funzione che esegue l'istruzione per l'operatore LOCS per le costanti stringa
 *
 * @param const_val --> Stringa da aggiungere nel campo sval del relativo oggetto
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
	// Se m non e' zero (cioe' se l'espressione e' vera), assegno al relativo oggetto il valore 0 (falso), altrimenti assegno il valore 1 (vero)
	push_bool(m ? 0 : 1);
}

/**
 * Funzione che esegue l'istruzione SKIP per eseguire l'unconditional relative jump
 *
 * @param offset --> Specifica di quante istruzioni bisogna saltare
 */
void execute_skip(int offset)
{
	pc += offset-1;
}

/**
 * Funzione che esegue l'istruzione SKPF per eseguire il conditional relative jump
 *
 * @param offset --> Specifica di quante istruzioni bisogna saltare
 */
void execute_skpf(int offset)
{
	if(!pop_bool())
		pc += offset-1;
}

/**
 * Funzione che esegue l'istruzione RETN per eseguire il return al termine di una funzione o procedura
 */
void execute_retn()
{
	// Setto il Program Counter con il Return Address dell'Activation Record in cima allo stack
	pc = top_astack()->retad;
}

/**
 * Funzione che esegue la somma tra due interi (operatore ADDI)
 */
void execute_addi()
{
	// Vengono recuperati i due addendi dagli oggetti in cima all'Ostack, che vengono dunque consumati
	int n, m;
	n = pop_int();
	m = pop_int();
	// Creazione di un nuovo oggetto nell'Object stack, il cui valore e' la somma dei due addendi
	push_int(m+n);
}

/**
 * Funzione che esegue la sottrazione tra due interi (operatore SUBI)
 */
void execute_subi()
{
	// Vengono recuperati il sottraendo e il minuendo dagli oggetti in cima all'Ostack, che vengono dunque consumati
	int n, m;
	n = pop_int();
	m = pop_int();
	// Creazione di un nuovo oggetto nell'Object stack, il cui valore e' la differenza tra minuendo e sottraendo
	push_int(m-n);
}

/**
 * Funzione che esegue la moltiplicazione tra due interi (operatore MULI)
 */
void execute_muli()
{
	// Vengono recuperati i due fattori dagli oggetti in cima all'Ostack, che vengono dunque consumati
	int n, m;
	n = pop_int();
	m = pop_int();
	// Creazione di un nuovo oggetto nell'Object stack, il cui valore e' il prodotto dei due
	push_int(m*n);
}

/**
 * Funzione che esegue la divisione tra due interi (operatore DIVI)
 */
void execute_divi()
{
	// Vengono recuperati prima il divisore e poi il dividendo dagli oggetti in cima all'Ostack, che vengono dunque consumati
	int n, m;
	n = pop_int();
	if(n==0) // Controllo che non venga eseguita una divisione per zero (errore a runtime)
		abstract_machine_Error("Error: Divide by 0");
	m = pop_int();
	// Creazione di un nuovo oggetto nell'Object Stack, il cui valore e' il quoziente
	push_int(m/n);
}

/**
 * Confronta due interi e stabilisce se il primo e' maggiore del secondo.
 * In caso affermativo viene assegnato all'oggetto in cima all'Object stack il valore 1 (corrispondente a true), altrimenti il valore 0
 */
void execute_igrt()
{
	int n, m;
	n = pop_int();
	m = pop_int();
	push_bool(m>n);
}

/**
 * Confronta due interi e stabilisce se il primo e' maggiore o uguale al secondo.
 * In caso affermativo viene assegnato all'oggetto in cima all'Object stack il valore 1 (corrispondente a true), altrimenti il valore 0
 */
void execute_igeq()
{
	int n, m;
	n = pop_int();
	m = pop_int();
	push_bool(m>=n);
}

/**
 * Confronta due interi e stabilisce se il primo e' minore del secondo.
 * In caso affermativo viene assegnato all'oggetto in cima all'Object stack il valore 1 (corrispondente a true), altrimenti il valore 0
 */
void execute_ilet()
{
	int n, m;
	n = pop_int();
	m = pop_int();
	push_bool(m<n);
}

/**
 * Confronta due interi e stabilisce se il primo e' minore o uguale al secondo.
 * In caso affermativo viene assegnato all'oggetto in cima all'Object stack il valore 1 (corrsipondente a true), altrimenti il valore 0
 */
void execute_ileq() {
	int n, m;
	n = pop_int();
	m = pop_int();
	push_bool(m<=n);
}

/**
 * Confronta due stringhe e stabilisce se la prima è maggiore della
 * seconda. In caso affermativo viene assegnato all'oggetto in cima all'Object stack il valore 1 (corrsipondente a true), altrimenti il valore 0.
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
 * Confronta due stringhe e stabilisce se la prima è maggiore o
 * uguale alla seconda. In caso affermativo viene assegnato all'oggetto in cima all'Object Stack il valore 1 (corrispondente a true), altrimenti il valore 0.
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
 * Confronta due stringhe e stabilisce se la prima è minore della
 * seconda. In caso affermativo viene assegnato all'oggetto in cima all'Object Stack il valore 1 (corrispondente a true), altrimenti il valore 0.
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
 * Confronta due stringhe e stabilisce se la prima è minore della
 * seconda. In caso affermativo viene assegnato all'oggetto in cima all'Object Stack il valore 1 (corrispondente a true), altrimenti il valore 0.
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
 *
 * @param size --> La dimensione del tipo atomico
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
 *
 * @param size --> La dimensione del tipo strutturato
 */
void execute_sdef(int size)
{
	// Creo l'Object Record da inserire nella prima posizione libera dell'Object stack
	Orecord *po;
	po = push_ostack();
	po->type = VECTOR;
	po->size = size;
	po->instance.ival=ip;
	int i = 0;
	// Viene assegnato nell'Instance Stack lo spazio necessario per contenere gli elementi atomici del vettore
	for(;i<size;i++)
		push_istack();
}

/**
 * Funzione che implementa l'operatore PACK per aggregare tutti gli elementi di un array in un unico oggetto array
 *
 * @param n_elem --> Il numero di elementi dell'array
 * @param sizeof_elem --> La dimensione del singolo elemento dell'array
 */
void execute_pack(int n_elem, int sizeof_elem){
	int i=0, bytes = n_elem*sizeof_elem;
	// Vengono recuperati i valori degli oggetti atomici, i quali vengono rimossi dall'Object Stack
	unsigned char* byte_arr = malloc(bytes);
	for(;i<n_elem;i++){
		memcpy((byte_arr+(i*sizeof_elem)),&(top_ostack()->instance), sizeof_elem);
		pop_ostack();
	}
	// I valori degli elementi atomici dell'array vengono inseriti nell'Instance Stack
	push_n_istack(byte_arr, bytes);
	free(byte_arr);
	// Creazione del nuovo oggetto array
	Orecord *po;
	po = push_ostack();
	po->type = VECTOR;
	po->size = bytes;
	// Il campo instance del nuovo oggetto array punta alla prima cella dell'Instance stack contenente i valori degli elementi atomici dell'array
	po->instance.ival=ip-bytes;
}

/**
 * Funzione che implementa l'operatore IXAD per il caricamento dell'indirizzo di un elemento dell'array (indexed address)
 *
 * @param sizeof_elem --> La dimensione dell'elemento
 */
void execute_ixad(int sizeof_elem){
	int position = pop_int(); // Posizione all'interno dell'array dell'elemento da caricare
	int start_addr = pop_int(); // Indirizzo di partenza dell'array
	int dest_addr = start_addr + sizeof_elem*position; // Calcola la posizione dell'elemento da caricare
	push_int(dest_addr);   // Aggiunge la posizione calcolata nello stack
	top_ostack()->type=VECTOR;
}

/**
 * Funzione che implementa l'operatore AIND per effettuare l'indirect load di un oggetto di tipo atomico
 *
 * @param sizeof_elem --> La dimensione dell'elemento
 */
void execute_aind(int sizeof_elem){
	// Recupero dall'Instance stack i bytes necessari e li copio nell'indirizzo del campo instance dell'oggetto che rappresenta l'elemento dell'array
	int start_addr = pop_int();
	unsigned char* bytes = load_n_istack(sizeof_elem, start_addr);
	execute_adef(sizeof_elem);
	memcpy(&(top_ostack()->instance), bytes, sizeof_elem);
	free(bytes);
}

/**
 * Funzione che implementa l'operatore SIND per effettuare l'indirect load di un oggetto di tipo strutturato
 *
 * @param sizeof_elem --> La dimensione dell'elemento
 */
void execute_sind(int sizeof_elem){
	// Recupero dall'Instance stack i bytes necessari
	int start_addr = pop_int();
	unsigned char* bytes = load_n_istack(sizeof_elem, start_addr);
	int start = ip;
	execute_sdef(sizeof_elem);
	write_n_istack(bytes, sizeof_elem, start);
	free(bytes);
}

/**
 * Funzione che implementa l'operatore ISTO per effettuare l'indirect store
 */
void execute_isto(){
	// Recupero dall'Object Stack gli oggetti contenenti l'indirizzo dell'oggetto da assegnare e l'oggetto da assegnare.
	Orecord *obj = malloc(sizeof(Orecord));
	*obj=*top_ostack();
	pop_ostack();
	Orecord* addr_descr = top_ostack();
	unsigned char* bytes = malloc(sizeof(unsigned char)*(obj->size));
	if(addr_descr->type==ATOM){ // Indirect store per un oggetto di tipo atomico
		// Ottengo il puntatore alla memoria della destinazione
		Orecord *dest = (Orecord*)addr_descr->instance.sval;
		// Copio il campo instance dell'oggetto da assegnare nel campo instance dell'oggetto puntato dall'indirizzo
		memcpy(&(dest->instance), &(obj->instance), obj->size);
	}
	else{ // Indirect store per un oggetto di tipo strutturato
		// Per oggetti di tipo atomico recupero il valore da assegnare dal campo instance
		if(obj->type==ATOM){
			memcpy(bytes, &(obj->instance), obj->size);
			write_n_istack(bytes, obj->size, addr_descr->instance.ival);
		}else{ // Per oggetti strutturati recupero il valore da assegnare dall'Instance stack
			bytes = pop_n_istack(obj->size);
			write_n_istack(bytes, obj->size,addr_descr->instance.ival);
		}
	}
	// Rimuovo dall'Object stack l'indirizzo dell'oggetto da assegnare
	pop_ostack();
	free(obj);
	free(bytes);
}

/**
 * Funzione che implementa l'operatore READ per lo statement di input
 *
 * @param chain --> Indica la distanza nella catena statica, cioe' di quanti Activation record bisogna risalire per
 *                  trovare l'ambiente in cui la l'oggetto da assegnare e' definito
 * @param oid --> Identificatore dell'oggetto (necessario per recuperare l'oggetto da assegnare tra quelli relativi
 *                all' Activation Record
 *
 * @param format --> Stringa che specifica il formato secondo cui deve essere letto l'input
 */
void execute_read(int chain, int oid, char* format) {
	Type type = -999;
	char* format_cpy = malloc(strlen(format)+1);
	strcpy(format_cpy, format);
	if((strcmp(format,INTFORMAT)==0) || (strcmp(format,BOOLFORMAT)==0)) { 	// Lettura di un intero o di un booleano
		type = T_INT;
	}
	else if(strcmp(format, STRFORMAT)==0){ // Lettura di una stringa
		type = T_STRING;
	}
	else { // Lettura di un array
		char* tk_str = strtok(format_cpy, ",");
		while(tk_str != NULL){
			if(tk_str[0]!='['){
				// Viene tokenizzata la stringa finche' non si arriva al tipo degli elementi atomici
				if(tk_str[0]==INTFORMAT_C)
					type=T_ARR_INT;
				else
					type=T_ARR_STR;
			}
			tk_str = strtok(NULL, ",");
		}
	}

	Arecord* target_ar = top_astack();
	int i=0;
	// Tramite l'access link, risalgo di un numero di Activation Record pari a quello specificato nel parametro chain
	for(; i<chain; i++) {
		target_ar = target_ar->al;
	}
	// Recupero dall'Object Stack l'oggetto relativo all'identificatore assegnato, sfruttando il campo head dell'Activation Record e l'oid dell'oggetto
	Orecord * obj_to_load  = *(target_ar->head + oid);
	// Viene effettuata la lettura dallo standard input
	read_from_stream(obj_to_load, type);
}

/**
 * Funzione che implementa l'operatore WRIT per l'output statement (stampa a video di un oggetto)
 *
 * @param format --> Stringa che specifica il formato dell'oggetto da stampare
 */
void execute_writ(char* format) {
	char* format_cpy = malloc(strlen(format)+1);
	strcpy(format_cpy, format);
	// A seconda del formato dell'oggetto da stampare, viene chiamata la funzione printf con lo specificatore di formato appropriato
	if((strcmp(format,INTFORMAT)==0)) { // Stampa di un intero
		printf("%d\n", pop_int());
	}
	else if((strcmp(format,BOOLFORMAT)==0)) { // Stampo false o true a seconda del valore che recupero
		printf("%s\n", pop_int()==0?"FALSE":"TRUE");
	}
	else if(strcmp(format, STRFORMAT)==0){ // Stampa di una
		// Stampo una stringa
		printf("%s\n", pop_string());
	}
	else { // Stampa di un array
		char* tk_str = strtok(format_cpy, ",");
		int *dims=malloc(sizeof(int)), array_level = 0, tot_elem = 0;
		char type = 'e';
		while(tk_str != NULL){
			// Tokenizzo la stringa del formato per recuperare le singole dimensioni, calcolare il numero degli elementi e il tipo
			if(tk_str[0]!='[')
				type = tk_str[0];
			else{
				if(array_level==0)
					tot_elem+=atoi(tk_str+1);
				else
					tot_elem*=atoi(tk_str+1);
				dims = realloc(dims,(++array_level)*sizeof(int));
				dims[array_level-1] = atoi(tk_str+1);
			}
			tk_str = strtok(NULL, ",");
		}
		print_array(type,dims,tot_elem, array_level);
	}
}

/**
 * Confronta due oggetti (operatore ==): se questi sono uguali, carica valore 1 (corrispondente a true), altrimenti carica
 * il valore 0 (corrispondente a false)
 */
void execute_equa()
{
	Orecord obj1, obj2;
	int s1 = top_ostack()->size;
	obj1 = *top_ostack();
	pop_ostack();
	obj2 = *top_ostack();
	pop_ostack();
	push_bool(memcmp(&obj1.instance, &obj2.instance, s1)==0);
}

/**
 * Confronta due oggetti (operatore !=): se questi sono diversi, carica nell'instance stack il valore 1 (corrispondente a true), altrimenti
 * carica il valore 0 (corrispondente a false)
 */
void execute_nequ()
{
	Orecord *obj1, *obj2;
	int s1 = top_ostack()->size;
	obj1 = top_ostack();
	pop_ostack();
	obj2 = top_ostack();
	pop_ostack();
	push_bool(memcmp(&obj1->instance, &obj2->instance, s1)!=0);
}

/**
 * Funzione che implementa l'operatore APOP, necessario per cancellare l'Activation Record in cima all'astack e tutti gli oggetti
 * ad esso associati
 */
void execute_apop(){
	// Vengono eliminati tutti gli oggetti associati all'Activation Record, a partire da quello puntato dal campo
	int i=0;
	Orecord **start = top_astack()->head;
	Orecord ** position = start;
	for(;i<top_astack()->objects; i++){
		if((*position)->size!=ADDR && (*position)->type==VECTOR)
			ip-=(*position)->size;
		freemem((char*)*position, sizeof(Orecord));
		position++;
	}
	// Gli eventuali oggetti sottostanti vengono spostati per riempire le posizioni dell'astack rimaste vuote dopo la cancellazione degli oggetti
	while(position!=&ostack[op]){
		*start = *position;
		start++;
		position++;
	}
	op-=top_astack()->objects; // L'Object Pointer viene opportunamente decrementato
	pop_activation_record();
}

/**
 * Funzione ausiliaria che stampa l'Object Stack
 */
void print_ostack(){
	int i;
	if(op!=0)
		printf("# objects on ostack: %d\n", op);
	printf("---------------------------------------\n");
	for(i=0; i<op; i++){
		printf("type of object: %s\n", (ostack[i]->type==0? "ATOM" : "VECTOR"));
		printf("size of object: %d\n", ostack[i]->size);
		printf("value of object: %d\n", ostack[i]->instance.ival);
		printf("---------------------------------------\n");
	}
}

/**
 * Funzione ausiliaria che stampa l'Activation Stack
 */
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

/**
 * Funzione ausiliaria che stampa l'Instance Stack
 */
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

/**
 * Funzione che esegue la lettura dallo standard input e assegna all'oggetto passato come primo parametro
 *
 * @param obj_to_load --> L'oggetto a cui assegnare cio' che l'utente fornisce in input
 * @param type --> Il tipo di oggetto da assegnare
 */
void read_from_stream(Orecord* obj_to_load, Type type) {
	char * str = malloc(PTRSIZE);
	int elems_to_read, letto, i=0, posizione, scanf_return;
	switch(type) {
	case T_INT:  // Lettura di un intero dallo standard input. Tale valore viene inserito nel campo ival dell'oggetto
		scanf_return = fscanf(stdin, "%d", &(obj_to_load->instance.ival));
		if(scanf_return == 0 || scanf_return == EOF)
			abstract_machine_Error("Error reading from stdin");
		break;
	case T_STRING: // Lettura di una stringa dallo standard input. Tale valore e' salvato nel campo sval dell'oggetto
		scanf_return = fscanf(stdin, "%s", str);
		if(scanf_return == 0 || scanf_return == EOF)
			abstract_machine_Error("Error reading from stdin");
		obj_to_load->instance.sval = insertFind(hash(str), str); // Per le stringhe si usa una hash table per evitare spreco di memoria dovuto a duplicati
		break;
	case T_ARR_INT: // Lettura di un array di interi
		elems_to_read = (obj_to_load->size)/INTSIZE; // Calcolo del numero elementi da leggere
		// Ciascun intero letto dallo standard input viene aggiunto all'Instance Stack nella posizione specificata
		posizione = obj_to_load->instance.ival;
		for(; i<elems_to_read; i++) {
			scanf_return = fscanf(stdin, "%d", &letto);
			if(scanf_return == 0 || scanf_return == EOF)
				abstract_machine_Error("Error reading from stdin");
			write_n_istack((unsigned char *)&letto, INTSIZE, posizione);
			posizione += INTSIZE;
		}
		break;
	case T_ARR_STR: // Lettura di un array di stringhe
		elems_to_read = (obj_to_load->size)/PTRSIZE; // Calcolo del numero elementi da leggere
		// Per ciascuna stringa letta dallo standard input viene aggiunto nell'Instance Stack il puntatore
		int posizione = obj_to_load->instance.ival;
		for(; i<elems_to_read; i++) {
			scanf_return = fscanf(stdin, "%s", str);
			if(scanf_return == 0 || scanf_return == EOF)
				abstract_machine_Error("Error reading from stdin");
			write_n_istack((unsigned char*)str, PTRSIZE, posizione);
			posizione += PTRSIZE;
		}
		break;
	default:
		scanf_return = -999;
		break;
	}
}

/**
 * Funzione che esegue la stampa di un array
 * @param type --> Il tipo degli elementi atomici
 * @param dims --> Le singole dimensioni
 * @param tot_elem --> Numero complessivo di elementi dell'array
 * @param array_levels --> Numero di livelli
 */
void print_array(char type, int* dims, int tot_elem, int array_levels) {
	int i=0, q=0, p_to_print, offset[array_levels-1];
	offset[array_levels-2] = dims[array_levels-1];
	for(q=array_levels-3; q>=0;q--){
		offset[q]=offset[q+1]*dims[q+1];
	}

	if(type==INTFORMAT_C){
		int val[tot_elem];
		for (; i < tot_elem; i++) {
			val[tot_elem-i-1] = bytes_to_int(pop_n_istack(INTSIZE));
		}
		printf("[");
		for(i = 0; i < tot_elem; i++){
			p_to_print = 0;
			for(q = 0; q<array_levels-1; q++){
				if(i%offset[q]==0)
					p_to_print++;
			}
			if(i!=0){
				for(q = 0; q < p_to_print; q++){
					printf("%s","]");
				}
				printf("%s",",");
			}
			for(q = 0; q < p_to_print; q++){
				printf("%s","[");
			}
			printf("%d ", val[i]);
		}
		for(q=0;q<array_levels;q++)
			printf("]");
		printf("\n");
	}else if(type==STRFORMAT_C){
		char* val[tot_elem];
		for (; i < tot_elem; i++) {
			memcpy(&val[tot_elem-i-1], pop_n_istack(PTRSIZE), PTRSIZE);
		}
		printf("[");
		for(i = 0; i < tot_elem; i++){
			p_to_print = 0;
			for(q = 0; q<array_levels-1; q++){
				if(i%offset[q]==0)
					p_to_print++;
			}
			if(i!=0){
				for(q = 0; q < p_to_print; q++){
					printf("%s","]");
				}
				printf("%s",",");
			}
			for(q = 0; q < p_to_print; q++){
				printf("%s","[");
			}
			printf("%s ", val[i]);
		}
		for(q=0;q<array_levels;q++)
			printf("]");
		printf("\n");
	}
	pop_ostack();
}

/**
 * Funzione ausiliaria che converte un array di bytes in un intero
 *
 * @param bytes --> L'array di bytes da convertire
 */
int bytes_to_int(unsigned char* bytes) {
	int i;
	int res = bytes[0];
	for(i=1; i<INTSIZE; i++) {
		res += (bytes[i] << (8*i));
	}
	return res;
}
