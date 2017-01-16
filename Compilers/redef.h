/*
 * redef.h
 *
 *  Created on: 09/dic/2016
 *      Author: fabio & cristian
 */

#ifndef REDEF_H_
#define REDEF_H_

#define NUMOPERANDS 3
#define INTSIZE sizeof(int)
#define ENDIANESS endian()
#define PTRSIZE sizeof(void *)


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
  //enum {TRUE, FALSE} bval;

} Value;

typedef struct _acode{
	Operator operator;
	Value operands[NUMOPERANDS];
} Acode;

typedef enum {ATOM, VECTOR} Otype;

typedef struct _orecord{
	Otype type;
	int size;
	Value instance;
} Orecord;

typedef struct _arecord {
	int objects;
	Orecord **head;
	int retad;
	struct _arecord *al;
} Arecord;



void start_abstract_machine();
void stop_abstract_machine();
void execute(Acode *);

#endif /* REDEF_H_ */
