/*
 * redef.h
 *
 *  Created on: 09/dic/2016
 *      Author: fabio & cristian
 */

#ifndef REDEF_H_
#define REDEF_H_

#define NUMOPERANDS 3

typedef struct _acode{
	/*Operator operator;
	Lexval operands[NUMOPERANDS];*/
} Acode;

typedef enum {ATOM, VECTOR} Otype;
typedef struct _orecord{
	Otype type;
	int size;
	//Lexval instance;
} Orecord;

typedef struct _arecord {
	int objects;
	Orecord *head;
	int retad;
	struct _arecord *al;
} Arecord;



void start_abstract_machine();
void stop_abstract_machine();
void execute(Acode);

#endif /* REDEF_H_ */
