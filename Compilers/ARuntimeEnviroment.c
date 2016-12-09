/*
 * ARuntimeEnviroment.c
 *
 *  Created on: 09/dic/2016
 *      Author: fabio & cristian
 */

main(int argc, char *argv[]) {
	Acode *instruction;
	start_abstract_machine();
	while ((instruction = &program[pc++])->operator != HALT)
		execute(instruction);
	stop_abstract_machine();
	printf("Hello World");
}
