/*
 * redef.h
 *
 * Created on: 09/dic/2016
 *      Author: Fabio Lorenzi & Cristian Sampietri
 */

#ifndef REDEF_H_
#define REDEF_H_

#define NUMOPERANDS 3
// Costanti a cui viene assegnata la dimensione di un intero e di un puntatore, relativamente alla macchina in uso
#define INTSIZE sizeof(int)
#define PTRSIZE sizeof(void *)
#define INTFORMAT "i"
#define STRFORMAT "s"
#define BOOLFORMAT "b"

// Enum contenente tutti i possibili operatori
typedef enum {
    ACODE,
    PUSH,
    JUMP,
    APOP,
    HALT,
    ADEF,
    SDEF,
    LOAD,
    PACK,
    LODA,
    IXAD,
    AIND,
    SIND,
    STOR,
    ISTO,
    SKIP,
    SKPF,
    EQUA,
    NEQU,
    IGRT,
    IGEQ,
    ILET,
    ILEQ,
    SGRT,
    SGEQ,
    SLET,
    SLEQ,
    ADDI,
    SUBI,
    MULI,
    DIVI,
    UMIN,
    NEGA,
    READ,
    WRIT,
    MODL,
    RETN,
    LOCS,
    LOCI,
    NOOP
} Operator;

enum {INTEGER, STRING};

typedef union{
    int ival;
    char *sval;
} Value;

/**
 * Struttura contenente una singola riga del codice generato
 */
typedef struct _acode{
	Operator operator;            // Contiene il codice corrispondente all'operazione da eseguire (LOCI, LOCS, ADEF, etc...)
	Value operands[NUMOPERANDS];  // Contiene gli operandi (da un minimo di 0 ad un massimo 3) che servono per eseguire l'istruzione.
} Acode;

typedef enum {ATOM, VECTOR} Otype; // Il tipo (atomico o strutturato), da inserire nell'apposito campo di un Obect Record

/**
 * Obect Record che rappresenta un singolo elemento dell'Obect Stack
 */
typedef struct _orecord{
	Otype type;     // Tipo (atomico o strutturato)
	int size;       // Dimensione in bytes (ad es. 4 per un intero)
	Value instance; // Il valore dell'oggetto (per i tipi atomici). Per i tipi strutturati questo campo punta alla prima cella dell'Instance stack associata all'oggetto
} Orecord;

/**
 * Activation Record che rappresenta un singolo elemento dell'Activation Stack
 */
typedef struct _arecord {
	int objects;          // Numero di oggetti dell'Obect Stack associati all'Activation Record
	Orecord **head;       // Puntatore al primo elemento dell'Object Stack relativo all'Activation Record
	int retad;            // Return Address per quell'Activation Record
	struct _arecord *al;  // Puntatore all'Activation Record che consente di risalire la catena statica
} Arecord;



void start_abstract_machine();
void stop_abstract_machine();
void execute(Acode *);

#endif /* REDEF_H_ */
