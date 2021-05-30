#ifndef i8080_h
#define i8080_h

#include <cstdint>

class i8080 {
private:
	// Routine registers
	uint16_t SP, PC;

	// Registers
	uint8_t A, B, C, D, E, H, L;
	struct PSW{
		bool Z;
		bool S;
		bool P;
		bool CY;
		bool AC;
	};
	PSW flags;
	uint8_t readPSW();
	void writePSW(uint8_t);

	bool hold = false;
	//interrupt enable bit
	bool INTE = true;
	// interrupt
	bool INT = false;
	void signalInt();	
	// IO
	uint8_t dataBus = 0;
	uint16_t bitShifter = 0;
	uint8_t bitShifterAmount = 0;

	// XXX move out of 8080 implementation? so it is portable/reusable for 8085 etc?
	// XXX had made 16bit
	uint8_t memory[0x8000] = {false};


	uint8_t opcode;
	uint8_t opcodeCycleCount;


	uint16_t readRegisterPair(char);
	void writeRegisterPair(char, uint16_t);

	// returns pointer to a register variable from 'DDD'/'SSS' bits in opcode
	uint8_t* opcodeDecodeRegisterBits(uint8_t);

	void updateFlags(const uint8_t&);

	void writeMem(uint16_t, uint8_t);
	// ===== GENERIC CODES =====
	void LXI(char); void INX(char); void INR(uint8_t&); void DCR(uint8_t&); void MVI(uint8_t&);void DAD(char);void DCX(char);
	uint16_t POP(); void JUMP(bool); void genericCALL(bool); void PUSH(uint16_t); void RESET(uint16_t);
	void RETURN(bool);

	// opcode functions
	// 0x00, 0x01, 0x02, 0x03, 0x04 
	void NOP();	void LXIB(); void STAXB(); void INXB(); void INRB();
	// 0x05, 0x06, 0x07, 0x08, 0x09
	void DCRB(); void MVIB(); void RLC(); /*NOP*/ void DADB();
	// 0x0A, 0x0B, 0x0C, 0x0D, 0x0E
	void LDAXB(); void DCXB(); void INRC();	void DCRC(); void MVIC();
	// 0x0F, 0x10, 0x11, 0x12, 0x13
	void RRC(); /*NOP*/ void LXID(); void STAXD(); void INXD();
	// 0x14, 0x15, 0x16, 0x17, 0x18
	void INRD(); void DCRD(); void MVID(); void RAL(); /*NOP*/ 

	// 0x19, 0x1A, 0x1B, 0x1C, 0x1D
	void DADD(); void LDAXD(); void DCXD();	void INRE(); void DCRE();
	// 0x1E, 0x1F, 0x20, 0x21, 0x22
	void MVIE(); void RAR(); /*NOP*/ void LXIH(); void SHLD();
	// 0x23, 0x24, 0x25, 0x26, 0x27
	void INXH(); void INRH(); void DCRH(); void MVIH(); void DAA();
	// 0x28, 0x29, 0x2A, 0x2B, 0x2C
	/*NOP*/ void DADH(); void LHLD(); void DCXH(); void INRL();
	// 0x2D, 0x2E, 0x2F, 0x30, 0x31
	void DCRL(); void MVIL(); void CMA(); /*NOP*/ void LXISP();

	// 0x32, 0x33, 0x34, 0x35, 0x36
	void STA(); void INXSP(); void INRM(); void DCRM(); void MVIM();
	// 0x37, 0x38, 0x39, 0x3A, 0x3B
	void STC(); /*NOP*/ void DADSP(); void LDA(); void DCXSP();
	// 0x3C, 0x3D, 0x3E, 0x3F 
	void INRA(); void DCRA(); void MVIA(); void CMC();	

	// (0x40 - 75, 0x77 - 0x7F), 0x76, (0x80 - 0x87), (0x88 - 0x8f), (0x90 - 0x97),
	void MOV(); void HLT();	void ADD(); void ADC(); void SUB();
	// (0x98 - 0x9f), (0xA0 - 0xA7), (0xA8 - 0xAF), (0xB0 - 0xB7), (0xB8 - 0xBF)
 	void SBB(); void ANA(); void XRA(); void ORA(); void CMP();

	// 0xC0, 0xC1, 0xC2, 0xC3, 0xC4
	void RNZ(); void POPB(); void JNZ(); void JMP(); void CNZ();
	// 0xC5, 0xC6, 0xC7, 0xC8, 0xC9
	void PUSHB(); void ADI(); void RST0(); void RZ(); void RET();
	// 0xCA, 0xCB, 0xCC, 0xCD, 0xCE
	void JZ(); /*NOP*/ void CZ(); void CALL(); void ACI();
	// 0xCF, 0xD0, 0xD1, 0xD2, 0xD3
	void RST1(); void RNC(); void POPD(); void JNC(); void OUT();
	// 0xD4, 0xD5, 0xD6, 0xD7, 0xD8
	void CNC(); void PUSHD(); void SUI(); void RST2(); void RC();
	
	// 0xD9, 0xDA, 0xDB, 0xDC, 0xDD
	/*NOP*/ void JC(); void IN(); void CC(); /*NOP*/
	// 0xDE, 0xDF, 0xE0, 0xE1, 0xE2
	void SBI(); void RST3(); void RPO(); void POPH(); void JPO();
	// 0xE3, 0xE4, 0xE5, 0xE6, 0xE7
	void XTHL(); void CPO(); void PUSHH(); void ANI(); void RST4();
	// 0xE8, 0xE9, 0xEA, 0xEB, 0xEC
	void RPE(); void PCHL(); void JPE(); void XCHG(); void CPE();
	// 0xED, 0xEE, 0xEF, 0xF0, 0xF1
	/*NOP*/ void XRI(); void RST5(); void RP(); void POPPSW();
	
	// 0xF2, 0xF3, 0xF4, 0xF5, 0xF6
	void JP(); void DI(); void CP(); void PUSHPSW(); void ORI();
	// 0xF7, 0xF8, 0xF9, 0xFA, 0xFB
	void RST6(); void RM(); void SPHL(); void JM(); void EI();
	// 0xFC, 0xFD, 0xFE, 0xFF
	void CM(); /*NOP*/ void CPI(); void RST7();

	void (i8080::*opcodeArray[256])()={
			&i8080::NOP, &i8080::LXIB, &i8080::STAXB, &i8080::INXB, &i8080::INRB, &i8080::DCRB, &i8080::MVIB, &i8080::RLC, &i8080::NOP, &i8080::DADB, &i8080::LDAXB, &i8080::DCXB, &i8080::INRC, &i8080::DCRC, &i8080::MVIC, &i8080::RRC,
			&i8080::NOP, &i8080::LXID, &i8080::STAXD, &i8080::INXD, &i8080::INRD, &i8080::DCRD, &i8080::MVID, &i8080::RAL, &i8080::NOP, &i8080::DADD, &i8080::LDAXD, &i8080::DCXD, &i8080::INRE, &i8080::DCRE, &i8080::MVIE, &i8080::RAR,
			&i8080::NOP, &i8080::LXIH, &i8080::SHLD, &i8080::INXH, &i8080::INRH, &i8080::DCRH, &i8080::MVIH, &i8080::DAA, &i8080::NOP, &i8080::DADH, &i8080::LHLD, &i8080::DCXH, &i8080::INRL, &i8080::DCRL, &i8080::MVIL, &i8080::CMA,
			&i8080::NOP, &i8080::LXISP, &i8080::STA, &i8080::INXSP, &i8080::INRM, &i8080::DCRM, &i8080::MVIM, &i8080::STC, &i8080::NOP, &i8080::DADSP, &i8080::LDA, &i8080::DCXSP, &i8080::INRA, &i8080::DCRA, &i8080::MVIA, &i8080::CMC,
			&i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, 	
			&i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, 
			&i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, 
			&i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::HLT, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, &i8080::MOV, 
			&i8080::ADD, &i8080::ADD, &i8080::ADD, &i8080::ADD, &i8080::ADD, &i8080::ADD, &i8080::ADD, &i8080::ADD, &i8080::ADC, &i8080::ADC, &i8080::ADC, &i8080::ADC, &i8080::ADC, &i8080::ADC, &i8080::ADC, &i8080::ADC, 
			&i8080::SUB, &i8080::SUB, &i8080::SUB, &i8080::SUB, &i8080::SUB, &i8080::SUB, &i8080::SUB, &i8080::SUB, &i8080::SBB, &i8080::SBB, &i8080::SBB, &i8080::SBB, &i8080::SBB, &i8080::SBB, &i8080::SBB, &i8080::SBB, 
			&i8080::ANA, &i8080::ANA, &i8080::ANA, &i8080::ANA, &i8080::ANA, &i8080::ANA, &i8080::ANA, &i8080::ANA, &i8080::XRA, &i8080::XRA, &i8080::XRA, &i8080::XRA, &i8080::XRA, &i8080::XRA, &i8080::XRA, &i8080::XRA,  
			&i8080::ORA, &i8080::ORA, &i8080::ORA, &i8080::ORA, &i8080::ORA, &i8080::ORA, &i8080::ORA, &i8080::ORA, &i8080::CMP, &i8080::CMP, &i8080::CMP, &i8080::CMP, &i8080::CMP, &i8080::CMP, &i8080::CMP, &i8080::CMP,  
			&i8080::RNZ, &i8080::POPB, &i8080::JNZ, &i8080::JMP, &i8080::CNZ, &i8080::PUSHB, &i8080::ADI, &i8080::RST0, &i8080::RZ, &i8080::RET, &i8080::JZ, &i8080::NOP, &i8080::CZ, &i8080::CALL, &i8080::ACI, &i8080::RST1,
			&i8080::RNC, &i8080::POPD, &i8080::JNC, &i8080::OUT, &i8080::CNC, &i8080::PUSHD, &i8080::SUI, &i8080::RST2, &i8080::RC, &i8080::NOP, &i8080::JC, &i8080::IN, &i8080::CC, &i8080::NOP, &i8080::SBI, &i8080::RST3,
			&i8080::RPO, &i8080::POPH, &i8080::JPO, &i8080::XTHL, &i8080::CPO, &i8080::PUSHH, &i8080::ANI, &i8080::RST4, &i8080::RPE, &i8080::PCHL, &i8080::JPE, &i8080::XCHG, &i8080::CPE, &i8080::NOP, &i8080::XRI, &i8080::RST5,
			&i8080::RP, &i8080::POPPSW, &i8080::JP, &i8080::DI, &i8080::CP, &i8080::PUSHPSW, &i8080::ORI, &i8080::RST6, &i8080::RM, &i8080::SPHL, &i8080::JM, &i8080::EI, &i8080::CM, &i8080::NOP, &i8080::CPI, &i8080::RST7 }; 

public:

	// Initialisation
	i8080();
	~i8080();
	bool loadROM(const char*);

	// Ports
	/* port1 bits:
		0: coin 0=active
		1: P2 start
		2: P1 start
		3: ?
		4: P1 shoot
		5: P1 joystick left
		6: P1 joystick right
		7: ?
	*/
	uint8_t port1;
	/* port2 bits:
		0: dipswitch # lives, 0:3, 1:4, 2:5, 3:6
		1: dipswitch # lives
		2: tilt 'button'
		3: dipswitch bonus life at 1:1000, 0:1500
		4: P2 shoot
		5: P2 joystick left
		6: P2 joystick right
		7: dipswitch coin info 1:off, 0:on
	*/
	uint8_t port2;

	// Emulation
	void emulateCycle();
	uint8_t fetchGFXPixel(int index);

	// Debug
	void printState();
	void printStateShort();
	uint8_t fetchReg(char);
	uint16_t fetchRegPair(char);
	uint8_t fetchRAM(uint16_t);
};
#endif
