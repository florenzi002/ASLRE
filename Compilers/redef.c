/*
 * redef.c
 *
 *  Created on: 09/dic/2016
 *      Author: fabio & cristian
 */

const int ASEGMENT = 10;
const int OSEGMENT = 10;
const int ISEGMENT = 10;

Acode *program;
int pc;
Arecord **astack;
Orecord **ostack;
char *istack;
int asize, osize, isize;

int ap, op, ip;

long allocated = 0, deallocated = 0;

void start_abstract_machine()
{
	load_acode();
	pc = ap = op = ip = 0;
	astack = (Arecord**) newmem(sizeof(Arecord*) * ASEGMENT);
	asize = ASEGMENT;
	asize = ASEGMENT;
	ostack = (Orecord**) newmem(sizeof(Orecord*) * OSEGMENT);
	osize = OSEGMENT;
	istack = (char*) newmem(ISEGMENT);
	isize = ISEGMENT;
}
