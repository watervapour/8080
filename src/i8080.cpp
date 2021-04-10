#include "i8080.h"
#include <stdio.h>
#include <cstdint>
#include <cstdlib>

i8080::i8080(){
	PC = 0;
	SP = 0x2FFF; // XXX need to tweak

	//register
	A = B = C = D = E = H = L = 0;
	flags.Z = flags.S = flags.P = flags.CY = flags.AC = 0;

	opcode = opcodeCycleCount = 0;	

	port1 = 0x10;
	port2 = 0x80;
}

i8080::~i8080(){

}

bool i8080::loadROM(const char* path){
	fputs("reading file!", stderr);
	fputs(path, stderr);

	FILE* rom = fopen(path, "rb");
	if(rom == NULL){
		fputs("file error", stderr);
		return false;
	}
	fseek(rom, 0, SEEK_END);
	long lSize = ftell(rom);
	rewind(rom);
	uint8_t* buffer = (uint8_t*)malloc(sizeof(uint8_t)*lSize);
	if(buffer == NULL){
		fputs("mem error", stderr);
		return false;
	}
	size_t result = fread(buffer, 1, lSize, rom);
	if(result != lSize){
		fputs("read err", stderr);
		return false;
	}
	if(0x2000 >= lSize){
		for(int i = 0; i < lSize; ++i){
			memory[i] = buffer[i];
		}
	} else {
		fputs("err: rom too big", stderr);
	}
	fclose(rom);
	free(buffer);
	return true;
}

void i8080::emulateCycle(){

	// XXX 1==1 to override the cycle count aspect
	if(opcodeCycleCount <= 0 | 1==1){
		opcode = memory[PC];
		printf("PC: %X | OP: %X\n",PC, opcode);
		(this->*opcodeArray[opcode])();
	}
	opcodeCycleCount -= 1;
}

/* returns byte (8 pixels of graphics)
@param index: the element within the gfx array to read
@return: a byte of scanline data
*/
uint8_t i8080::fetchGFXPixel(int index){
	index += 0x2400;
	if((index < 0x2400) | (index > 0x3FFF)){
		printf("Accesing GFX outside video RAM!\n");
		return 0;
	} else {
		return memory[index];
	}
}

/* calls printf, outputting:
	registers
	pc
	sp
	opcode
*/	
void i8080::printState(){
	printf("=== OPERATING AREA === \n");
	printf("PC:%X\n", PC);
	printf("SP:%X\n", SP);
	printf("opcode:%X\n", opcode);
	printf("opCycles:%X\n", opcodeCycleCount);

	printf("=== register: === \n");
	printf("reg A:%X\n", A);
	printf("reg B:%X\n", B);
	printf("reg C:%X\n", C);
	printf("reg D:%X\n", D);
	printf("reg E:%X\n", E);
	printf("reg H:%X\n", H);
	printf("reg L:%X\n", L);
	printf("reg PSW:%X\n", readPSW());

	printf("reg pair BC:%X\n", readRegisterPair('B'));
	printf("reg pair DE:%X\n", readRegisterPair('D'));
	printf("reg pair HL:%X\n", readRegisterPair('H'));
}

/* a condensed summary of the cpus actions
*/
void i8080::printStateShort(){
	printf("---- \n");
	printf("PC:%X\n", PC);
	printf("opcode:%X\n", opcode);
}

/* fetches a register value, for use in display
*/
uint8_t i8080::fetchReg(char reg){
	switch(reg){
	case('A'):
		return A;
	case('B'):
		return B;
	case('C'):
		return C;
	case('D'):
		return D;
	case('E'):
		return E;
	case('H'):
		return H;
	case('L'):
		return L;
	case('P'):
		return readPSW();
	case('O'):
		return opcode;
	}
}

/* fetches register pair, for display
*/
uint16_t i8080::fetchRegPair(char regPair){
	if(regPair == 'P'){
		return PC;
	} else if(regPair == 'S'){
		return SP;
	} else {
		return readRegisterPair(regPair);
	}
}


/* fetches value from RAM, is used for rendering ram contents*/
uint8_t i8080::fetchRAM(uint16_t address){
	if(address >= 0x2000 && address < 0x4000){
		return memory[address];
	} else { 
		return 255;
	}
}

/*
 ========== OPCODE UTILITY ==========
*/

uint8_t i8080::readPSW(){
	return (flags.Z << 7) | (flags.S << 6) | (flags.P << 5) | (flags.CY << 4) | (flags.AC << 3);
}

void i8080::writePSW(uint8_t data){
	flags.Z = data & 0x80;
	flags.S = data & 0x40;
	flags.P = data & 0x20;
	flags.CY = data & 0x10;
	flags.AC = data & 0x08;
}

/* returns 16bit value, which is formed from pairs of register
	ID: upper byte register, lower byte register
	B: BC
	D: DE
	M: HL
*/
uint16_t i8080::readRegisterPair(char pairID){
	uint8_t upperByte, lowerByte;//XXX issues with shifting 8 bit into bits XXXX0000?
	switch(pairID){
	
	case 'B':
		upperByte = B;
		lowerByte = C;
		break;
	case 'D':
		upperByte = D;
		lowerByte = E;
		break;
	case 'H':
		upperByte = H;
		lowerByte = L;
		break;

	}
	return ((upperByte << 8) | lowerByte);
}
/* stores two bytes of data into a register pair
	@param pairID: which register pair to store into 
		B: BC
		D: DE
		M: HL
*/
void i8080::writeRegisterPair(char pairID, uint16_t value){
	uint8_t lowerByte = (value & 0x00FF);
	uint8_t upperByte = (value & 0xFF00) >> 8;

	switch (pairID) {

	case 'B':
		B = upperByte;
		C = lowerByte;
		break;
	case 'D':
		D = upperByte;
		E = lowerByte;
		break;
	case 'H':
		H = upperByte;
		L = lowerByte;
		break;
	}
}


/*
	A 3 bit field, used to determine which register the opcode is operating on
	000: register B
	001: register C
	010: register D
	011: register E
	100: register H
	101: register L
	110: register M (a memory location determined by HL byte pair)
	111: register A (accumulator) 
*/
uint8_t* i8080::opcodeDecodeRegisterBits(uint8_t bits){
	switch (bits & 0x07) {

	case 0b000:
		return &B;
	case 0b001:
		return &C;
	case 0b010:
		return &D;
	case 0b011:
		return &E;
	case 0b100:
		return &H;
	case 0b101:
		return &L;
	case 0b110:
		//return &(memory[((H << 4) & L)]);
		return &(memory[readRegisterPair('H')]);
	case 0b111:
		return &A;
	default:
		return &A;
	}
}

// updates flags
// XXX AC unimplemented, not used in space invaders
void i8080::updateFlags(const uint8_t& value){
	printf("value = %X\n", value);
	flags.Z = (value == 0);	
	flags.S = (value & 0x80);
	flags.P = parityCheck(value);
	flags.AC = 0; 
}
// checks if there are an equal number of 1's and 0's 
// XXX does this need to check register pairs (non 1 byte) 
bool i8080::parityCheck(const uint8_t& reg){
	uint8_t ones = 0;
	for(int shift = 0; shift < 8; ++shift){
		if((reg >> shift) & 0x01){ ++ones; }
	}
	return (ones == 4);
}

// safely writes to RAM/GFX
void i8080::writeMem(uint16_t address, uint8_t data){
	// confine address to 0x2000 - 0x3FFF space, 0x4000- is RAM mirror
	// XXX check RAM mirror
	if(address >= 0x4000){
		address = address % 0x2000 + 0x2000;
	}
	if(address < 0x2000){
		printf("Illegal address write: %d\n", address);
	} else {
		memory[address] = data;
	}

}
/*
	========== OPCODES ==========
*/
// ===== GENERIC CODES =====

// generic LXI | loads immediate into register pair
void i8080::LXI(char pairID) {
	writeRegisterPair(pairID, ((memory[PC+2] << 8) | memory[PC+1]) );
	PC += 3;
	opcodeCycleCount = 10;
}

// generic INX | add one to register pair
void i8080::INX(char ID){
	uint16_t result = readRegisterPair(ID) + 1;
	writeRegisterPair(ID, result);
	PC += 1;
	opcodeCycleCount = 5;
}
// generic INR | adds one to a register, and alters Z, S, P, AC flags
void i8080::INR(uint8_t& reg){
	reg +=1;
	updateFlags(reg);	
	PC += 1;
	opcodeCycleCount = 5;
}

// generic DCR | adds one to a register, and alters Z, S, P, AC flags
void i8080::DCR(uint8_t& reg){
	reg -= 1;
	updateFlags(reg);	
	PC += 1;
	opcodeCycleCount = 5;
}

// generic MVI | sets a register to the byte after PC
void i8080::MVI(uint8_t& reg){
	reg = memory[PC+1];
	PC += 2;
	opcodeCycleCount = 7;
}

// generic POP | fetches data from the stack, at SP (SP points to top filled stack slot)
uint16_t  i8080::POP(){
	uint16_t value = (memory[SP + 1] << 8) | memory[SP];
	SP += 1;
	PC += 1;
	opcodeCycleCount = 10;
	return value;
}

//generic JUMP | grabs 2 bytes after PC and set the PC to that address
// XXX does auto pc increment mess with this?
// XXX issues with cyclecount for failed jumps should be handled here?
void i8080::JUMP(){
	uint16_t address = (memory[PC+2] << 8) | memory[PC+1];
	PC = address;
	opcodeCycleCount = 10;
}

// generic CALL | pushes PC, then sets PC
void i8080::genericCALL(bool condition){
	if(condition){
		PUSH(PC);
		PC = (memory[PC+2]) << 8 | memory[PC+1];
		//printf("GC caled: %X\n", PC);
		opcodeCycleCount = 17;
	} else {
		PC += 1;
		opcodeCycleCount = 11;
	}
}

// generic PUSH | places 2 bytes onto the stack, then decrements SP
void i8080::PUSH(uint16_t value){
	writeMem( (SP-1), (value & 0xFF00));
	writeMem( (SP-2), (value & 0x00FF));
	SP -= 2;
	PC += 1; 
	opcodeCycleCount = 11;
}

// generic RESET | behaves like call (saves pc to stack), but jumps to a specified number
//XXX pc auto issues?
void i8080::RESET(uint16_t callAddress){
	writeMem( (SP-1), (PC >> 8));
	writeMem( (SP-2), (PC));

	PC = callAddress;
	opcodeCycleCount = 11;
}

// generic RETURN | fetches a PC from the stack
void i8080::RETURN(bool condition){
	if(condition){
		PC = (memory[SP+1] << 8) | memory[SP];
		SP += 2;
		opcodeCycleCount = 11;
	} else {
		PC += 1;
		opcodeCycleCount = 5;
	}
}

// ===== OPCODES =====
// 0x00 |  No OPeration
void i8080::NOP(){
	PC += 1;
	opcodeCycleCount = 4;
}
// 0x01 | Load immediate register pair B & C
void i8080::LXIB(){
	LXI('B');
}

// 0x02 | Write A to mem at address BC 
void i8080::STAXB(){
	writeMem(readRegisterPair('B'), A);		
	opcodeCycleCount = 1;
}

// 0x03 | Increase the B register pair
void i8080::INXB(){
	INX('B');
}

// 0x04 | adds one to B and alters Z, S, P, AC flags
void i8080::INRB(){
	INR(B);
}

// 0x05 | subtracts one from B and alters Z, S, P, AC flags
void i8080::DCRB(){
	DCR(B);
}

// 0x06 | moves next byte into B register
void i8080::MVIB(){
	MVI(B);
}

// 0x07 | bit shift A left, bit 0 & Cy = prev bit 7
void i8080::RLC(){
	uint8_t bit0 = (A & 0x01);
	A <<= 1;
	A = A | bit0;
	flags.CY = (1 == (bit0 & 0x01));
	PC += 1;
	opcodeCycleCount = 4;
}

// 0x09 | adds BC onto HL
void i8080::DADB(){
	uint32_t result = readRegisterPair('H') + readRegisterPair('B');
	writeRegisterPair('H', result);
	flags.CY = (result & 0xFFFF0000) > 0;
	PC += 1;
 	opcodeCycleCount = 10;
}

// 0x0A | set register A to the contents or memory pointed by BC
void i8080::LDAXB(){
	A = memory[readRegisterPair('B')];
	PC += 1;
	opcodeCycleCount = 7;
}

// 0x0B | decrease B pair by 1
void i8080::DCXB(){
	uint16_t result = readRegisterPair('B') - 1;
	writeRegisterPair('B', result);
	PC += 1;
	opcodeCycleCount = 5;
}

// 0x0C | increases register C by 1, alters Z, S, P, AC flags
void i8080::INRC(){
	INR(C);
}

// 0x0D | decreases C by 1
void i8080::DCRC(){
	DCR(C);
}

// 0x0E | stores byte after PC into C
void i8080::MVIC(){
	MVI(C);
}

// 0x0F | rotates A right 1, bit 7 & CY = prev bit 7
void i8080::RRC(){
	uint8_t bit7 = (A & 0x80);
	A >>= 1;
	A = A | bit7;
	flags.CY = (1 == (bit7 & 0x80));
	PC += 1;
	opcodeCycleCount = 4;
}

// 0x10 | not an 8080 instruction, use as NOP

// 0x11 | writes the bytes after PC into the D register pair, E=pc+1, D=pc+2
void i8080::LXID(){
	LXI('D');
}

// 0x12 | stores A into the address pointed to by the D reg pair
// XXX no opcode count for stax
void i8080::STAXD(){
	writeMem(readRegisterPair('B'), A);		
	opcodeCycleCount = 1;
}

// 0x13 | increases the D reg pair by one
void i8080::INXD(){
	INX('D');
}

// 0x14 | increases the D register by one
void i8080::INRD(){
	INR(D);
}

// 0x15 | decreases register D by one
void i8080::DCRD(){
	DCR(D);
}

// 0x16 | set reg D to the byte following PC
void i8080::MVID(){
	MVI(D);
}

// 0x17 | shifts A Left 1, bit 0 = prev CY, CY = prev bit 7
// XXX ternary
void i8080::RAL(){
	uint8_t oldA = A;
	A <<= 1;
	A = A | ((flags.CY)? 0x01 : 0x00);
	flags.CY = (1 == (oldA & 0x80));
	PC += 1;
	opcodeCycleCount = 4;
}

// 0x18 | not an 8080 code, use as NOP

// 0x19 | add reg pair D onto reg pair H
void i8080::DADD(){
	uint32_t result = readRegisterPair('H') + readRegisterPair('D');
	writeRegisterPair('B', result);
	flags.CY = (result & 0xFFFF0000) > 0;
	PC += 1;
 	opcodeCycleCount = 10;
}

// 0x1A | store the value at the memory referenced by DE in A
void i8080::LDAXD(){
	A = memory[readRegisterPair('D')];
	PC += 1;
	opcodeCycleCount = 7;
}

// 0x1B | decrease DE pair by 1
void i8080::DCXD(){
	uint16_t result = readRegisterPair('D') - 1;
	writeRegisterPair('D', result);
	PC += 1;
	opcodeCycleCount = 5;
}

// 0x1C | increase reg E by 1
void i8080::INRE(){
	INR(E);
}

// 0x1D | decrease reg E by 1
void i8080::DCRE(){
	DCR(E);
}

// 0x1E | move byte after PC into reg E
void i8080::MVIE(){
	MVI(E);
}

// 0x1F | rotate A right one, bit 7 = prev bit 7, CY = prevbit0
void i8080::RAR(){
	uint8_t oldA = A;
	A >>= 1;
	A = A | (oldA & 0x80);
	flags.CY = (1 == (oldA & 0x01));
	PC += 1;
	opcodeCycleCount = 4;
}

// 0x20 | not an 8080 code, use as NOP

// 0x21 | loads data after PC into H pair
void i8080::LXIH(){
	LXI('H');
}

// 0x22 | stores HL into adress listed after PC
// adr <- L , adr+1 <- H
void i8080::SHLD(){
	uint16_t address = (memory[PC+1] << 8) | memory[PC+2];
	writeMem(address, L);
	writeMem(address + 1, H);
	PC += 3;
	opcodeCycleCount = 16;
}

// 0x23 | Increase the H register pair
void i8080::INXH(){
	INX('H');
}

// 0x24 | adds one to H and alters Z, S, P, AC flags
void i8080::INRH(){
	INR(H);
}

// 0x25 | subtracts one from H and alters Z, S, P, AC flags
void i8080::DCRH(){
	DCR(H);
}

// 0x26 | moves next byte into H register
void i8080::MVIH(){
	MVI(H);
}

// 0x27 | DAA // XXX need to do
void i8080::DAA(){
	printf("DAA 0x27 not implemented!\n");
	PC += 1;
	opcodeCycleCount = 1;
}

// 0x28 | not 8080 code use as NOP

// 0x29 | add HL onto HL
void i8080::DADH(){
	uint32_t result = readRegisterPair('H') + readRegisterPair('H');
	writeRegisterPair('H', result);
	flags.CY = (result & 0xFFFF0000) > 0;
	PC += 1;
 	opcodeCycleCount = 10;
}

// 0x2A | load 2bytes after PC into HL
// L <- adr, H <- adr+1
void i8080::LHLD(){
	writeRegisterPair('H', ((memory[PC+2] << 8) | memory[PC+1]) );
	PC += 3;
	opcodeCycleCount = 16;
}

// 0x2B | decrease HL by 1
void i8080::DCXH(){
	uint16_t result = readRegisterPair('H') - 1;
	writeRegisterPair('H', result);
	PC += 1;
	opcodeCycleCount = 5;
}

// 0x2C | adds one to L and alters Z, S, P, AC flags
void i8080::INRL(){
	INR(L);
}

// 0x2D | subtracts one from L and alters Z, S, P, AC flags
void i8080::DCRL(){
	DCR(L);
}

// 0x2E | moves next byte into L register
void i8080::MVIL(){
	MVI(L);
}

// 0x2F | invert A
void i8080::CMA(){
	A = ~A;
	PC += 1;
	opcodeCycleCount = 1;
}

// 0x30 | unimplemented in 8080, treat as NOP

// 0x31 | load data after PC as new Stack pointer
// sp.lo = pc+1, sp.hi = pc+2
// XXX alter generic LXI to be usable with SP?
void i8080::LXISP(){
	SP = (memory[PC+2] << 8) | memory[PC+1];
	PC += 3;
	opcodeCycleCount = 10;
}

// 0x32 | stores A into the address from bytes after PC
// adr.low = PC+1, adr.hi = PC+2
void i8080::STA(){
	uint16_t address = (memory[PC+2] << 8) | memory[PC+1];
	writeMem(address, A);
	PC += 3;
	opcodeCycleCount = 19;
}

// 0x33 | add one to SP
void i8080::INXSP(){
	SP += 1;
	PC += 1;
	opcodeCycleCount += 5;
}

// 0x34 | increase the value pointed to by HL
void i8080::INRM(){
	uint16_t address = readRegisterPair('H');
	uint8_t newValue = memory[address] + 1;
	writeMem(address, newValue);
	updateFlags(newValue);
	PC += 1;
	opcodeCycleCount = 10;
}

// 0x35 | decrease tge value pointed to by HL
void i8080::DCRM(){
	uint16_t address = readRegisterPair('H');
	uint8_t newValue = memory[address] - 1;
	writeMem(address, newValue);
	updateFlags(newValue);
	PC += 1;
	opcodeCycleCount = 10;
}

// 0x36 | move the byte after PC into memory pointed to by HL
void i8080::MVIM(){
	writeMem(readRegisterPair('H'), memory[PC+1]);
	PC += 2;
	opcodeCycleCount = 10;
}

// 0x37 | set carry flag to 1
void i8080::STC(){
	flags.CY = true;
	PC += 1;
	opcodeCycleCount = 1;
}

// 0x38 | unimplemented in 8080, use as alt NOP

// 0x39 | add SP onto HL
void i8080::DADSP(){
	uint32_t result = readRegisterPair('H') + SP;
	writeRegisterPair('H', result);
	flags.CY = (result & 0xFFFF0000) > 0;
	PC += 1;
 	opcodeCycleCount = 10;
}

// 0x3A | set reg A to the value pointed by bytes after PC
void i8080::LDA(){
	A = memory[((memory[PC+2] << 8) | memory[PC+1])];
	PC += 3;
	opcodeCycleCount = 13;
}

// 0x3B | decrease SP by 1
void i8080::DCXSP(){
	SP -= 1;
	PC += 1;
	opcodeCycleCount = 1;
}

// 0x3C | adds one to A and alters Z, S, P, AC flags
void i8080::INRA(){
	INR(A);
}

// 0x3D | subtracts one from A and alters Z, S, P, AC flags
void i8080::DCRA(){
	DCR(A);
}

// 0x3E | moves next byte into A register
void i8080::MVIA(){
	MVI(A);
}

// 0x3F | invert carry flag
void i8080::CMC(){
	flags.CY = !flags.CY;
	PC += 1;
	opcodeCycleCount = 1;
}

// (0x40 - 0x74), (0x77 - 0x7F) | MOV
void i8080::MOV(){
	uint8_t dest = (opcode & 0b00111000) >> 3;
	uint8_t source = (opcode & 0b00000111);

	// MOV's involving Mem use 7, not 5
	opcodeCycleCount = 7;
	if(dest == 0b110) { // if dest is mem
		//printf("Mov reg => mem");
		writeMem(readRegisterPair('H'), *opcodeDecodeRegisterBits(source));  
	} else if(source == 0b110){ // if source is mem
		printf("Mov mem => reg");
		writeMem(*opcodeDecodeRegisterBits(dest), readRegisterPair('H'));
	} else { // dest & source are normal registers
		//printf("Mov %X reg %d => %X reg %d", *opcodeDecodeRegisterBits(dest), *opcodeDecodeRegisterBits(source), opcodeDecodeRegisterBits(dest), opcodeDecodeRegisterBits(source));
		//writeMem(*opcodeDecodeRegisterBits(dest), *opcodeDecodeRegisterBits(source));
		*opcodeDecodeRegisterBits(dest) = *opcodeDecodeRegisterBits(source);
		opcodeCycleCount = 5;
	}

	PC += 1;
}

// 0x76 | XXX not done yet
void i8080::HLT(){
	printf("HLT not implemented yet!\n");
	PC += 1;
	opcodeCycleCount = 1;
}

// (0x80 - 0x87) | Add a register/mem value onto A, and update all flags
void i8080::ADD(){
	uint8_t sourceAddress = (opcode & 0b00000111);
	uint8_t increment = 0;
	if(sourceAddress == 0b110){ //sourcing from mem pointed by HL
		increment = memory[readRegisterPair('H')];	
		opcodeCycleCount = 7;
	} else { // not sourcing from mem
		increment = *opcodeDecodeRegisterBits(sourceAddress); //XXX *odrp()?
		opcodeCycleCount = 4;
	}
	uint16_t sum = A + increment;
	A = (sum & 0xFF);
	flags.CY = (sum & 0xFF00) > 0;
	updateFlags(A);
	PC += 1;
}

// (0x88 - 0x8F) | Add a register/mem value + carry bit onto A, update registers
void i8080::ADC(){
	uint8_t sourceAddress = (opcode & 0b00000111);
	uint8_t increment = 0;
	if(sourceAddress == 0b110){ //sourcing from mem pointed by HL
		increment = memory[readRegisterPair('H')];	
		opcodeCycleCount = 7;
	} else { // not sourcing from mem
		increment = *opcodeDecodeRegisterBits(sourceAddress);
		opcodeCycleCount = 4;
	}
	uint16_t sum = A + increment + (uint8_t)flags.CY;
	A = (sum & 0xFF);
	flags.CY = (sum & 0xFF00) > 0;
	updateFlags(A);
	PC += 1;
}

// (0x90 - 0x97) | Sub a register/mem value from A, and update all flags
void i8080::SUB(){
	uint8_t sourceAddress = (opcode & 0b00000111);
	uint8_t decrement = 0;
	if(sourceAddress == 0b110){ //sourcing from mem pointed by HL
		decrement = memory[readRegisterPair('H')];	
		opcodeCycleCount = 7;
	} else { // not sourcing from mem
		decrement = *opcodeDecodeRegisterBits(sourceAddress);
		opcodeCycleCount = 4;
	}
	uint16_t sum = A - decrement;
	A = (sum & 0xFF);
	flags.CY = (sum & 0xFF00) > 0;
	updateFlags(A);
	PC += 1;
}

// (0x98 - 0x9F) | Add a register/mem value - carry bit onto A, update registers
void i8080::SBB(){
	uint8_t sourceAddress = (opcode & 0b00000111);
	uint8_t decrement = 0;
	if(sourceAddress == 0b110){ //sourcing from mem pointed by HL
		decrement = memory[readRegisterPair('H')];	
		opcodeCycleCount = 7;
	} else { // not sourcing from mem
		decrement = *opcodeDecodeRegisterBits(sourceAddress);
		opcodeCycleCount = 4;
	}
	uint16_t sum = A - decrement - (uint8_t)flags.CY;
	A = (sum & 0xFF);
	flags.CY = (sum & 0xFF00) > 0;
	updateFlags(A);
	PC += 1;
}

// (0xA0 - 0xA7) | Bitwise AND a register/mem value with A, update registers
void i8080::ANA(){
	uint8_t sourceAddress = (opcode & 0b00000111);
	uint8_t modifier = 0;
	if(sourceAddress == 0b110){ //sourcing from mem pointed by HL
		modifier = memory[readRegisterPair('H')];	
		opcodeCycleCount = 7;
	} else { // not sourcing from mem
		modifier = *opcodeDecodeRegisterBits(sourceAddress);
		opcodeCycleCount = 4;
	}
	uint16_t result = A & modifier;
	A = (result & 0xFF);
	flags.CY = (result & 0xFF00) > 0;
	updateFlags(A);
	PC += 1;
}

// (0xA8 - 0xAF) | Bitwise XOR a register/mem value with A, update registers
void i8080::XRA(){
	uint8_t sourceAddress = (opcode & 0b00000111);
	uint8_t modifier = 0;
	if(sourceAddress == 0b110){ //sourcing from mem pointed by HL
		modifier = memory[readRegisterPair('H')];	
		opcodeCycleCount = 7;
	} else { // not sourcing from mem
		modifier = *opcodeDecodeRegisterBits(sourceAddress);
		opcodeCycleCount = 4;
	}
	uint16_t result = A ^ modifier;
	A = (result & 0xFF);
	flags.CY = (result & 0xFF00) > 0;
	updateFlags(A);
	PC += 1;
}

// (0xB0 - 0xB7) | Bitwise OR a register/mem value with A, update registers
void i8080::ORA(){
	uint8_t sourceAddress = (opcode & 0b00000111);
	uint8_t modifier = 0;
	if(sourceAddress == 0b110){ //sourcing from mem pointed by HL
		modifier = memory[readRegisterPair('H')];	
		opcodeCycleCount = 7;
	} else { // not sourcing from mem
		modifier = *opcodeDecodeRegisterBits(sourceAddress);
		opcodeCycleCount = 4;
	}
	uint16_t result = A | modifier;
	A = (result & 0xFF);
	flags.CY = (result & 0xFF00) > 0;
	updateFlags(A);
	PC += 1;
}

// (0xB8 - 0xBF) | Compare a register/mem value with A, update registers
void i8080::CMP(){
	uint8_t sourceAddress = (opcode & 0b00000111);
	uint8_t modifier = 0;
	if(sourceAddress == 0b110){ //sourcing from mem pointed by HL
		modifier = memory[readRegisterPair('H')];	
		opcodeCycleCount = 7;
	} else { // not sourcing from mem
		modifier = *opcodeDecodeRegisterBits(sourceAddress);
		opcodeCycleCount = 4;
	}
	uint16_t result = A - modifier;
	flags.CY = (result & 0xFF00) > 0;
	updateFlags(result);
	PC += 1;
}

// 0xC0 | return on nonzero
// XXX do i needa mess with PC auto changing?
void i8080::RNZ(){
	RETURN((flags.Z == false));
	opcodeCycleCount = 1;
}

// 0xC1 | pull BC pair from stack
void i8080::POPB(){
	writeRegisterPair('B', POP());
	PC += 1;
	opcodeCycleCount = 1;
}

// 0xC2 | jump to address from after PC if non zero
void i8080::JNZ(){
	printf("flags.Z = %s\n", (flags.Z ? "true" : "false"));
	if(flags.Z == false){
		JUMP();
	} else {
		PC += 3;
		opcodeCycleCount = 1;
	}
}

// 0xC3 | normal jump
void i8080::JMP(){
	JUMP();
}

// 0xC4 | run call if non zero
void i8080::CNZ(){
	genericCALL((flags.Z == false));
}

// 0xC5 | store BC pair on stack
void i8080::PUSHB(){
	PUSH(readRegisterPair('B'));
}

// 0xC6 | adds a byte onto A, fetched after PC
void i8080::ADI(){
	uint16_t sum = A + memory[PC+1];
	A = (sum & 0xFF);
	flags.CY = (sum & 0xFF00) > 0;
	updateFlags(A);
	PC += 1;	
	opcodeCycleCount = 7;
}

// 0xC7 | calls to $0 
void i8080::RST0(){
	RESET(0);
}

// 0xC8 | return if Z
void i8080::RZ(){
	RETURN((flags.Z));
}

// 0xC9 | standard return
void i8080::RET(){
	RETURN(true);
}

// 0xCA | 
void i8080::JZ(){
	if(flags.Z){
		JUMP();
	}
	PC += 1;
	opcodeCycleCount = 1;
}

// 0xCB | not 8080 code, treat as NOP

// 0xCC | call if Z flag set
void i8080::CZ(){
	genericCALL(flags.Z);
}

// 0xCD | store PC on stack, and jump to a new location
// sp-1   = pc.hi | sp-2  = pc.lo
// pc.low = pc +1 | pc.hi = pc+2
// XXX pc auto
void i8080::CALL(){
	uint16_t ret = PC + 3;
	writeMem( (SP-1), (ret>> 8));
	writeMem( (SP-2), (ret));
	SP -= 2;
	PC = (memory[PC+2] << 8) | memory[PC+1];
	opcodeCycleCount = 17;
}

// 0xCE | add carry bit and Byte onto A
void i8080::ACI(){
	uint16_t sum = A + memory[PC+1] + (uint8_t)flags.CY;
	A = (sum & 0xFF);
	flags.CY = (sum & 0xFF00) > 0;
	updateFlags(A);
	PC += 1;	
	opcodeCycleCount = 7;
}

// 0xCF | resets to $8
void i8080::RST1(){
	RESET(8);
}

// 0xD0 | return if Cy = 0
void i8080::RNC(){
	RETURN(flags.CY == false);
}

// 0xD1 | fetch data from the stack, and store in DE pair
void i8080::POPD(){
	writeRegisterPair('D', POP());
	PC += 1;
	opcodeCycleCount = 1;
}

// 0xD2 | jump when carry = 0
void i8080::JNC(){
	if(flags.CY == false){
		JUMP();
	} else {
		PC += 1;
		opcodeCycleCount = 10;
	}
}

// 0xD3 | XXX special
void i8080::OUT(){
	printf("OUT unimplemented!\n");
	PC += 1;
	opcodeCycleCount = 1;
}

// 0xD4 | call on carry = 0false
void i8080::CNC(){
	genericCALL((flags.CY == false));
}

// 0xD5 | store DE pair onto stack
void i8080::PUSHD(){
	PUSH(readRegisterPair('D'));
}

// 0xD6 | subtract a byte from A
void i8080::SUI(){
	uint16_t result = A - memory[PC+1];
	A = (result & 0xFF);
	flags.CY = (result & 0xFF00) > 0;
	updateFlags(A);
	PC += 2;	
	opcodeCycleCount = 7;
}

// 0xD7 | reset to $10
void i8080::RST2(){
	RESET(0x10);
}

// 0xD8 | return if CY = 1
void i8080::RC(){
	RETURN(flags.CY);	
}

// 0xD9 | not an 8080 code

// 0xDA | jump if cy = 1
void i8080::JC(){
	if(flags.CY){
		JUMP();
	} else {
		PC += 1;
		opcodeCycleCount = 1;
	}	
}

// 0xDB | XXX SPECIAL
void i8080::IN(){
	printf("IN unimplemented!\n");
	PC += 1;
	opcodeCycleCount = 1;
}

// 0xDC | call if cy =1
void i8080::CC(){
	genericCALL(flags.CY);
}

// 0xDD | not 8080 code

// 0xDE | sub byte and cy from A
void i8080::SBI(){
	uint16_t sum = A - memory[PC+1] - (uint8_t)flags.CY;
	A = (sum & 0xFF);
	flags.CY = (sum & 0xFF00) > 0;
	updateFlags(A);
	PC += 2;	
	opcodeCycleCount = 7;
}

// 0xDF | reset to $18
void i8080::RST3(){
	RESET(0x18);
}

// 0xE0 | return if P = O
void i8080::RPO(){
	RETURN(flags.P == 0);
}

// 0xE1 | store values from stack in HL pair
void i8080::POPH(){
	writeRegisterPair('H', POP());
	PC += 1;
	opcodeCycleCount = 1;
}

// 0xE2 | jump if p = 0
void i8080::JPO(){
	if(flags.P){
		JUMP();
	} else {
		PC += 1;
		opcodeCycleCount = 1;
	}
}

// 0xE3 | exchange HL and SP
// L <-> SP | H <-> SP+1
void i8080::XTHL(){
	uint16_t oldSP = POP();
	SP = (H << 8) | L;
	writeRegisterPair('H', oldSP);
	PC += 1;
	opcodeCycleCount = 18;
} 

// 0xE4 | call if p = 0
void i8080::CPO(){
	genericCALL(flags.P);
}

// 0xE5 | store HL on the stack
void i8080::PUSHH(){
	PUSH(readRegisterPair('H'));
}

// 0xE6 | bitwise AND byte with A
void i8080::ANI(){
	uint16_t sum = A & memory[PC+1];
	A = (sum & 0xFF);
	flags.CY = (sum & 0xFF00) > 0;
	updateFlags(A);
	PC += 2;	
	opcodeCycleCount = 7;
}

// 0xE7 | reset to $20
void i8080::RST4(){
	RESET(0x20);
}

// 0xE8 | return if p = 1
void i8080::RPE(){
	RETURN(flags.P);
}

// 0xE9 | set pc to HL
void i8080::PCHL(){
	PC = readRegisterPair('H');
}

// 0xEA | jump if p = 1
void i8080::JPE(){
	if(flags.P){
		JUMP();
	} else {
		PC += 1;
		opcodeCycleCount = 1;
	}
}

// 0xEB | exchange HL and DE
void i8080::XCHG(){
	uint16_t oldDE = readRegisterPair('D');
	writeRegisterPair('D', (H << 8) | L);
	writeRegisterPair('H', oldDE);
	PC += 1;
	opcodeCycleCount = 4;
}

// 0xEC | call if p = 1
void i8080::CPE(){
	genericCALL(flags.P);
}

// 0xED | not 8080 code

// 0xEE | XOR A with a byte
void i8080::XRI(){
	uint16_t sum = A ^ memory[PC+1];
	A = (sum & 0xFF);
	flags.CY = (sum & 0xFF00) > 0;
	updateFlags(A);
	PC += 2;	
	opcodeCycleCount = 7;
}

// 0xEF | reset to $28
void i8080::RST5(){
	RESET(0x28);
}

// 0xF0 | return if positive
void i8080::RP(){
	RETURN(flags.S == false);
}

// 0xF1 | pops flags and A off of stack
void i8080::POPPSW(){
	uint16_t data = POP();
	writePSW(data & 0x00FF);
	A = (data >> 8);
	PC += 1;
	opcodeCycleCount = 10;
}

// 0xF2 | jump if positive
void i8080::JP(){
	if(flags.S == false){
		JUMP();
	} else {
		PC += 1;
		opcodeCycleCount = 1;
	}
}

// 0xF3 | XXX special
void i8080::DI(){
	printf("DI unimplemented!\n");
	PC += 1;
	opcodeCycleCount = 1;
}

// 0xF4 | call if positive
void i8080::CP(){
	genericCALL(flags.S == false);
}

// 0xF5 | store PSW and A in the stack (PSW as low byte)
void i8080::PUSHPSW(){
	uint16_t data  = (A << 8) | readPSW();
	PUSH(data);
}

// 0xF6 | biwise OR A with a byte
void i8080::ORI(){
	uint16_t sum = A | memory[PC+1];
	A = (sum & 0xFF);
	flags.CY = (sum & 0xFF00) > 0;
	updateFlags(A);
	PC += 2;	
	opcodeCycleCount = 7;
}

// 0xF7 | rest to $30
void i8080::RST6(){
	RESET(0x30);
}

// 0xF8 | return if minus
void i8080::RM(){
	RETURN(flags.S == true);
}

// 0xF9 | sets SP to HL
void i8080::SPHL(){
	SP = readRegisterPair('H');
	PC += 1;
	opcodeCycleCount = 5;
}

// 0xFA | jump if minus
void i8080::JM(){
	if(flags.S == true){
		JUMP();
	} else {
		PC += 1;
		opcodeCycleCount = 10;
	}
}

// 0xFB | XXX special
void i8080::EI(){
	printf("EI not implemented yet!\n");
	PC += 1;
	opcodeCycleCount = 1;
}

// 0xFC | call if minus
void i8080::CM(){
	genericCALL(flags.S == true);
}

// 0xFD | not 8080 code

// 0xFE | compare A to byte
void i8080::CPI(){
	uint8_t sum = A - memory[PC+1];
	//uint16_t sum = A - memory[PC+1];
	printf("A: %X - byte: %X  sum = %X\n",A, memory[PC+1],  sum);
	//A = (sum & 0xFF);
	flags.CY = sum < memory[PC+1];
	updateFlags(sum);
	PC += 2;	
	opcodeCycleCount = 7;
}

// 0xFF | reset to $38
void i8080::RST7(){
	RESET(0x38);
}
