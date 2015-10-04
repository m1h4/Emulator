#include "Emulator.h"

#include <stdio.h>

COMMAND commands[] =
{
	{"JUMP",	INSTRUCTION_JUMP,	ParseInstructionJump,		ExecuteInstructionJump},
	{"COND",	INSTRUCTION_COND,	ParseInstructionCond,		ExecuteInstructionCond},
	{"MOVE",	INSTRUCTION_MOVE,	ParseInstructionMove,		ExecuteInstructionMove},
	{"ADD",		INSTRUCTION_ADD,	ParseInstructionAdd,		ExecuteInstructionAdd},
	{"SUB",		INSTRUCTION_SUB,	ParseInstructionSub,		ExecuteInstructionSub},
	{"WRITE",	INSTRUCTION_WRITE,	ParseInstructionWrite,		ExecuteInstructionWrite},
	{"READ",	INSTRUCTION_READ,	ParseInstructionRead,		ExecuteInstructionRead},
	{"LOAD",	INSTRUCTION_LOAD,	ParseInstructionLoad,		ExecuteInstructionLoad},
	{"STORE",	INSTRUCTION_STORE,	ParseInstructionStore,		ExecuteInstructionStore},
	{"PUSH",	INSTRUCTION_PUSH,	ParseInstructionPush,		ExecuteInstructionPush},
	{"POP",		INSTRUCTION_POP,	ParseInstructionPop,		ExecuteInstructionPop},
	{"BREAK",	INSTRUCTION_BREAK,	ParseInstructionBreak,		ExecuteInstructionBreak},
	// TODO Add more commands here

	{"DW",		INSTRUCTION_NONE,	ParseDirectiveDefineWord,	NULL},
	{"DS",		INSTRUCTION_NONE,	ParseDirectiveDefineString,	NULL},
	// TODO Add more directives here
};

BOOL InitializeEmulator(LPEMULATOR emulator, ULONG memory)
{
	if(!memory)
		memory = EMULATOR_DEFAULT_MEMORY;

	ZeroMemory(emulator,sizeof(EMULATOR));

	emulator->memory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, memory);
	if(!emulator->memory)
		return FALSE;

	emulator->capacity = memory;

	return TRUE;
}

VOID UninitializeEmulator(LPEMULATOR emulator)
{
	ULONG index;

	if(!emulator->memory)
		return;

	// Free the allocated instructions
	for(index = 0; index < emulator->capacity && emulator->memory[index]; ++index)
		HeapFree(GetProcessHeap(), 0, (LPVOID)emulator->memory[index]);

	HeapFree(GetProcessHeap(), 0, emulator->memory);

	emulator->memory = NULL;
	emulator->capacity = 0;
}

BOOL LoadProgramFromSourceFile(LPEMULATOR emulator, LPCSTR path)
{
	BYTE buffer[EMULATOR_READ_BUFFER];

	FILE* file = fopen(path, "rt");
	if(!file)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_FILE_OPEN);
		return FALSE;
	}

	while(fgets(buffer, sizeof(buffer), file))
	{
		LPINSTRUCTION instruction;
		LPSTR buff = buffer;

		// Remove whitespaces
		while(buff[0] && (buff[0] == ' ' || buff[0] == '\t')) ++buff;

		// Remove empty lines and comments
		if(!lstrlen(buff) || buff[0] == '\n' || buff[0] == ';')
			continue;
		
		instruction = ParseCommand(emulator, buff);
		if(!instruction)
		{
			fclose(file);
			return FALSE;
		}

		// -1 means a directive was parsed (directives are not stored in memory like instructions)
		if(instruction != LPINSTRUCTION_NONE)
		{
			emulator->memory[emulator->instructions] = (ULONG)instruction;
			++emulator->instructions;
		}
	}

	fclose(file);
	return TRUE;
}

BOOL LoadProgramFromFile(LPEMULATOR emulator, LPCSTR path)
{
	//
	// TODO Not yet implemented
	//
	DebugBreak();

	return TRUE;
}

BOOL ParseRegister(LPCSTR text, PULONG value)
{
	if(!lstrcmpi(text, "pc"))
		*value = EMULATOR_REGISTER_PROGRAM_COUNTER;
	else if(!lstrcmpi(text, "sp"))
		*value = EMULATOR_REGISTER_STACK_POINTER;
	else if(sscanf(text, "r%u", value) != 1)
		return FALSE;

	if(*value >= EMULATOR_REGISTERS)
		return FALSE;

	return TRUE;
}

BOOL ParseAddress(LPCSTR text, PULONG value)
{
	if(sscanf(text, "%u", value) != 1)
		return FALSE;

	return TRUE;
}

BOOL ParseConstant(LPCSTR text, PULONG value)
{
	if(sscanf(text, "#%u", value) != 1)
		return FALSE;

	return TRUE;
}

BOOL ParseCharacter(LPCSTR text, PULONG value)
{
	if(sscanf(text, "'%c'", value) != 1)
		return FALSE;

	return TRUE;
}

LPINSTRUCTION ParseCommand(LPEMULATOR emulator, LPCSTR text)
{
	ULONG index;
	CHAR name[EMULATOR_COMMAND_NAME];

	if(sscanf(text, "%s", name) != 1)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	for(index = 0; index < _countof(commands); ++index)
	{
		if(!_strcmpi(name, commands[index].name))
			return commands[index].parser(&commands[index], emulator, text);
	}

	SetEmulatorError(emulator, EMULATOR_ERROR_UNKNOWN_INSTRUCTION);
	return NULL;
}

LPINSTRUCTION ParseInstructionJump(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text)
{
	LPINSTRUCTIONJUMP instruction;
	CHAR name[EMULATOR_COMMAND_NAME];
	CHAR argument[EMULATOR_COMMAND_ARGUMENT];

	if(sscanf(text, "%s %s", name, argument) != 2)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	instruction = HeapAlloc(GetProcessHeap(), 0, sizeof(INSTRUCTIONJUMP));
	if(!instruction)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_NO_MEMORY);
		return NULL;
	}

	instruction->instruction.command = command;
	instruction->instruction.size = sizeof(INSTRUCTIONJUMP);

	if(ParseRegister(argument, &instruction->argument))
		instruction->type = ARGUMENT_REGISTER;
	else if(ParseAddress(argument, &instruction->argument))
		instruction->type = ARGUMENT_ADDRESS;
	else
	{
		HeapFree(GetProcessHeap(), 0, instruction);

		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	return (LPINSTRUCTION)instruction;
}

LPINSTRUCTION ParseInstructionCond(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text)
{
	LPINSTRUCTIONCOND instruction;
	CHAR name[EMULATOR_COMMAND_NAME];
	CHAR argument[EMULATOR_COMMAND_ARGUMENT];

	if(sscanf(text, "%s %s", name, argument) != 2)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	instruction = HeapAlloc(GetProcessHeap(), 0, sizeof(INSTRUCTIONCOND));
	if(!instruction)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_NO_MEMORY);
		return NULL;
	}

	instruction->instruction.command = command;
	instruction->instruction.size = sizeof(INSTRUCTIONCOND);
	
	if(ParseRegister(argument, &instruction->argument))
		instruction->type = ARGUMENT_REGISTER;
	else if(ParseConstant(argument, &instruction->argument))
		instruction->type = ARGUMENT_CONSTANT;
	else if(ParseAddress(argument, &instruction->argument))
		instruction->type = ARGUMENT_ADDRESS;
	else
	{
		HeapFree(GetProcessHeap(), 0, instruction);

		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	return (LPINSTRUCTION)instruction;
}

LPINSTRUCTION ParseInstructionMove(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text)
{
	LPINSTRUCTIONMOVE instruction;
	CHAR name[EMULATOR_COMMAND_NAME];
	CHAR arguments[2][EMULATOR_COMMAND_ARGUMENT];

	if(sscanf(text, "%s %s %s", name, arguments[0], arguments[1]) != 3)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	instruction = HeapAlloc(GetProcessHeap(), 0, sizeof(INSTRUCTIONMOVE));
	if(!instruction)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_NO_MEMORY);
		return NULL;
	}

	instruction->instruction.command = command;
	instruction->instruction.size = sizeof(INSTRUCTIONMOVE);
	
	if(ParseRegister(arguments[0], &instruction->arguments[0]))
		instruction->types[0] = ARGUMENT_REGISTER;
	else if(ParseConstant(arguments[0], &instruction->arguments[0]))
		instruction->types[0] = ARGUMENT_CONSTANT;
	else if(ParseAddress(arguments[0], &instruction->arguments[0]))
		instruction->types[0] = ARGUMENT_ADDRESS;
	else
	{
		HeapFree(GetProcessHeap(), 0, instruction);

		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	if(ParseRegister(arguments[1], &instruction->arguments[1]))
		instruction->types[1] = ARGUMENT_REGISTER;
	else if(ParseConstant(arguments[1], &instruction->arguments[1]))
		instruction->types[1] = ARGUMENT_CONSTANT;
	else if(ParseAddress(arguments[1], &instruction->arguments[1]))
		instruction->types[1] = ARGUMENT_ADDRESS;
	else
	{
		HeapFree(GetProcessHeap(), 0, instruction);

		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	return (LPINSTRUCTION)instruction;
}

LPINSTRUCTION ParseInstructionAdd(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text)
{
	LPINSTRUCTIONARTH instruction;
	CHAR name[EMULATOR_COMMAND_NAME];
	CHAR arguments[3][EMULATOR_COMMAND_ARGUMENT];

	if(sscanf(text, "%s %s %s %s", name, arguments[0], arguments[1], arguments[2]) != 4)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	instruction = HeapAlloc(GetProcessHeap(), 0, sizeof(INSTRUCTIONARTH));
	if(!instruction)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_NO_MEMORY);
		return NULL;
	}
	
	instruction->instruction.command = command;
	instruction->instruction.size = sizeof(INSTRUCTIONARTH);

	if(ParseRegister(arguments[0], &instruction->arguments[0]))
		instruction->types[0] = ARGUMENT_REGISTER;
	else
	{
		HeapFree(GetProcessHeap(), 0, instruction);

		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	if(ParseRegister(arguments[1], &instruction->arguments[1]))
		instruction->types[1] = ARGUMENT_REGISTER;
	else if(ParseConstant(arguments[1], &instruction->arguments[1]))
		instruction->types[1] = ARGUMENT_CONSTANT;
	else if(ParseAddress(arguments[1], &instruction->arguments[1]))
		instruction->types[1] = ARGUMENT_ADDRESS;
	else
	{
		HeapFree(GetProcessHeap(), 0, instruction);

		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	if(ParseRegister(arguments[2], &instruction->arguments[2]))
		instruction->types[2] = ARGUMENT_REGISTER;
	else if(ParseConstant(arguments[2], &instruction->arguments[2]))
		instruction->types[2] = ARGUMENT_CONSTANT;
	else if(ParseAddress(arguments[2], &instruction->arguments[2]))
		instruction->types[2] = ARGUMENT_ADDRESS;
	else
	{
		HeapFree(GetProcessHeap(), 0, instruction);

		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	return (LPINSTRUCTION)instruction;
}

LPINSTRUCTION ParseInstructionSub(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text)
{
	LPINSTRUCTIONARTH instruction;
	CHAR name[EMULATOR_COMMAND_NAME];
	CHAR arguments[3][EMULATOR_COMMAND_ARGUMENT];

	if(sscanf(text, "%s %s %s %s", name, arguments[0], arguments[1], arguments[2]) != 4)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	instruction = HeapAlloc(GetProcessHeap(), 0, sizeof(INSTRUCTIONARTH));
	if(!instruction)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_NO_MEMORY);
		return NULL;
	}
	
	instruction->instruction.command = command;
	instruction->instruction.size = sizeof(INSTRUCTIONARTH);

	if(ParseRegister(arguments[0], &instruction->arguments[0]))
		instruction->types[0] = ARGUMENT_REGISTER;
	else
	{
		HeapFree(GetProcessHeap(), 0, instruction);

		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	if(ParseRegister(arguments[1], &instruction->arguments[1]))
		instruction->types[1] = ARGUMENT_REGISTER;
	else if(ParseConstant(arguments[1], &instruction->arguments[1]))
		instruction->types[1] = ARGUMENT_CONSTANT;
	else if(ParseAddress(arguments[1], &instruction->arguments[1]))
		instruction->types[1] = ARGUMENT_ADDRESS;
	else
	{
		HeapFree(GetProcessHeap(), 0, instruction);

		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	if(ParseRegister(arguments[2], &instruction->arguments[2]))
		instruction->types[2] = ARGUMENT_REGISTER;
	else if(ParseConstant(arguments[2], &instruction->arguments[2]))
		instruction->types[2] = ARGUMENT_CONSTANT;
	else if(ParseAddress(arguments[2], &instruction->arguments[2]))
		instruction->types[2] = ARGUMENT_ADDRESS;
	else
	{
		HeapFree(GetProcessHeap(), 0, instruction);

		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	return (LPINSTRUCTION)instruction;
}

LPINSTRUCTION ParseInstructionWrite(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text)
{
	LPINSTRUCTIONIO instruction;
	CHAR name[EMULATOR_COMMAND_NAME];
	CHAR argument[EMULATOR_COMMAND_ARGUMENT];

	if(sscanf(text, "%s %s", name, argument) != 2)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	instruction = HeapAlloc(GetProcessHeap(), 0, sizeof(INSTRUCTIONIO));
	if(!instruction)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_NO_MEMORY);
		return NULL;
	}
	
	instruction->instruction.command = command;
	instruction->instruction.size = sizeof(INSTRUCTIONIO);

	if(ParseRegister(argument, &instruction->argument))
		instruction->type = ARGUMENT_REGISTER;
	else if(ParseCharacter(argument, &instruction->argument))
		instruction->type = ARGUMENT_CHARACTER;
	else if(ParseConstant(argument, &instruction->argument))
		instruction->type = ARGUMENT_CONSTANT;
	else
	{
		HeapFree(GetProcessHeap(), 0, instruction);

		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	return (LPINSTRUCTION)instruction;
}

LPINSTRUCTION ParseInstructionRead(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text)
{
	LPINSTRUCTIONIO instruction;
	CHAR name[EMULATOR_COMMAND_NAME];
	CHAR argument[EMULATOR_COMMAND_ARGUMENT];

	if(sscanf(text, "%s %s", name, argument) != 2)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	instruction = HeapAlloc(GetProcessHeap(), 0, sizeof(INSTRUCTIONIO));
	if(!instruction)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_NO_MEMORY);
		return NULL;
	}
	
	instruction->instruction.command = command;
	instruction->instruction.size = sizeof(INSTRUCTIONIO);

	if(ParseRegister(argument, &instruction->argument))
		instruction->type = ARGUMENT_REGISTER;
	else
	{
		HeapFree(GetProcessHeap(), 0, instruction);

		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	return (LPINSTRUCTION)instruction;
}

LPINSTRUCTION ParseInstructionLoad(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text)
{
	LPINSTRUCTIONMEMORY instruction;
	CHAR name[EMULATOR_COMMAND_NAME];
	CHAR arguments[2][EMULATOR_COMMAND_ARGUMENT];

	if(sscanf(text, "%s %s %s", name, arguments[0], arguments[1]) != 3)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	instruction = HeapAlloc(GetProcessHeap(), 0, sizeof(INSTRUCTIONMEMORY));
	if(!instruction)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_NO_MEMORY);
		return NULL;
	}
	
	instruction->instruction.command = command;
	instruction->instruction.size = sizeof(INSTRUCTIONMEMORY);

	if(!ParseRegister(arguments[0], &instruction->arguments[0]))
	{
		HeapFree(GetProcessHeap(), 0, instruction);

		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}
	
	instruction->types[0] = ARGUMENT_REGISTER;
	
	if(ParseRegister(arguments[1], &instruction->arguments[1]))
		instruction->types[1] = ARGUMENT_REGISTER;
	else if(ParseConstant(arguments[1], &instruction->arguments[1]))
		instruction->types[1] = ARGUMENT_CONSTANT;
	else
	{
		HeapFree(GetProcessHeap(), 0, instruction);

		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	return (LPINSTRUCTION)instruction;
}

LPINSTRUCTION ParseInstructionStore(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text)
{
	LPINSTRUCTIONMEMORY instruction;
	CHAR name[EMULATOR_COMMAND_NAME];
	CHAR arguments[2][EMULATOR_COMMAND_ARGUMENT];

	if(sscanf(text, "%s %s %s", name, arguments[0], arguments[1]) != 3)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	instruction = HeapAlloc(GetProcessHeap(), 0, sizeof(INSTRUCTIONMEMORY));
	if(!instruction)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_NO_MEMORY);
		return NULL;
	}
	
	instruction->instruction.command = command;
	instruction->instruction.size = sizeof(INSTRUCTIONMEMORY);

	if(ParseRegister(arguments[0], &instruction->arguments[0]))
		instruction->types[0] = ARGUMENT_REGISTER;
	else if(ParseConstant(arguments[0], &instruction->arguments[0]))
		instruction->types[0] = ARGUMENT_CONSTANT;
	else
	{
		HeapFree(GetProcessHeap(), 0, instruction);

		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	if(ParseRegister(arguments[1], &instruction->arguments[1]))
		instruction->types[1] = ARGUMENT_REGISTER;
	else if(ParseConstant(arguments[1], &instruction->arguments[1]))
		instruction->types[1] = ARGUMENT_CONSTANT;
	else
	{
		HeapFree(GetProcessHeap(), 0, instruction);

		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	return (LPINSTRUCTION)instruction;
}

LPINSTRUCTION ParseInstructionPush(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text)
{
	LPINSTRUCTIONSTACK instruction;
	CHAR name[EMULATOR_COMMAND_NAME];
	CHAR argument[EMULATOR_COMMAND_ARGUMENT];

	if(sscanf(text, "%s %s", name, argument) != 2)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	instruction = HeapAlloc(GetProcessHeap(), 0, sizeof(INSTRUCTIONMEMORY));
	if(!instruction)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_NO_MEMORY);
		return NULL;
	}
	
	instruction->instruction.command = command;
	instruction->instruction.size = sizeof(INSTRUCTIONMEMORY);

	if(ParseRegister(argument, &instruction->argument))
		instruction->type = ARGUMENT_REGISTER;
	else if(ParseConstant(argument, &instruction->argument))
		instruction->type = ARGUMENT_CONSTANT;
	else
	{
		HeapFree(GetProcessHeap(), 0, instruction);

		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	return (LPINSTRUCTION)instruction;
}

LPINSTRUCTION ParseInstructionPop(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text)
{
	LPINSTRUCTIONSTACK instruction;
	CHAR name[EMULATOR_COMMAND_NAME];
	CHAR argument[EMULATOR_COMMAND_ARGUMENT];

	if(sscanf(text, "%s %s", name, argument) != 2)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	instruction = HeapAlloc(GetProcessHeap(), 0, sizeof(INSTRUCTIONMEMORY));
	if(!instruction)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_NO_MEMORY);
		return NULL;
	}
	
	instruction->instruction.command = command;
	instruction->instruction.size = sizeof(INSTRUCTIONMEMORY);

	if(!ParseRegister(argument, &instruction->argument))
	{
		HeapFree(GetProcessHeap(), 0, instruction);

		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	instruction->type = ARGUMENT_REGISTER;

	return (LPINSTRUCTION)instruction;
}

LPINSTRUCTION ParseInstructionBreak(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text)
{
	LPINSTRUCTION instruction;
	CHAR name[EMULATOR_COMMAND_NAME];

	if(sscanf(text, "%s", name) != 1)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	instruction = HeapAlloc(GetProcessHeap(), 0, sizeof(INSTRUCTION));
	if(!instruction)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_NO_MEMORY);
		return NULL;
	}
	
	instruction->command = command;
	instruction->size = sizeof(INSTRUCTION);

	return instruction;
}

LPINSTRUCTION ParseDirectiveDefineWord(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text)
{
	CHAR name[EMULATOR_COMMAND_NAME];
	CHAR address[EMULATOR_COMMAND_ARGUMENT];
	CHAR word[EMULATOR_COMMAND_ARGUMENT];
	ULONG addr;
	ULONG constant;

	if(sscanf(text, "%s %s %s", name, address, word) != 3)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	if(!ParseAddress(address, &addr))
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	if(!ParseConstant(word, &constant))
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	if(!IsValidAddressWrite(emulator, addr, 1))
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	emulator->memory[addr] = constant;

	return LPINSTRUCTION_NONE;
}

LPINSTRUCTION ParseDirectiveDefineString(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text)
{
	CHAR name[EMULATOR_COMMAND_NAME];
	CHAR address[EMULATOR_COMMAND_ARGUMENT];
	CHAR string[EMULATOR_COMMAND_STRING];
	CHAR processed[EMULATOR_COMMAND_STRING];
	ULONG addr;
	ULONG index[2];

	if(sscanf(text, "%s %s \"%[^\"]s\"", name, address, string) != 3)
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	if(!ParseAddress(address, &addr))
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	for(index[0] = 0, index[1] = 0; string[index[0]]; ++index[0], ++index[1])
	{
		if(string[index[0]] == '\\')
		{
			++index[0];

			switch(string[index[0]])
			{
			case '\\': processed[index[1]] = '\\'; break;
			case 'a': processed[index[1]] = '\a'; break;
			case 'b': processed[index[1]] = '\b'; break;
			case 'f': processed[index[1]] = '\f'; break;
			case 'n': processed[index[1]] = '\n'; break;
			case 'r': processed[index[1]] = '\r'; break;
			case 't': processed[index[1]] = '\t'; break;
			case 'v': processed[index[1]] = '\v'; break;
			case '"': processed[index[1]] = '"'; break;
			case '0': processed[index[1]] = '\0'; break;
			default: emulator->error = EMULATOR_ERROR_INVALID_INSTRUCTION; return NULL;
			}
		}
		else
			processed[index[1]] = string[index[0]];
	}

	if(!IsValidAddressWrite(emulator, addr, index[1]))
	{
		SetEmulatorError(emulator, EMULATOR_ERROR_INVALID_INSTRUCTION);
		return NULL;
	}

	for(index[0] = 0; index[0] < index[1]; ++index[0])
		emulator->memory[addr + index[0]] = processed[index[0]];

	// TODO Reconsider automatic string null termination
	//emulator->memory[addr + index] = 0;

	return LPINSTRUCTION_NONE;
}

BOOL IsValidAddressRead(LPEMULATOR emulator, ULONG address, ULONG range)
{
	if(address >= emulator->instructions && address + range < emulator->capacity)
		return TRUE;

	return FALSE;
}

BOOL IsValidAddressWrite(LPEMULATOR emulator, ULONG address, ULONG range)
{
	if(address >= emulator->instructions && address + range < emulator->capacity)
		return TRUE;

	return FALSE;
}

BOOL IsValidAddressExecute(LPEMULATOR emulator, ULONG address)
{
	if(address < emulator->instructions)
		return TRUE;

	return FALSE;
}

BOOL ExecuteInstruction(LPEMULATOR emulator)
{
	LPINSTRUCTION instruction;

	if(!IsValidAddressExecute(emulator, emulator->registers[EMULATOR_REGISTER_PROGRAM_COUNTER]))
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_INVALID_INSTRUCTION);
		return FALSE;
	}

	instruction = (LPINSTRUCTION)emulator->memory[emulator->registers[EMULATOR_REGISTER_PROGRAM_COUNTER]];
	if(!instruction)
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_INVALID_INSTRUCTION);
		return FALSE;
	}

	if(!instruction->command || !instruction->command->executor)
		DebugBreak();	// Should not get here

	return instruction->command->executor(instruction, emulator);
}

BOOL ExecuteInstructionJump(LPINSTRUCTION inst, LPEMULATOR emulator)
{
	LPINSTRUCTIONJUMP instruction = (LPINSTRUCTIONJUMP)inst;
	if(!instruction)
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_NO_INSTRUCTION);
		return FALSE;
	}

	switch(instruction->type)
	{
	case ARGUMENT_ADDRESS:
		if(instruction->argument >= emulator->instructions)
		{
			SetEmulatorException(emulator, EMULATOR_EXCEPTION_INVALID_INSTRUCTION);
			return FALSE;
		}

		emulator->registers[EMULATOR_REGISTER_PROGRAM_COUNTER] = instruction->argument;
		break;

	case ARGUMENT_REGISTER:
		if(emulator->registers[instruction->argument] >= emulator->instructions)
		{
			SetEmulatorException(emulator, EMULATOR_EXCEPTION_INVALID_INSTRUCTION);
			return FALSE;
		}

		emulator->registers[EMULATOR_REGISTER_PROGRAM_COUNTER] = emulator->registers[instruction->argument];
		break;
	}

	//++emulator->registers[EMULATOR_REGISTER_PROGRAM_COUNTER];		// LOL
	return TRUE;
}

BOOL ExecuteInstructionCond(LPINSTRUCTION inst, LPEMULATOR emulator)
{
	LPINSTRUCTIONCOND instruction = (LPINSTRUCTIONCOND)inst;
	if(!instruction)
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_NO_INSTRUCTION);
		return FALSE;
	}

	switch(instruction->type)
	{
	case ARGUMENT_ADDRESS:
		if(instruction->argument >= emulator->capacity)
		{
			SetEmulatorException(emulator, EMULATOR_EXCEPTION_INVALID_INSTRUCTION);
			return FALSE;
		}

		if(emulator->memory[instruction->argument])
			++emulator->registers[EMULATOR_REGISTER_PROGRAM_COUNTER];
		break;

	case ARGUMENT_REGISTER:
		if(emulator->registers[instruction->argument])
			++emulator->registers[EMULATOR_REGISTER_PROGRAM_COUNTER];
		break;
	}

	++emulator->registers[EMULATOR_REGISTER_PROGRAM_COUNTER];
	return TRUE;
}

BOOL ExecuteInstructionMove(LPINSTRUCTION inst, LPEMULATOR emulator)
{
	LPINSTRUCTIONMOVE instruction = (LPINSTRUCTIONMOVE)inst;
	if(!instruction)
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_NO_INSTRUCTION);
		return FALSE;
	}

	if(instruction->types[0] == ARGUMENT_CONSTANT)
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_INVALID_INSTRUCTION);
		return FALSE;
	}

	if(instruction->types[0] == ARGUMENT_ADDRESS)
	{
		if(!IsValidAddressWrite(emulator, instruction->arguments[0], 1))
		{
			SetEmulatorException(emulator, EMULATOR_EXCEPTION_ACCESS_VIOLATION);
			return FALSE;
		}

		if(instruction->types[1] == ARGUMENT_ADDRESS)
		{
			if(!IsValidAddressRead(emulator, instruction->arguments[1], 1))
			{
				SetEmulatorException(emulator, EMULATOR_EXCEPTION_ACCESS_VIOLATION);
				return FALSE;
			}

			emulator->memory[instruction->arguments[0]] = emulator->memory[instruction->arguments[1]];
		}
		else if(instruction->types[1] == ARGUMENT_REGISTER)
		{
			emulator->memory[instruction->arguments[0]] = emulator->registers[instruction->arguments[1]];
		}
		else if(instruction->types[1] == ARGUMENT_CONSTANT)
		{
			emulator->memory[instruction->arguments[0]] = instruction->arguments[1];
		}
		else
		{
			SetEmulatorException(emulator, EMULATOR_EXCEPTION_INVALID_INSTRUCTION);
			return FALSE;
		}
	}
	else if(instruction->types[0] == ARGUMENT_REGISTER)
	{
		if(instruction->types[1] == ARGUMENT_ADDRESS)
		{
			if(!IsValidAddressRead(emulator, instruction->arguments[1], 1))
			{
				SetEmulatorException(emulator, EMULATOR_EXCEPTION_ACCESS_VIOLATION);
				return FALSE;
			}

			emulator->registers[instruction->arguments[0]] = emulator->memory[instruction->arguments[1]];
		}
		else if(instruction->types[1] == ARGUMENT_REGISTER)
		{
			emulator->registers[instruction->arguments[0]] = emulator->registers[instruction->arguments[1]];
		}
		else if(instruction->types[1] == ARGUMENT_CONSTANT)
		{
			emulator->registers[instruction->arguments[0]] = instruction->arguments[1];
		}
		else
		{
			SetEmulatorException(emulator, EMULATOR_EXCEPTION_INVALID_INSTRUCTION);
			return FALSE;
		}
	}
	else
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_INVALID_INSTRUCTION);
		return FALSE;
	}

	++emulator->registers[EMULATOR_REGISTER_PROGRAM_COUNTER];
	return TRUE;
}

BOOL ExecuteInstructionAdd(LPINSTRUCTION inst, LPEMULATOR emulator)
{
	ULONG values[2];
	LPINSTRUCTIONARTH instruction = (LPINSTRUCTIONARTH)inst;
	if(!instruction)
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_NO_INSTRUCTION);
		return FALSE;
	}

	if(instruction->types[1] == ARGUMENT_REGISTER)
		values[0] = emulator->registers[instruction->arguments[1]];
	else if(instruction->types[1] == ARGUMENT_ADDRESS)
	{
		if(!IsValidAddressRead(emulator, instruction->arguments[1], 1))
		{
			SetEmulatorException(emulator, EMULATOR_EXCEPTION_ACCESS_VIOLATION);
			return FALSE;
		}

		values[0] = emulator->memory[instruction->arguments[1]];
	}
	else if(instruction->types[1] == ARGUMENT_CONSTANT)
		values[0] = instruction->arguments[1];
	else
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_INVALID_INSTRUCTION);
		return FALSE;
	}

	if(instruction->types[2] == ARGUMENT_REGISTER)
		values[1] = emulator->registers[instruction->arguments[2]];
	else if(instruction->types[2] == ARGUMENT_ADDRESS)
	{
		if(!IsValidAddressRead(emulator, instruction->arguments[2], 1))
		{
			SetEmulatorException(emulator, EMULATOR_EXCEPTION_ACCESS_VIOLATION);
			return FALSE;
		}

		values[1] = emulator->memory[instruction->arguments[2]];
	}
	else if(instruction->types[2] == ARGUMENT_CONSTANT)
		values[1] = instruction->arguments[2];
	else
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_INVALID_INSTRUCTION);
		return FALSE;
	}

	emulator->registers[instruction->arguments[0]] = values[0] + values[1];

	++emulator->registers[EMULATOR_REGISTER_PROGRAM_COUNTER];
	return TRUE;
}

BOOL ExecuteInstructionSub(LPINSTRUCTION inst, LPEMULATOR emulator)
{
	ULONG values[2];
	LPINSTRUCTIONARTH instruction = (LPINSTRUCTIONARTH)inst;
	if(!instruction)
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_NO_INSTRUCTION);
		return FALSE;
	}

	if(instruction->types[1] == ARGUMENT_REGISTER)
		values[0] = emulator->registers[instruction->arguments[1]];
	else if(instruction->types[1] == ARGUMENT_ADDRESS)
	{
		if(!IsValidAddressRead(emulator, instruction->arguments[1], 1))
		{
			SetEmulatorException(emulator, EMULATOR_EXCEPTION_ACCESS_VIOLATION);
			return FALSE;
		}

		values[0] = emulator->memory[instruction->arguments[1]];
	}
	else if(instruction->types[1] == ARGUMENT_CONSTANT)
		values[0] = instruction->arguments[1];
	else
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_INVALID_INSTRUCTION);
		return FALSE;
	}

	if(instruction->types[2] == ARGUMENT_REGISTER)
		values[1] = emulator->registers[instruction->arguments[2]];
	else if(instruction->types[2] == ARGUMENT_ADDRESS)
	{
		if(!IsValidAddressRead(emulator, instruction->arguments[2], 1))
		{
			SetEmulatorException(emulator, EMULATOR_EXCEPTION_ACCESS_VIOLATION);
			return FALSE;
		}

		values[1] = emulator->memory[instruction->arguments[2]];
	}
	else if(instruction->types[2] == ARGUMENT_CONSTANT)
		values[1] = instruction->arguments[2];
	else
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_INVALID_INSTRUCTION);
		return FALSE;
	}

	emulator->registers[instruction->arguments[0]] = values[0] - values[1];

	++emulator->registers[EMULATOR_REGISTER_PROGRAM_COUNTER];
	return TRUE;
}

BOOL ExecuteInstructionWrite(LPINSTRUCTION inst, LPEMULATOR emulator)
{
	ULONG written;
	LPINSTRUCTIONIO instruction = (LPINSTRUCTIONIO)inst;
	if(!instruction)
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_NO_INSTRUCTION);
		return FALSE;
	}

	if(instruction->type == ARGUMENT_CHARACTER || instruction->type == ARGUMENT_CONSTANT)
		WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), &instruction->argument, 1, &written, NULL);
	else if(instruction->type == ARGUMENT_REGISTER)
		WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), &emulator->registers[instruction->argument], 1, &written, NULL);
	else
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_INVALID_INSTRUCTION);
		return FALSE;
	}

	++emulator->registers[EMULATOR_REGISTER_PROGRAM_COUNTER];
	return TRUE;
}

BOOL ExecuteInstructionRead(LPINSTRUCTION inst, LPEMULATOR emulator)
{
	ULONG read;
	ULONG mode;
	LPINSTRUCTIONIO instruction = (LPINSTRUCTIONIO)inst;
	if(!instruction)
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_NO_INSTRUCTION);
		return FALSE;
	}

	GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode & ~ENABLE_LINE_INPUT & ~ENABLE_ECHO_INPUT);
	
	if(instruction->type == ARGUMENT_REGISTER)
		ReadConsole(GetStdHandle(STD_INPUT_HANDLE), &emulator->registers[instruction->argument], 1, &read, NULL);
	else
	{
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);

		SetEmulatorException(emulator, EMULATOR_EXCEPTION_INVALID_INSTRUCTION);
		return FALSE;
	}

	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);

	++emulator->registers[EMULATOR_REGISTER_PROGRAM_COUNTER];
	return TRUE;
}

BOOL ExecuteInstructionLoad(LPINSTRUCTION inst, LPEMULATOR emulator)
{
	ULONG address;
	LPINSTRUCTIONMEMORY instruction = (LPINSTRUCTIONMEMORY)inst;
	if(!instruction)
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_NO_INSTRUCTION);
		return FALSE;
	}

	if(instruction->types[1] == ARGUMENT_REGISTER)
		address = emulator->registers[instruction->arguments[1]];
	else if(instruction->types[1] == ARGUMENT_CONSTANT)
		address = instruction->arguments[1];
	else
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_INVALID_INSTRUCTION);
		return FALSE;
	}
	
	if(!IsValidAddressRead(emulator, address, 1))
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_ACCESS_VIOLATION);
		return FALSE;
	}

	emulator->registers[instruction->arguments[0]] = emulator->memory[address];

	++emulator->registers[EMULATOR_REGISTER_PROGRAM_COUNTER];
	return TRUE;
}

BOOL ExecuteInstructionStore(LPINSTRUCTION inst, LPEMULATOR emulator)
{
	ULONG address;
	ULONG value;
	LPINSTRUCTIONMEMORY instruction = (LPINSTRUCTIONMEMORY)inst;
	if(!instruction)
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_NO_INSTRUCTION);
		return FALSE;
	}

	if(instruction->types[0] == ARGUMENT_REGISTER)
		address = emulator->registers[instruction->arguments[0]];
	else if(instruction->types[0] == ARGUMENT_CONSTANT)
		address = instruction->arguments[0];
	else
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_INVALID_INSTRUCTION);
		return FALSE;
	}

	if(instruction->types[1] == ARGUMENT_REGISTER)
		value = emulator->registers[instruction->arguments[1]];
	else if(instruction->types[1] == ARGUMENT_CONSTANT)
		value = instruction->arguments[1];
	else
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_INVALID_INSTRUCTION);
		return FALSE;
	}
	
	if(!IsValidAddressWrite(emulator, address, 1))
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_ACCESS_VIOLATION);
		return FALSE;
	}

	emulator->memory[address] = value;

	++emulator->registers[EMULATOR_REGISTER_PROGRAM_COUNTER];
	return TRUE;
}

BOOL ExecuteInstructionPush(LPINSTRUCTION inst, LPEMULATOR emulator)
{
	ULONG value;
	LPINSTRUCTIONSTACK instruction = (LPINSTRUCTIONSTACK)inst;
	if(!instruction)
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_NO_INSTRUCTION);
		return FALSE;
	}

	if(!IsValidAddressWrite(emulator, emulator->registers[EMULATOR_REGISTER_STACK_POINTER], 1))
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_ACCESS_VIOLATION);
		return FALSE;
	}

	if(instruction->type == ARGUMENT_REGISTER)
		value = emulator->registers[instruction->argument];
	else if(instruction->type == ARGUMENT_CONSTANT)
		value = instruction->argument;
	else
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_INVALID_INSTRUCTION);
		return FALSE;
	}

	emulator->memory[emulator->registers[EMULATOR_REGISTER_STACK_POINTER]] = value;

	++emulator->registers[EMULATOR_REGISTER_STACK_POINTER];
	++emulator->registers[EMULATOR_REGISTER_PROGRAM_COUNTER];
	return TRUE;
}

BOOL ExecuteInstructionPop(LPINSTRUCTION inst, LPEMULATOR emulator)
{
	LPINSTRUCTIONSTACK instruction = (LPINSTRUCTIONSTACK)inst;
	if(!instruction)
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_NO_INSTRUCTION);
		return FALSE;
	}

	if(!IsValidAddressRead(emulator, emulator->registers[EMULATOR_REGISTER_STACK_POINTER] - 1, 1))
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_ACCESS_VIOLATION);
		return FALSE;
	}

	emulator->registers[instruction->argument] = emulator->memory[emulator->registers[EMULATOR_REGISTER_STACK_POINTER] - 1];

	--emulator->registers[EMULATOR_REGISTER_STACK_POINTER];
	++emulator->registers[EMULATOR_REGISTER_PROGRAM_COUNTER];
	return TRUE;
}

BOOL ExecuteInstructionBreak(LPINSTRUCTION inst, LPEMULATOR emulator)
{
	LPINSTRUCTION instruction = (LPINSTRUCTION)inst;
	if(!instruction)
	{
		SetEmulatorException(emulator, EMULATOR_EXCEPTION_NO_INSTRUCTION);
		return FALSE;
	}

	SetEmulatorException(emulator, EMULATOR_EXCEPTION_NONE);
	return FALSE;
}

VOID SetEmulatorException(LPEMULATOR emulator, ULONG exception)
{
	if(emulator->exception != EMULATOR_EXCEPTION_NONE)
		DebugBreak();	// Should not get here

	emulator->exception = exception;
}

VOID SetEmulatorError(LPEMULATOR emulator, ULONG error)
{
	if(emulator->error != EMULATOR_ERROR_NONE)
		DebugBreak();	// Should not get here

	emulator->error = error;
}