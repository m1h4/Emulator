#include <stdio.h>

#include "Emulator.h"

int main(int argc,const char** argv)
{
	EMULATOR emulator;

	--argc;
	++argv;

	if(!argc)
	{
		printf("No input file specified.\n");
		return 1;
	}

	if(!InitializeEmulator(&emulator, EMULATOR_DEFAULT_MEMORY))
	{
		printf("Failed to initialize the emulation engine.\n");
		return 1;
	}

	if(!LoadProgramFromSourceFile(&emulator, argv[0]))
	{
		printf("Failed to load the input file '%s'. Error %0#8x.\n", argv[0], emulator.error);

		UninitializeEmulator(&emulator);
		return 1;
	}

	while(ExecuteInstruction(&emulator));

	if(emulator.exception != EMULATOR_EXCEPTION_NONE)
		printf("Exception %0#8x occured at address %0#8x. Program terminated.\n", emulator.exception, emulator.registers[EMULATOR_REGISTER_PROGRAM_COUNTER]);

	UninitializeEmulator(&emulator);
	return 0;
}