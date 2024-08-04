/* 	Fries-8 Standard Library
	Project creation date: 8/4/2024 */

#include <time.h>
#include <stdlib.h>
#include <string.h>

// If target system has access to stdio.h (printf etc)
#define HAS_STDIO
#ifdef HAS_STDIO
	#include <stdio.h>
#endif
#undef HAS_STDIO
// Constants
#define MAX_ROM_SIZE			0xE00			// Maximum ROM size, by default 3584 bytes (0xE00 = 0x1000 - 0x200)
#define V_REGISTER_AMOUNT 		16
#define MEMORY_MAP_SIZE 		0x1000
#define CHARACTER_ADDRESS_OFFSET 	0x50
#define START_ADDRESS_OFFSET 		0x200
#define STACK_SIZE 			16
#define DISPLAY_WIDTH 			64
#define DISPLAY_HEIGHT 			32
#define KEYPAD_SIZE 			16
#define CHARACTER_SET_SIZE 		80
#define CHARACTER_BYTES_LENGTH		5

// Macros
#define OPCODE_N	(machine->opcode & 0x000F)
#define OPCODE_X 	(machine->opcode & 0x0F00) >> 8
#define OPCODE_Y 	(machine->opcode & 0x00F0) >> 4
#define OPCODE_ADDR	(machine->opcode & 0x0FFF)
#define OPCODE_BYTE	(machine->opcode & 0x00FF)

/* Fries-8 Specific Definitions */
#define PIXEL_AT_XY 	machine->display[(machine->Registers.V[OPCODE_Y] * DISPLAY_WIDTH) + (machine->Registers.V[OPCODE_X])]


// Errors
typedef enum Error { INVALID_OPCODE = 1, EXCESSIVE_ROM_SIZE = 2, STATUS_OK = 0, UNKNOWN_ERROR = 3 } Errorable;

// Components
struct Chip8Registers
{
	unsigned char V[V_REGISTER_AMOUNT];	// 16 8-bit registers V0-VF
	unsigned char DT;			// 8-bit delay timer
	unsigned char ST;			// 8-bit sound timer
	unsigned char SP;			// 8-bit stack pointer
	unsigned short I;			// 16-bit index register I
	unsigned short PC;			// 16-bit program counter

	/* Fries-8 Specific */
	unsigned char IP;
};

const unsigned char character_set[CHARACTER_SET_SIZE] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, 		// 0
	0x20, 0x60, 0x20, 0x20, 0x70, 		// 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, 		// 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, 		// 3
	0x90, 0x90, 0xF0, 0x10, 0x10, 		// 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, 		// 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, 		// 6
	0xF0, 0x10, 0x20, 0x40, 0x40, 		// 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, 		// 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, 		// 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, 		// A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, 		// B
	0xF0, 0x80, 0x80, 0x80, 0xF0, 		// C
	0xE0, 0x90, 0x90, 0x90, 0xE0, 		// D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, 		// E
	0xF0, 0x80, 0xF0, 0x80, 0x80  		// F
};

struct Chip8Machine
{
	struct Chip8Registers Registers;				// Struct of 16 registers plus program counter, increment and stack pointer
	unsigned char memory[MEMORY_MAP_SIZE];				// Memory map with address space 0x000 to 0xFFF
	unsigned char display[DISPLAY_WIDTH * DISPLAY_HEIGHT];		// 64 x 32 display
	unsigned char keypad[KEYPAD_SIZE];				// Keypad with 16 keys
	unsigned char random_byte;					// Random byte, randomly generated each cycle
	unsigned short opcode;						// Current opcode
	unsigned short stack[STACK_SIZE];				// 16-layer program stack
};


// Functions
/* Load character set into memory starting from 0x50 (CHARACTER_ADRESS_OFFSET) */
void load_charset(struct Chip8Machine *machine)
{
//	for (unsigned int x = 0; x < CHARACTER_SET_SIZE; ++x)
//	{
//		machine->memory[CHARACTER_ADDRESS_OFFSET + x] = character_set[x];
//	}

	memcpy(machine->memory + CHARACTER_ADDRESS_OFFSET, character_set, CHARACTER_SET_SIZE * sizeof(char));
}

// CPU instruction functions
/* 	00E0 - CLS
	Clear the display. */
void OP_00E0(struct Chip8Machine *machine)
{
	memset(machine->display, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT);						// Set all pixels to 0
}

/*	00EE - RET
	Return from a subroutine. */
void OP_00EE(struct Chip8Machine *machine)
{
	--machine->Registers.SP;										// Decrement stack pointer
	machine->Registers.PC = machine->stack[machine->Registers.SP];						// Set program counter to the new stack pointer
}

/*	1nnn - JP addr
	Jump to location nnnn. */
void OP_1nnn(struct Chip8Machine *machine)
{
	machine->Registers.PC = OPCODE_ADDR;									// Mask away first MSB and assign to program counter
}

/*	2nnn - CALL addr
	Call subroutine at nnn. */
void OP_2nnn(struct Chip8Machine *machine)
{
	machine->stack[machine->Registers.SP] = machine->Registers.PC;						// Put current PC onto the top of the stack
	++machine->Registers.SP;										// Increment stack pointer
	machine->Registers.PC = OPCODE_ADDR;									// Mask away first MSB and assign to program counter
}

/*	3xkk - SE Vx, byte
	Skip next instruction if Vx = kk. */
void OP_3xkk(struct Chip8Machine *machine)
{
	if (machine->Registers.V[OPCODE_X] == OPCODE_BYTE)							// If Vx equals byte
		machine->Registers.PC += 2;									// Increment PC by 2
}

/*	4xkk - SNE Vx, byte
	Skip next instruction if Vx != kk. */
void OP_4xkk(struct Chip8Machine *machine)
{
	if (machine->Registers.V[OPCODE_X] != OPCODE_BYTE)							// If Vx does not equal byte
		machine->Registers.PC += 2;									// Increment PC by 2 if Vx != byte
}

/*	5xy0 - SE Vx, Vy
	Skip next instruction if Vx = Vy. */
void OP_5xy0(struct Chip8Machine *machine)
{
	if (machine->Registers.V[OPCODE_X] == machine->Registers.V[OPCODE_Y])					// If Vx equals Vy
		machine->Registers.PC += 2;									// Increment PC
}


/*	6xkk - LD Vx, byte
	Set Vx = kk. */
void OP_6xkk(struct Chip8Machine *machine)
{
	machine->Registers.V[OPCODE_X] = OPCODE_BYTE;								// Set Vx to byte
}

/*	7xkk - ADD Vx, byte
	Set Vx = Vx + kk. */
void OP_7xkk(struct Chip8Machine *machine)
{
	machine->Registers.V[OPCODE_X] += OPCODE_BYTE;								// Increment Vx by byte
}

/*	8xy0 - LD Vx, Vy
	Set Vx = Vy. */
void OP_8xy0(struct Chip8Machine *machine)
{
	machine->Registers.V[OPCODE_X] = machine->Registers.V[OPCODE_Y];					// Set Vx to Vy
}

/*	8xy1 - OR Vx, Vy
	Set Vx = Vx OR Vy. */
void OP_8xy1(struct Chip8Machine *machine)
{
	machine->Registers.V[OPCODE_X] |= machine->Registers.V[OPCODE_Y];					// Set Vx to Vx OR Vy
}

/*	8xy2 - AND Vx, Vy
	Set Vx = Vx AND Vy. */
void OP_8xy2(struct Chip8Machine *machine)
{
	machine->Registers.V[OPCODE_X] &= machine->Registers.V[OPCODE_Y];					// Set Vx to Vx AND Vy
}

/*	8xy3 - XOR Vx, Vy
	Set Vx = Vx XOR Vy. */
void OP_8xy3(struct Chip8Machine *machine)
{
	machine->Registers.V[OPCODE_X] ^= machine->Registers.V[OPCODE_Y];					// Set Vx to Vx XOR Vy
}

/*	8xy4 - ADD Vx, Vy
	Set Vx = Vx + Vy, set VF = carry. */
void OP_8xy4(struct Chip8Machine *machine)
{
	unsigned short result = machine->Registers.V[OPCODE_X] + machine->Registers.V[OPCODE_Y];		// Calculate Vx + Vy
	machine->Registers.V[OPCODE_X] = (unsigned char)result;							// Set Vx = Vx + Vy
	if (result > 0xFF)											// On overflow
		machine->Registers.V[0xF] = 1;									// Set VF = 1
	else													// Otherwise
		machine->Registers.V[0xF] = 0;									// Set VF = 0
}

/*	8xy5 - SUB Vx, Vy
	Set Vx = Vx - Vy, set VF = NOT borrow. */
void OP_8xy5(struct Chip8Machine *machine)
{
	unsigned char temp_v = machine->Registers.V[0xF];							// Temporary register

	if (machine->Registers.V[OPCODE_X] >= machine->Registers.V[OPCODE_Y])					// On borrow
		machine->Registers.V[0xF] = 1;									// Set VF = 1
	else													// Otherwise
		machine->Registers.V[0xF] = 0;									// Set VF = 0

	// Account for edge-cases
	if ((OPCODE_X == 0xF) || (OPCODE_X == 0xF && OPCODE_Y == 0xF))						// On cases such as VF = Vx - VF
		return;												// Exit, ensuring that VF is correctly set
														// as the value of NOT borrow
	if (OPCODE_Y == 0xF)											// On cases such as Vx = Vx - VF
	{
		machine->Registers.V[OPCODE_X] -= temp_v;							// Ensure correct subtraction
		return;												// Exit, ensuring that Vx != Vx - !borrow
	}

	// Base case
	machine->Registers.V[OPCODE_X] -= machine->Registers.V[OPCODE_Y];					// Standard expected behaviour (Vx = Vx - Vy)
}

/*	8xy6 - SHR Vx
	Set Vx = Vx SHR 1
	If the least significant bit of Vx is 1, then VF is set to 1, otherwise 0.
	Then Vx is divided by 2. */
void OP_8xy6(struct Chip8Machine *machine)
{

	if (machine->Registers.V[OPCODE_X] % 2)									// If LSB of Vx is 1
		machine->Registers.V[0xF] = 1;									// Set VF to 1
	else													// Otherwise
		machine->Registers.V[0xF] = 0;									// Set VF to 0

	if (OPCODE_X == 0xF)											// Edge-case where VF = VF SHR 1
		return;												// Return without shifting right

	machine->Registers.V[OPCODE_X] >>= 1;									// Base-case where Vx SHR 1

}

/*	8xy7 - SUBN Vx, Vy
	Set Vx = Vy - Vx, set VF = NOT borrow.
	If Vy > Vx, then VF is set 1, otherwise 0. Then Vx is subtracted from Vy,
	and the results are stored in Vx. */
void OP_8xy7(struct Chip8Machine *machine)
{
	machine->Registers.V[OPCODE_X] = machine->Registers.V[OPCODE_Y] - machine->Registers.V[OPCODE_X];	// Vx = Vy - Vx.
	if (machine->Registers.V[OPCODE_Y] >= machine->Registers.V[OPCODE_X])					// If NOT borrow
		machine->Registers.V[0xF] = 1;									// Set VF = 1
	else													// Otheriwse
		machine->Registers.V[0xF] = 0;									// Set VF = 0

}

/*	8xyE - SHL Vx {, Vy}
	Set Vx = Vx SHL 1.
	If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0.
	Then Vx is multiplied by 2. */

void OP_8xyE(struct Chip8Machine *machine)
{
	machine->Registers.V[0xF] = (machine->Registers.V[OPCODE_X] > 0x80);					// VF = Most significant bit of VX.
	if (OPCODE_X == 0xF)											// Edge-case where VF = VF SHL 1
		return;												// Exit without shifting VF left
	machine->Registers.V[OPCODE_X] <<= 1;									// Base-case where Vx is shifted left
}

/*	9xy0 - SNE Vx, Vy
	Skip next instruction if Vx != Vy. */
void OP_9xy0(struct Chip8Machine *machine)
{
	if (machine->Registers.V[OPCODE_X] != machine->Registers.V[OPCODE_Y])					// If Vx is not equal to Vy.
		machine->Registers.PC += 2;									// Increment PC by 2.
}

/*	Annn - LD I, addr
	Set I = nnn. */
void OP_Annn(struct Chip8Machine *machine)
{
	machine->Registers.I = OPCODE_ADDR;									// Set I to nnn (addr).
}

/*	Bnnn - JP V0, addr
	Jump to location nnn + V0. */
void OP_Bnnn(struct Chip8Machine *machine)
{
	machine->Registers.PC = OPCODE_ADDR + machine->Registers.V[0x0];					// Set PC to nnn (addr) + V0.
}

/*	Cxkk - RND Vx, byte
	Set Vx = random byte AND kk. */
void OP_Cxkk(struct Chip8Machine *machine)
{
	machine->Registers.V[OPCODE_X] = machine->random_byte & OPCODE_BYTE;					// Set Vx to a random number between 0 and 255
}														// and kk (byte).

/*	Dxyn - DRW Vx, Vy, nibble
	Display n-byte sprite starting at memory location I at (Vx, Vy),
	set VF = collision. */
void OP_Dxyn(struct Chip8Machine *machine)
{
	unsigned char x_position = machine->Registers.V[OPCODE_X] % DISPLAY_WIDTH;				// Horizontal screen boundary wrapping.
	unsigned char y_position = machine->Registers.V[OPCODE_Y] % DISPLAY_HEIGHT;				// Vertical screen boundary wrapping.

	machine->Registers.V[0xF] = 0;										// Set VF to 0.
	for (unsigned char row = 0; row < OPCODE_N; ++row)							// For each row
	{
		unsigned char sprite_byte = machine->memory[machine->Registers.I + row];			// Store sprite data as a byte
		for (unsigned char column = 0; column < 8; ++column)						// For each column
		{
			unsigned char sprite_pixel = sprite_byte & (0x80 >> column);				// Get if sprite pixel should be on or off
			unsigned char* display_pixel = &machine->display[(y_position + row) * DISPLAY_WIDTH + (x_position + column)];	// Store pointer to current pixel in machine->display as a byte
			if (sprite_pixel)									// If sprite pixel is on
			{
				machine->Registers.V[0xF] |= (*display_pixel == 0xFF);				// Set VF to the state of display_pixel
				*display_pixel ^= 0xFF;								// XOR Display Pixel with state of sprite pixel (0xFF in this case)
			}
		}
	}
}

/*	Ex9E - SKP Vx
	Skip next instruction if key with the value of Vx is pressed. */

void OP_Ex9E(struct Chip8Machine *machine)
{
	if (machine->keypad[machine->Registers.V[OPCODE_X]])
		machine->Registers.PC += 2;
}

/*	ExA1 - SKNP Vx
	Skip next instruction if key with the value of Vx is not pressed. */
void OP_ExA1(struct Chip8Machine *machine)
{
	if (!machine->keypad[machine->Registers.V[OPCODE_X]])
		machine->Registers.PC += 2;
}

/*	Fx07 - LD Vx, DT
	Set Vx = delay timer value. */
void OP_Fx07(struct Chip8Machine *machine)
{
	machine->Registers.V[OPCODE_X] = machine->Registers.DT;
}

/*	Fx0A - LD Vx, K
	Wait for a key press, store the value of the key in Vx. */
void OP_Fx0A(struct Chip8Machine *machine)
{
	for (unsigned char i = 0x0; i <= 0xF; ++i)
	{
		if (machine->keypad[i])
		{
			machine->Registers.V[OPCODE_X] = i;
			return;
		}
	}

	machine->Registers.PC -= 2;
}

/* 	Fx15 - LD DT, Vx
	Set delay timer = Vx. */
void OP_Fx15(struct Chip8Machine *machine)
{
	machine->Registers.DT = machine->Registers.V[OPCODE_X];
}

/* 	Fx18 - LD ST, Vx
	Set sound timer = Vx. */
void OP_Fx18(struct Chip8Machine *machine)
{
	machine->Registers.ST = machine->Registers.V[OPCODE_X];
}

/*	Fx1E - ADD I, Vx
	Set I = I + Vx. */
void OP_Fx1E(struct Chip8Machine *machine)
{
	machine->Registers.I = machine->Registers.I + machine->Registers.V[OPCODE_X];
}

/*	Fx29 - LD F, Vx
	Set I = location of sprite for digit Vx. */
void OP_Fx29(struct Chip8Machine *machine)
{
	machine->Registers.I = CHARACTER_ADDRESS_OFFSET + machine->Registers.V[OPCODE_X] * CHARACTER_BYTES_LENGTH;
}

/*	Fx33 - LD B, Vx
	Store BCD representation of Vx in memory locations I, I+1, and I+2.
	The interpreter takes the decimal value of Vx, and places the hundreds digit in memory at location in I,
	the tens digit at location I+1, and the ones digit at location I+2. */

void OP_Fx33(struct Chip8Machine *machine)
{
	machine->memory[machine->Registers.I] = (machine->Registers.V[OPCODE_X] / 100) % 10;
	machine->memory[machine->Registers.I + 1] = (machine->Registers.V[OPCODE_X] / 10) % 10;
	machine->memory[machine->Registers.I + 2] = machine->Registers.V[OPCODE_X] % 10;
}

/*	Fx55 - LD [I], Vx
	Store registers V0 through Vx in memory starting at location I. */
void OP_Fx55(struct Chip8Machine *machine)
{
	for (unsigned char i = 0; i <= OPCODE_X; ++i)
	{
		machine->memory[machine->Registers.I + i] = machine->Registers.V[i];
//		machine->Registers.I += machine->Registers.V[i];
	}
}

/*	Fx65 - LD Vx, [I]
	Read registers V0 through Vx from memory starting at location I. */
void OP_Fx65(struct Chip8Machine *machine)
{
	for (unsigned char i = 0; i <= OPCODE_X; ++i)
	{
		machine->Registers.V[i] = machine->memory[machine->Registers.I + i];
//		machine->Registers.I += machine->memory[machine->Registers.I + i];
	}
}


/* --- UNIVERSAL INSTRUCTIONS --- */
/*	000n - PAGE n
	Sets the register IP to n. */
void UI_OP_000n(struct Chip8Machine *machine)
{
	machine->Registers.IP = OPCODE_N;
}

/* --- INSTRUCTION PAGE TWO --- */

/*	1xyn - MOP Vx, Vy, Vn
	Set Vn = value of display(Vx % #33, Vy % #65). */
void IP_2_OP_1xyn(struct Chip8Machine *machine)
{
	machine->Registers.V[OPCODE_N] = PIXEL_AT_XY;
}

/* 	2xyn - SEP Vx, Vy, Vn
	Set the pixel at display(Vx % #33, Vy % #65) = Vn. */

void IP_2_OP_2xyn(struct Chip8Machine *machine)
{
	PIXEL_AT_XY = machine->Registers.V[OPCODE_N];
}



// Private functions
/* Decodes OPCODE into corresponding instruction function and executes */
Errorable execute_instruction(struct Chip8Machine *machine)
{
	/* First, check for any universal instructions */
	// Edge-case: skip check if 0x0000
	if (machine->opcode == 0x0) goto skip_universal_instruction_check;

	switch (machine->opcode >> 12)
	{
		case 0x0:
			switch (machine->opcode >> 4)
			{
				case 0x0:
					UI_OP_000n(machine);
					goto successful_exit;
				default:
					break;
			}
			break;
		case 0x6:
			OP_6xkk(machine);
			break;
	}
	skip_universal_instruction_check:
	/* If none, goto the correct instruction page based on value in IP */
	switch (machine->Registers.IP)
	{
		case 1:
			goto instruction_page_one;
		case 2:
			goto instruction_page_two;
		default:
			break;
	}
	/* Instruction page TWO */
	instruction_page_two:
	switch (machine->opcode >> 12)
	{
		case 0x1:
			IP_2_OP_1xyn(machine);
			break;
		case 0x2:
			IP_2_OP_2xyn(machine);
			break;
	}
	goto successful_exit;

	instruction_page_one:
	/* Instruction page ONE*/
	switch (machine->opcode >> 12)
	{
		case 0x0:
			switch (machine->opcode)
			{
				case 0x00E0:
					OP_00E0(machine);
					break;
				case 0x00EE:
					OP_00EE(machine);
					break;
				default:
					break;
			}
			break;
		case 0x1:
			OP_1nnn(machine);
		        break;
		case 0x2:
			OP_2nnn(machine);
			break;
		case 0x3:
			OP_3xkk(machine);
			break;
		case 0x4:
			OP_4xkk(machine);
			break;
		case 0x5:
			OP_5xy0(machine);
			break;
		case 0x6:
			OP_6xkk(machine);
			break;
		case 0x7:
			OP_7xkk(machine);
			break;
		case 0x8:
			switch (machine->opcode & 0x000F)
			{
				case 0x0:
					OP_8xy0(machine);
					break;
				case 0x1:
					OP_8xy1(machine);
					break;
				case 0x2:
					OP_8xy2(machine);
					break;
				case 0x3:
					OP_8xy3(machine);
					break;
				case 0x4:
					OP_8xy4(machine);
					break;
				case 0x5:
					OP_8xy5(machine);
					break;
				case 0x6:
					OP_8xy6(machine);
					break;
				case 0x7:
					OP_8xy7(machine);
					break;
				case 0xE:
					OP_8xyE(machine);
					break;
				default:
					break;
			}
			break;
		case 0x9:
			OP_9xy0(machine);
			break;
		case 0xA:
			OP_Annn(machine);
			break;
		case 0xB:
			OP_Bnnn(machine);
			break;
		case 0xC:
			OP_Cxkk(machine);
			break;
		case 0xD:
			OP_Dxyn(machine);
			break;
		case 0xE:
			switch (machine->opcode & 0x00FF)
			{
				case 0x9E:
					OP_Ex9E(machine);
					break;
				case 0xA1:
					OP_ExA1(machine);
					break;
			}
			break;
		case 0xF:
			switch (machine->opcode & 0x00FF)
			{
				case 0x07:
					OP_Fx07(machine);
					break;
				case 0x0A:
					OP_Fx0A(machine);
					break;
				case 0x15:
					OP_Fx15(machine);
					break;
				case 0x18:
					OP_Fx18(machine);
					break;
				case 0x1E:
					OP_Fx1E(machine);
					break;
				case 0x29:
					OP_Fx29(machine);
					break;
				case 0x33:
					OP_Fx33(machine);
					break;
				case 0x55:
					OP_Fx55(machine);
					break;
				case 0x65:
					OP_Fx65(machine);
					break;
			}
			break;
	}
	goto successful_exit;

	successful_exit:
	return STATUS_OK;
}

// Global functions
/* ROM load function */
Errorable load_rom(struct Chip8Machine *machine, unsigned char* rom, size_t rom_size)
{
	if (rom_size > MAX_ROM_SIZE)
		return EXCESSIVE_ROM_SIZE;

	memcpy(machine->memory + START_ADDRESS_OFFSET, rom, rom_size * sizeof(char));
	return STATUS_OK;
}

/* Machine initialization function */
void init_machine(struct Chip8Machine *machine, unsigned char* rom, size_t rom_size)
{
	load_charset(machine);										// Load character set

	enum Error load_error = load_rom(machine, rom, rom_size);					// Load ROM and throw error on failure
	if (load_error != STATUS_OK)									// If error is detected
	{
		switch (load_error)
		{
			case INVALID_OPCODE:
#ifdef HAS_STDIO
				printf("Error! Invalid opcode: %x", machine->opcode);
#endif
				break;
		}
		exit((int)load_error);									// Exit with error code
	}
	machine->Registers.PC = START_ADDRESS_OFFSET;							// Move program counter to 0x200 (START_ADDRESS_OFFSET)
	srand(time(NULL));										// Seed the randomiser

	/* Fries-8 Specific */
	machine->Registers.IP = 1;									// Set instruction page to 1
}

/* Cycle function */
void do_cycle(struct Chip8Machine *machine)
{
	machine->opcode = (machine->memory[machine->Registers.PC] << 8) | machine->memory[machine->Registers.PC + 1];	// Fetch
	machine->Registers.PC += 2;												// Increment program counter
	enum Error execution_error = execute_instruction(machine);								// Decode and Execute, error on failure

	machine->random_byte = rand()%256;

#ifdef HAS_STDIO
	printf("%x\t%x\t", machine->opcode, machine->Registers.I);
//	for (size_t i = 0; i <= 0xF; ++i)
//		printf("V%x: %x\t", i, machine->Registers.V[i]);
	printf("%x\t", machine->Registers.SP);
	printf("%x\n", machine->Registers.PC);
#endif

	if (execution_error != STATUS_OK)											// If error is detected
	{
		switch (execution_error)
		{
			case INVALID_OPCODE:
#ifdef HAS_STDIO
				printf("Error! Invalid opcode: %x", machine->opcode);
#endif
				break;
		}
		exit((int)execution_error);											// Exit with error code
	}
	if (machine->Registers.DT > 0)
		--machine->Registers.DT;

	if (machine->Registers.ST > 0)
		--machine->Registers.ST;

}
