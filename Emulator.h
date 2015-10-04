#pragma once

#include <windows.h>

#define EMULATOR_COMMAND_NAME		64		// Max length of command name buffer
#define EMULATOR_COMMAND_ARGUMENT	64		// Max length of command argument buffer
#define EMULATOR_COMMAND_STRING		512		// Max length of command string buffer
#define EMULATOR_READ_BUFFER 4096			// Line buffer size used when parsing assembly source files
#define EMULATOR_DEFAULT_MEMORY 8192		// Default size of the memory space for the emulator (used if 0 passed to the emulator initialization function)

#define EMULATOR_REGISTERS 16				// Number of emulator registers
#define EMULATOR_REGISTER_PROGRAM_COUNTER (EMULATOR_REGISTERS-1)	// Index of the program counter register
#define EMULATOR_REGISTER_STACK_POINTER (EMULATOR_REGISTERS-2)		// Index of the stack pointer register

typedef struct
{
	PULONG memory;		// The memory of the emulator
	ULONG capacity;		// Total size of the emulator memory
	ULONG instructions;	// Size of the emulator memory populated by instructions, starts at 0x00000000
	ULONG exception;	// Error douring execution
	ULONG error;		// Error douring parsing/loading
	ULONG registers[EMULATOR_REGISTERS];
} EMULATOR,*LPEMULATOR;

// Instruction types
#define INSTRUCTION_NONE	0
#define INSTRUCTION_JUMP	1
#define INSTRUCTION_COND	2
#define INSTRUCTION_MOVE	3
#define INSTRUCTION_ADD		4
#define INSTRUCTION_SUB		5
#define INSTRUCTION_WRITE	6
#define INSTRUCTION_READ	7
#define INSTRUCTION_LOAD	8
#define INSTRUCTION_STORE	9
#define INSTRUCTION_PUSH	10
#define INSTRUCTION_POP		11
#define INSTRUCTION_BREAK	12
//...

// Argument types
#define ARGUMENT_NONE		0
#define ARGUMENT_ADDRESS	1
#define ARGUMENT_REGISTER	2
#define ARGUMENT_CONSTANT	3
#define ARGUMENT_CHARACTER	4

// Exception types
#define EMULATOR_EXCEPTION_NONE					0
#define EMULATOR_EXCEPTION_INVALID_INSTRUCTION	1
#define EMULATOR_EXCEPTION_ACCESS_VIOLATION		2
#define EMULATOR_EXCEPTION_NO_INSTRUCTION		3

// Error types
#define EMULATOR_ERROR_NONE						0
#define EMULATOR_ERROR_INVALID_INSTRUCTION		1
#define EMULATOR_ERROR_UNKNOWN_INSTRUCTION		2
#define EMULATOR_ERROR_NO_MEMORY				3
#define EMULATOR_ERROR_FILE_OPEN				4

typedef struct COMMAND* LPCOMMAND;
typedef struct INSTRUCTION* LPINSTRUCTION;

// Prototype for command parsers
typedef LPINSTRUCTION (*LPCOMMANDPARSER)(LPCOMMAND, LPEMULATOR, LPCSTR);

// Prototype for instruction executors
typedef BOOL (*LPCOMMANDEXECUTOR)(LPINSTRUCTION, LPEMULATOR);

// Structure for the command registry
typedef struct COMMAND
{
	LPCSTR name;
	ULONG type;
	LPCOMMANDPARSER parser;
	LPCOMMANDEXECUTOR executor;
} *LPCOMMAND;

typedef struct COMMAND COMMAND;

// Base instruction structure, all instruction structures begin with this struct
typedef struct INSTRUCTION
{
	LPCOMMAND command;
	ULONG size;
} *LPINSTRUCTION;

typedef struct INSTRUCTION INSTRUCTION;

// This define is returned by directive parsers to indicate a successful parse operation but no instruction generation
#define LPINSTRUCTION_NONE (LPINSTRUCTION)-1

#pragma region Instruction structures
typedef struct
{
	INSTRUCTION instruction;

	ULONG argument;
	ULONG type;
} INSTRUCTIONJUMP,*LPINSTRUCTIONJUMP;

typedef struct
{
	INSTRUCTION instruction;

	ULONG argument;
	ULONG type;
} INSTRUCTIONCOND,*LPINSTRUCTIONCOND;

typedef struct
{
	INSTRUCTION instruction;

	ULONG arguments[2];
	ULONG types[2];
} INSTRUCTIONMOVE,*LPINSTRUCTIONMOVE;

// ADD,SUB,MUL,DIV
typedef struct
{
	INSTRUCTION instruction;

	ULONG arguments[3];
	ULONG types[3];
} INSTRUCTIONARTH,*LPINSTRUCTIONARTH;

// WRITE,READ
typedef struct
{
	INSTRUCTION instruction;

	ULONG argument;
	ULONG type;
} INSTRUCTIONIO,*LPINSTRUCTIONIO;

// LOAD,STORE
typedef struct
{
	INSTRUCTION instruction;

	ULONG arguments[2];
	ULONG types[2];
} INSTRUCTIONMEMORY,*LPINSTRUCTIONMEMORY;

// PUSH,POP
typedef struct
{
	INSTRUCTION instruction;

	ULONG argument;
	ULONG type;
} INSTRUCTIONSTACK,*LPINSTRUCTIONSTACK;
#pragma endregion

// Execution memory address sanity checker
BOOL IsValidAddressExecute(LPEMULATOR emulator, ULONG address);
// Write memory address sanity checker
BOOL IsValidAddressWrite(LPEMULATOR emulator, ULONG address, ULONG range);
// Read memory address sanity checker
BOOL IsValidAddressRead(LPEMULATOR emulator, ULONG address, ULONG range);

// Instruction executor functions
BOOL ExecuteInstructionJump(LPINSTRUCTION inst, LPEMULATOR emulator);
BOOL ExecuteInstructionCond(LPINSTRUCTION inst, LPEMULATOR emulator);
BOOL ExecuteInstructionMove(LPINSTRUCTION inst, LPEMULATOR emulator);
BOOL ExecuteInstructionAdd(LPINSTRUCTION inst, LPEMULATOR emulator);
BOOL ExecuteInstructionSub(LPINSTRUCTION inst, LPEMULATOR emulator);
BOOL ExecuteInstructionWrite(LPINSTRUCTION inst, LPEMULATOR emulator);
BOOL ExecuteInstructionRead(LPINSTRUCTION inst, LPEMULATOR emulator);
BOOL ExecuteInstructionLoad(LPINSTRUCTION inst, LPEMULATOR emulator);
BOOL ExecuteInstructionStore(LPINSTRUCTION inst, LPEMULATOR emulator);
BOOL ExecuteInstructionPush(LPINSTRUCTION inst, LPEMULATOR emulator);
BOOL ExecuteInstructionPop(LPINSTRUCTION inst, LPEMULATOR emulator);
BOOL ExecuteInstructionBreak(LPINSTRUCTION inst, LPEMULATOR emulator);

// Register string parser
BOOL ParseRegister(LPCSTR text, PULONG value);
// Address string parser
BOOL ParseAddress(LPCSTR text, PULONG value);
// Constant string parser
BOOL ParseConstant(LPCSTR text, PULONG value);
// Character string parser
BOOL ParseCharacter(LPCSTR text, PULONG value);

// General instruction/directive parser function, calls the specific instruction/directive parser function based on the instruction's name
LPINSTRUCTION ParseCommand(LPEMULATOR emulator, LPCSTR text);

// Instruction parser functions
LPINSTRUCTION ParseInstructionJump(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text);
LPINSTRUCTION ParseInstructionCond(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text);
LPINSTRUCTION ParseInstructionMove(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text);
LPINSTRUCTION ParseInstructionAdd(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text);
LPINSTRUCTION ParseInstructionSub(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text);
LPINSTRUCTION ParseInstructionWrite(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text);
LPINSTRUCTION ParseInstructionRead(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text);
LPINSTRUCTION ParseInstructionLoad(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text);
LPINSTRUCTION ParseInstructionStore(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text);
LPINSTRUCTION ParseInstructionPush(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text);
LPINSTRUCTION ParseInstructionPop(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text);
LPINSTRUCTION ParseInstructionBreak(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text);

// Directive parser functions
LPINSTRUCTION ParseDirectiveDefineWord(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text);
LPINSTRUCTION ParseDirectiveDefineString(LPCOMMAND command, LPEMULATOR emulator, LPCSTR text);

// Initializes the emulator internal data structures
BOOL InitializeEmulator(LPEMULATOR emulator, ULONG memory);
// Frees the memory associated with the emulator internal data structures
VOID UninitializeEmulator(LPEMULATOR emulator);

// Loads a program into a initialized emulator from a textual assembly source file
BOOL LoadProgramFromSourceFile(LPEMULATOR emulator, LPCSTR path);

// Loads a program into a initialized emulator from a binary file
BOOL LoadProgramFromFile(LPEMULATOR emulator, LPCSTR path);

// Executes a singe instruction at the current instruction position
BOOL ExecuteInstruction(LPEMULATOR emulator);

VOID SetEmulatorException(LPEMULATOR emulator, ULONG exception);
VOID SetEmulatorError(LPEMULATOR emulator, ULONG error);