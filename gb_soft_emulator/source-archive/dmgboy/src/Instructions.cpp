/*
 This file is part of DMGBoy.
 
 DMGBoy is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 DMGBoy is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with DMGBoy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <assert.h>
#include "GBException.h"
#include "Registers.h"
#include "Memory.h"
#include "Instructions.h"

using namespace std;

#define _8bitsInmValue (mem->MemR(reg->GetPC() + 1))
#define _16bitsInmValue ((mem->MemR(reg->GetPC() + 2)) << 8) | mem->MemR(reg->GetPC() + 1)

Instructions::Instructions(Registers* reg, Memory* mem)
{
	this->reg = reg;
	this->mem = mem;
}

Instructions::~Instructions(void)
{
}

void Instructions::NOP(){reg->AddPC(1);}

void Instructions::LD_r1_r2(e_registers e_reg1, e_registers e_reg2)
{
	BYTE length = 1;

	if (e_reg1 == c_HL)
	{
		if (e_reg2 == $)
		{
			mem->MemW(reg->GetHL(), _8bitsInmValue);
			length = 2;
		}
		else
			mem->MemW(reg->GetHL(), (BYTE)reg->GetReg(e_reg2));
	}
	else
	{
		if (e_reg2 == c_HL)
		{
			reg->SetReg(e_reg1, mem->MemR(reg->GetHL()));
		}
		else
		{
			reg->SetReg(e_reg1, reg->GetReg(e_reg2));
		}
	}

	reg->AddPC(length);
}


void Instructions::LD_A_n(e_registers place)
{
    int address, value, length = 1;

	switch(place)
	{
		case $:
			value = _8bitsInmValue;
			length = 2;
			break;
		case c_$$:
			address = _16bitsInmValue;
			value = mem->MemR(address);
			length = 3;
			break;
		case c_BC:
			value = mem->MemR(reg->GetBC());
			break;
		case c_DE:
			value = mem->MemR(reg->GetDE());
			break;
		case c_HL:
			value = mem->MemR(reg->GetHL());
			break;
		default:
			value = reg->GetReg(place);
	}

	reg->SetA(value);

    reg->AddPC(length);
}


void Instructions::LD_n_A(e_registers place)
{
    int address, length = 1;

	switch (place)
	{
		case c_$$:
			address = _16bitsInmValue;
			mem->MemW(address, reg->GetA());
			length = 3;
			break;
		case c_BC:
			mem->MemW(reg->GetBC(), reg->GetA());
			break;
		case c_DE:
			mem->MemW(reg->GetDE(), reg->GetA());
			break;
		case c_HL:
			mem->MemW(reg->GetHL(), reg->GetA());
			break;
		default:
			reg->SetReg(place, reg->GetA());
	}

	reg->AddPC(length);
}

void Instructions::JP_nn()
{
	reg->SetPC(_16bitsInmValue);
}

void Instructions::LDH_A_n()
{
	reg->SetA(mem->MemR(0xFF00 + _8bitsInmValue));
	reg->AddPC(2);
}

void Instructions::LDH_c$_A()
{
	mem->MemW(0xFF00 + _8bitsInmValue, reg->GetA());
	reg->AddPC(2);
}

void Instructions::CCF()
{
	reg->SetFlagN(0);
	reg->SetFlagH(0);
	reg->SetFlagC(!reg->GetFlagC());
	reg->AddPC(1);
}

void Instructions::CP_n(e_registers place)
{
	BYTE value;
	BYTE length = 1;

	switch (place)
	{
		case $:
			value = _8bitsInmValue;
			length = 2;
			break;
		case c_HL:
			value = mem->MemR(reg->GetHL());
			break;
		default:
			value = (BYTE)reg->GetReg(place);
	}

	reg->SetFlagZ((reg->GetA() == value) ? 1 : 0);
	reg->SetFlagN(1);
	reg->SetFlagH(((reg->GetA() & 0x0F) < (value & 0x0F)) ? 1 : 0);
	reg->SetFlagC((reg->GetA() < value) ? 1 : 0);

	reg->AddPC(length);
}

void Instructions::CPL()
{
	reg->SetA(~reg->GetA());

	reg->SetFlagN(1);
	reg->SetFlagH(1);

	reg->AddPC(1);
}

void Instructions::LD_n_nn(e_registers place)
{
	assert((place == BC) || (place == DE) || (place == HL) || (place == SP));
	reg->SetReg(place, _16bitsInmValue);
    reg->AddPC(3);
}

void Instructions::LD_nn_SP()
{
	WORD destAddress = _16bitsInmValue;
	mem->MemW(destAddress, reg->GetSP() & 0x00FF);
	mem->MemW(destAddress + 1, reg->GetSP() >> 8);
    reg->AddPC(3);
}

void Instructions::JR()
{
    char address;	//Con signo

	address = _8bitsInmValue;

	//El "2 +" es porque antes de saltar ha tenido que ver cuales eran los dos opcodes de la instruccion
	//y tiene importancia al ser un salto relativo con respecto al actual PC.
    reg->AddPC(2 + address);
}

void Instructions::JR_CC_n(e_registers flag, BYTE value2check)
{
	if (reg->GetFlag(flag) == value2check)
    {
		JR();
        reg->SetConditionalTaken(true);
    }
	else
		reg->AddPC(2);
}

void Instructions::CALL_nn()
{
	reg->AddSP(-1);
	mem->MemW(reg->GetSP(),((reg->GetPC() + 3) & 0xFF00) >> 8);
	reg->AddSP(-1);
	mem->MemW(reg->GetSP(),(reg->GetPC() + 3) & 0x00FF);
	reg->SetPC(_16bitsInmValue);
}

void Instructions::CALL_cc_nn(e_registers flag, BYTE value2check)
{
	if (reg->GetFlag(flag) == value2check)
    {
		CALL_nn();
        reg->SetConditionalTaken(true);
    }
	else
		reg->AddPC(3);
}

void Instructions::LDI_A_cHL()
{
	reg->SetA(mem->MemR(reg->GetHL()));
	reg->SetHL(reg->GetHL() + 1);
	reg->AddPC(1);
}

void Instructions::LDI_cHL_A()
{
	mem->MemW(reg->GetHL(), reg->GetA());
	reg->SetHL(reg->GetHL() + 1);
	reg->AddPC(1);
}

void Instructions::LDD_A_cHL()
{
	reg->SetA(mem->MemR(reg->GetHL()));
	reg->SetHL(reg->GetHL() - 1);
	reg->AddPC(1);
}

void Instructions::LDD_cHL_A()
{
	mem->MemW(reg->GetHL(), reg->GetA());
	reg->SetHL(reg->GetHL() - 1);
	reg->AddPC(1);
}

void Instructions::LD_SP_HL()
{
	reg->SetSP(reg->GetHL());

	reg->AddPC(1);
}

void Instructions::SUB_n(e_registers place)
{
	int value, length = 1;

	if (place == c_HL)
		value = mem->MemR(reg->GetHL());
	else if (place == $)
	{
		value = _8bitsInmValue;
		length = 2;
	}
	else
		value = reg->GetReg(place);

	reg->SetFlagZ((reg->GetA() - value) ? 0 : 1);
	reg->SetFlagN(1);
	reg->SetFlagH(((reg->GetA() & 0x0F) < (value & 0x0F)) ? 1 : 0);
	reg->SetFlagC((reg->GetA() < value) ? 1 : 0);

	reg->SetA(reg->GetA() - value);

	reg->AddPC(length);
}

void Instructions::ADD_A_n(e_registers place)
{
	int value, length = 1;
	BYTE valueReg;

	switch (place)
	{
		case $:		valueReg = _8bitsInmValue; length = 2; break;
		case c_HL:	valueReg = mem->MemR(reg->GetHL()); break;
		default:	valueReg = (BYTE)reg->GetReg(place); break;
	}

	value = reg->GetA() + valueReg;

	reg->SetFlagZ(!(value & 0xFF) ? 1 : 0);
	if (((reg->GetA() & 0x0F) + (valueReg & 0x0F)) > 0x0F) reg->SetFlagH(1); else reg->SetFlagH(0);
	reg->SetFlagN(0);
	reg->SetFlagC((value > 0xFF) ? 1 : 0);

	reg->SetA(value & 0xFF);

	reg->AddPC(length);
}

void Instructions::ADC_A_n(e_registers lugar)
{
	int value, length = 1;
	BYTE valueReg;

	switch (lugar)
	{
		case $:		valueReg = _8bitsInmValue; length = 2; break;
		case c_HL:	valueReg = mem->MemR(reg->GetHL()); break;
		default:	valueReg = (BYTE)reg->GetReg(lugar); break;
	}

	value = reg->GetFlagC() + valueReg + reg->GetA();

	reg->SetFlagZ(!(value & 0xFF) ? 1 : 0);
	reg->SetFlagN(0);
	if ((reg->GetFlagC() + (valueReg & 0x0F) + (reg->GetA() & 0x0F)) > 0x0F)
		reg->SetFlagH(1);
	else
		reg->SetFlagH(0);
	reg->SetFlagC((value > 0xFF) ? 1 : 0);

	reg->SetA(value & 0xFF);

	reg->AddPC(length);
}

void Instructions::INC_n(e_registers place)
{
	BYTE value;

	if (place == c_HL)
	{
		value = mem->MemR(reg->GetHL()) + 1;
		mem->MemW(reg->GetHL(), value);
	}
	else
	{
		value = reg->GetReg(place) + 1;
		reg->SetReg(place, value);
	}
	reg->SetFlagZ(value ? 0 : 1);
	reg->SetFlagN(0);
	reg->SetFlagH((value & 0x0F) ? 0 : 1);

	reg->AddPC(1);
}

void Instructions::ADD_HL_n(e_registers place)
{
	WORD value, hl;
	
	value = reg->GetReg(place);
	hl = reg->GetHL();
	
	reg->SetFlagN(0);
	reg->SetFlagH((((hl & 0x0FFF) + (value & 0x0FFF)) > 0x0FFF) ? 1 : 0);
	reg->SetFlagC(((hl + value) > 0xFFFF) ? 1 : 0);
	
	reg->SetHL(hl + value);
	
	reg->AddPC(1);
}

void Instructions::RLC_n(e_registers place)
{
	BYTE bit7, value;

	if (place == c_HL)
	{
		value = mem->MemR(reg->GetHL());
		bit7 = BIT7(value) >> 7;
		value = (value << 1) | bit7;
		mem->MemW(reg->GetHL(), value);
	}
	else
	{
		value = (BYTE)reg->GetReg(place);
		bit7 = BIT7(value) >> 7;
		value = (value << 1) | bit7;
		reg->SetReg(place, value);
	}

	reg->SetFlagZ(0);
	reg->SetFlagN(0);
	reg->SetFlagH(0);
	reg->SetFlagC(bit7);

    if (mem->MemR(reg->GetPC()) == 0xCB)
    {
        reg->SetFlagZ(value ? 0 : 1);
		reg->AddPC(2);
    }
	else
		reg->AddPC(1);
}

void Instructions::INC_nn(e_registers lugar)
{
	reg->SetReg(lugar, reg->GetReg(lugar) + 1);
	reg->AddPC(1);
}

void Instructions::DAA()
{
	/*
	 http://www.emutalk.net/showthread.php?t=41525&page=108
	 
	 Detailed info DAA
	 Instruction Format:
	 OPCODE                    CYCLES
	 --------------------------------
	 27h                       4
	 
	 
	 Description:
	 This instruction conditionally adjusts the accumulator for BCD addition
	 and subtraction operations. For addition (ADD, ADC, INC) or subtraction
	 (SUB, SBC, DEC, NEC), the following table indicates the operation performed:
	 
	 --------------------------------------------------------------------------------
	 |           | C Flag  | HEX value in | H Flag | HEX value in | Number  | C flag|
	 | Operation | Before  | upper digit  | Before | lower digit  | added   | After |
	 |           | DAA     | (bit 7-4)    | DAA    | (bit 3-0)    | to byte | DAA   |
	 |------------------------------------------------------------------------------|
	 |           |    0    |     0-9      |   0    |     0-9      |   00    |   0   |
	 |   ADD     |    0    |     0-8      |   0    |     A-F      |   06    |   0   |
	 |           |    0    |     0-9      |   1    |     0-3      |   06    |   0   |
	 |   ADC     |    0    |     A-F      |   0    |     0-9      |   60    |   1   |
	 |           |    0    |     9-F      |   0    |     A-F      |   66    |   1   |
	 |   INC     |    0    |     A-F      |   1    |     0-3      |   66    |   1   |
	 |           |    1    |     0-2      |   0    |     0-9      |   60    |   1   |
	 |           |    1    |     0-2      |   0    |     A-F      |   66    |   1   |
	 |           |    1    |     0-3      |   1    |     0-3      |   66    |   1   |
	 |------------------------------------------------------------------------------|
	 |   SUB     |    0    |     0-9      |   0    |     0-9      |   00    |   0   |
	 |   SBC     |    0    |     0-8      |   1    |     6-F      |   FA    |   0   |
	 |   DEC     |    1    |     7-F      |   0    |     0-9      |   A0    |   1   |
	 |   NEG     |    1    |     6-F      |   1    |     6-F      |   9A    |   1   |
	 |------------------------------------------------------------------------------|
	 
	 
	 Flags:
	 C:   See instruction.
	 N:   Unaffected.
	 P/V: Set if Acc. is even parity after operation, reset otherwise.
	 H:   See instruction.
	 Z:   Set if Acc. is Zero after operation, reset otherwise.
	 S:   Set if most significant bit of Acc. is 1 after operation, reset otherwise.
	 
	 Example:
	 
	 If an addition operation is performed between 15 (BCD) and 27 (BCD), simple decimal
	 arithmetic gives this result:
	 
	 15
	 +27
	 ----
	 42
	 
	 But when the binary representations are added in the Accumulator according to
	 standard binary arithmetic:
	 
	 0001 0101  15
	 +0010 0111  27
	 ---------------
	 0011 1100  3C
	 
	 The sum is ambiguous. The DAA instruction adjusts this result so that correct
	 BCD representation is obtained:
	 
	 0011 1100  3C result
	 +0000 0110  06 +error
	 ---------------
	 0100 0010  42 Correct BCD!
	*/
    
    int a = reg->GetA();
    
    if (reg->GetFlagN() == 0)
    {
        if (reg->GetFlagH() || ((a & 0xF) > 9))
            a += 0x06;
        
        if (reg->GetFlagC() || (a > 0x9F))
            a += 0x60;
    }
    else
    {
        if (reg->GetFlagH())
            a = (a - 6) & 0xFF;
        
        if (reg->GetFlagC())
            a -= 0x60;
    }
    
    reg->SetFlagH(0);
    reg->SetFlagZ(0);
    
    if ((a & 0x100) == 0x100)
        reg->SetFlagC(1);
    
    a &= 0xFF;
    
    if (a == 0)
        reg->SetFlagZ(1);
    
    reg->SetA(a);
    
    reg->AddPC(1);
}

void Instructions::DEC_n(e_registers place)
{
	BYTE value;

	if (place == c_HL)
	{
		value = mem->MemR(reg->GetHL()) - 1;
		mem->MemW(reg->GetHL(), value);
	}
	else
	{
		value = reg->GetReg(place) - 1;
		reg->SetReg(place, value);
	}
	
	reg->SetFlagZ(!value ? 1 : 0);
	reg->SetFlagN(1);
	reg->SetFlagH(((value & 0x0F) == 0x0F) ? 1 : 0);

	reg->AddPC(1);
}

void Instructions::DEC_nn(e_registers lugar)
{
	reg->SetReg(lugar, reg->GetReg(lugar) - 1);
	reg->AddPC(1);
}

void Instructions::OR_n(e_registers lugar)
{
	BYTE longitud = 1;
    BYTE value;

	switch (lugar)
	{
		case $:
			value = reg->GetA() | _8bitsInmValue;
			longitud = 2;
			break;
		case c_HL:
			value = reg->GetA() | mem->MemR(reg->GetHL());
			break;
		default:
			value = reg->GetA() | reg->GetReg(lugar);
	}

    reg->SetA(value);

	reg->SetFlagZ(value ? 0 : 1);
	reg->SetFlagN(0);
	reg->SetFlagH(0);
	reg->SetFlagC(0);

	reg->AddPC(longitud);
}

void Instructions::XOR_n(e_registers lugar)
{
    BYTE longitud = 1;
    BYTE value;

	switch (lugar)
	{
		case $:
			value = reg->GetA() ^ _8bitsInmValue;
			longitud = 2;
			break;
		case c_HL:
			value = reg->GetA() ^ mem->MemR(reg->GetHL());
			break;
		default:
			value = reg->GetA() ^ reg->GetReg(lugar);
	}
    
    reg->SetA(value);

	reg->SetFlagZ(value ? 0 : 1);
	reg->SetFlagN(0);
	reg->SetFlagH(0);
	reg->SetFlagC(0);

	reg->AddPC(longitud);
}

void Instructions::RET()
{
	reg->SetPC((mem->MemR(reg->GetSP() + 1) << 8) | mem->MemR(reg->GetSP()));
	reg->AddSP(2);
}

void Instructions::RETI()
{
	EI();
	RET();
}

void Instructions::RET_cc(e_registers flag, BYTE value2check)
{
	reg->AddPC(1);
	if (reg->GetFlag(flag) == value2check)
    {
		RET();
        reg->SetConditionalTaken(true);
    }
}

void Instructions::LD_nn_n(e_registers lugar)
{
	reg->SetReg(lugar, _8bitsInmValue);
	reg->AddPC(2);
}

void Instructions::LD_A_cC()
{
	reg->SetA(mem->MemR(0xFF00 + reg->GetC()));
	reg->AddPC(1);
}

void Instructions::LD_cC_A()
{
	mem->MemW(0xFF00 + reg->GetC(), reg->GetA());
	reg->AddPC(1);
}

void Instructions::SET_b_r(BYTE bit, e_registers place)
{
	if (place == c_HL)
		mem->MemW(reg->GetHL(), mem->MemR(reg->GetHL()) | (1 << bit));
	else
		reg->SetReg(place, reg->GetReg(place) | (1 << bit));

	reg->AddPC(2);
}

void Instructions::BIT_b_r(BYTE bit, e_registers lugar)
{
	BYTE value;

	if (lugar == c_HL)
		value = mem->MemR(reg->GetHL());
	else
		value = (BYTE)reg->GetReg(lugar);

	if (!(value & (1 << bit)))
		reg->SetFlagZ(1);
	else
		reg->SetFlagZ(0);

	reg->SetFlagN(0);
	reg->SetFlagH(1);

	reg->AddPC(2);
}

void Instructions::RES_b_r(BYTE bit, e_registers lugar)
{
    if (lugar == c_HL)
		mem->MemW(reg->GetHL(), mem->MemR(reg->GetHL()) & ~(1 << bit));
	else
		reg->SetReg(lugar, reg->GetReg(lugar) & ~(1 << bit));

	reg->AddPC(2);
}

void Instructions::DI()
{
	reg->SetIME(0);
	reg->AddPC(1);
}

void Instructions::EI()
{
	reg->SetIME(1);
	reg->AddPC(1);
}

void Instructions::SBC_A(e_registers place)
{
	WORD value;
    BYTE result;
    int sum;
	int length = 1;

	switch(place)
	{
		case c_HL:
            value = mem->MemR(reg->GetHL());
            sum = value + reg->GetFlagC();
			break;
		case $:
            value = _8bitsInmValue;
            sum = value + reg->GetFlagC();
			length = 2;
			break;
		default:
            value = reg->GetReg(place);
			sum = value + reg->GetFlagC();
	}
    result = reg->GetA() - sum;
    
	reg->SetFlagZ(!result);
	reg->SetFlagN(1);
    
    if ((reg->GetA() & 0x0F) < (value & 0x0F))
        reg->SetFlagH(true);
    else if ((reg->GetA() & 0x0F) < (sum & 0x0F))
        reg->SetFlagH(true);
    else if (((reg->GetA() & 0x0F)==(value & 0x0F)) && ((value & 0x0F)==0x0F) && (reg->GetFlagC()))
        reg->SetFlagH(true);
    else
        reg->SetFlagH(false);
    
    reg->SetFlagC(reg->GetA() < sum);

	reg->SetA(result);

	reg->AddPC(length);
}

void Instructions::AND(e_registers lugar)
{
	BYTE longitud = 1;

	switch (lugar)
	{
		case $:
			reg->SetA(reg->GetA() & _8bitsInmValue);
			longitud = 2;
			break;
		case c_HL:
			reg->SetA(reg->GetA() & mem->MemR(reg->GetHL()));
			break;
		default:
			reg->SetA(reg->GetA() & reg->GetReg(lugar));
	}

	reg->SetFlagZ(reg->GetA() ? 0 : 1);
	reg->SetFlagN(0);
	reg->SetFlagH(1);
	reg->SetFlagC(0);

	reg->AddPC(longitud);
}


void Instructions::SLA_n(e_registers place)
{
	BYTE bit7, value;

	if (place == c_HL)
	{
		bit7 = BIT7(mem->MemR(reg->GetHL())) >> 7;
		value = mem->MemR(reg->GetHL()) << 1;
		mem->MemW(reg->GetHL(), value);
	}
	else
	{
		bit7 = BIT7(reg->GetReg(place)) >> 7;
		value = reg->GetReg(place) << 1;
		reg->SetReg(place, value);
	}

	reg->SetFlagZ(!value ? 1 : 0);
	reg->SetFlagN(0);
	reg->SetFlagH(0);
	reg->SetFlagC(bit7);

	reg->AddPC(2);
}

void Instructions::SRA_n(e_registers lugar)
{
    BYTE bit0, bit7, value;

	if (lugar == c_HL)
	{
		bit0 = BIT0(mem->MemR(reg->GetHL()));
		bit7 = BIT7(mem->MemR(reg->GetHL()));
		mem->MemW(reg->GetHL(), bit7 | (mem->MemR(reg->GetHL()) >> 1));
		value = mem->MemR(reg->GetHL());
	}
	else
	{
		bit0 = BIT0(reg->GetReg(lugar));
		bit7 = BIT7(reg->GetReg(lugar));
		reg->SetReg(lugar, bit7 | (reg->GetReg(lugar) >> 1));
		value = (BYTE)reg->GetReg(lugar);
	}

	reg->SetFlagZ(!value ? 1 : 0);
	reg->SetFlagN(0);
	reg->SetFlagH(0);
	reg->SetFlagC(bit0);

	reg->AddPC(2);
}

void Instructions::SRL_n(e_registers place)
{
    BYTE bit0, value;

	if (place == c_HL)
	{
		bit0 = BIT0(mem->MemR(reg->GetHL()));
		value = mem->MemR(reg->GetHL()) >> 1;
		mem->MemW(reg->GetHL(), value);
	}
	else
	{
		bit0 = BIT0(reg->GetReg(place));
		value = reg->GetReg(place) >> 1;
		reg->SetReg(place, value);
	}

	reg->SetFlagZ(!value ? 1 : 0);
	reg->SetFlagN(0);
	reg->SetFlagH(0);
	reg->SetFlagC(bit0);

	reg->AddPC(2);
}

void Instructions::ADD_SP_n()
{
	char n = _8bitsInmValue;

	reg->SetFlagZ(0);
	reg->SetFlagN(0);

    reg->SetFlagH(((reg->GetSP() & 0x0F) + (n & 0x0F)) > 0x0F);
    reg->SetFlagC(((reg->GetSP() & 0xFF) + (n & 0xFF)) > 0xFF);

	reg->AddSP(n);

	reg->AddPC(2);
}

void Instructions::JP_HL()
{
	reg->SetPC(reg->GetHL());
}

void Instructions::SCF()
{
	reg->SetFlagN(0);
	reg->SetFlagH(0);
	reg->SetFlagC(1);

	reg->AddPC(1);
}

void Instructions::HALT()
{
	reg->SetHalt(true);
	
	reg->AddPC(1);

}

void Instructions::STOP()
{
    // En una gameboy real apagaria la pantalla si ha
    // transcurrido demasiado tiempo sin pulsar ningun
    // boton. Aqui no se va simular ese comportamiento
	//reg->Set_Stop(true);

	reg->AddPC(2);
}

void Instructions::SWAP(e_registers place)
{
	BYTE value;

	if (place == c_HL)
	{
		value = mem->MemR(reg->GetHL());
		value = ((value & 0x0F) << 4) | ((value & 0xF0) >> 4);
		mem->MemW(reg->GetHL(), value);
	}
	else
	{
		value = (BYTE)reg->GetReg(place);
		value = ((value & 0x0F) << 4) | ((value & 0xF0) >> 4);
		reg->SetReg(place, value);
	}

	reg->SetFlagZ(value ? 0 : 1);
	reg->SetFlagN(0);
	reg->SetFlagH(0);
	reg->SetFlagC(0);

	reg->AddPC(2);
}

void Instructions::PUSH_nn(e_registers lugar)
{
	reg->AddSP(-1);
	mem->MemW(reg->GetSP(), (reg->GetReg(lugar) & 0xFF00) >> 8);
	reg->AddSP(-1);
	mem->MemW(reg->GetSP(), reg->GetReg(lugar) & 0x00FF);

	reg->AddPC(1);
}

void Instructions::POP_nn(e_registers lugar)
{
	reg->SetReg(lugar, (mem->MemR(reg->GetSP() + 1) << 8) | mem->MemR(reg->GetSP()));
	reg->AddSP(2);

	reg->AddPC(1);
}

void Instructions::JP_cc_nn(e_registers flag, BYTE value2check)
{
	WORD nn;

	nn = _16bitsInmValue;

	reg->AddPC(3);

	if (reg->GetFlag(flag) == value2check)
    {
		reg->SetPC(nn);
        reg->SetConditionalTaken(true);
    }
}

void Instructions::RL_n(e_registers place)
{
	BYTE oldBit7, value;

	if (place == c_HL)
	{
		oldBit7 = BIT7(mem->MemR(reg->GetHL())) >> 7;
		value = (mem->MemR(reg->GetHL()) << 1) | reg->GetFlagC();
		mem->MemW(reg->GetHL(), value);
	}
	else
	{
		oldBit7 = BIT7(reg->GetReg(place)) >> 7;
		value = (reg->GetReg(place) << 1) | reg->GetFlagC();
		reg->SetReg(place, value);
	}

	reg->SetFlagZ(0);
	reg->SetFlagN(0);
	reg->SetFlagH(0);
	reg->SetFlagC(oldBit7);

	if (mem->MemR(reg->GetPC()) == 0xCB)
    {
        reg->SetFlagZ(value ? 0 : 1);
		reg->AddPC(2);
    }
	else
		reg->AddPC(1);
}

void Instructions::RR_n(e_registers place)
{
	BYTE bit0, value;

	if (place == c_HL)
	{
		value = mem->MemR(reg->GetHL());
		bit0 = BIT0(value);
		value = (reg->GetFlagC() << 7) | (value >> 1);
		mem->MemW(reg->GetHL(), value);
	}
	else
	{
		value = (BYTE)reg->GetReg(place);
		bit0 = BIT0(value);
		value = (reg->GetFlagC() << 7) | (value >> 1);
		reg->SetReg(place, value);
	}

	reg->SetFlagZ(0);
	reg->SetFlagN(0);
	reg->SetFlagH(0);
	reg->SetFlagC(bit0);

	if (mem->MemR(reg->GetPC()) == 0xCB)
    {
        reg->SetFlagZ(value ? 0 : 1);
		reg->AddPC(2);
    }
	else
		reg->AddPC(1);
}

void Instructions::RRC_n(e_registers place)
{
	BYTE bit0, value;

	if (place == c_HL)
	{
		value = mem->MemR(reg->GetHL());
		bit0 = BIT0(value);
		value = (bit0 << 7) | (value >> 1);
		mem->MemW(reg->GetHL(), value);
	}
	else
	{
		value = (BYTE)reg->GetReg(place);
		bit0 = BIT0(value);
		value = (bit0 << 7) | (value >> 1);
		reg->SetReg(place, value);
	}

	reg->SetFlagZ(0);
	reg->SetFlagN(0);
	reg->SetFlagH(0);
	reg->SetFlagC(bit0);

	if (mem->MemR(reg->GetPC()) == 0xCB)
    {
        reg->SetFlagZ(value ? 0 : 1);
		reg->AddPC(2);
    }
	else
		reg->AddPC(1);
}

void Instructions::PUSH_PC()
{
	reg->AddSP(-1);
	mem->MemW(reg->GetSP(), (reg->GetPC() & 0xFF00) >> 8);
	reg->AddSP(-1);
	mem->MemW(reg->GetSP(), reg->GetPC() & 0x00FF);
}

void Instructions::RST_n(BYTE desp)
{
	//Queremos que se guarde la siquiente instruccion a ejecutar
	reg->AddPC(1);

	PUSH_PC();

	reg->SetPC(0x0000 + desp);
}

void Instructions::LDHL_SP_n()
{
	char n = _8bitsInmValue;

	reg->SetFlagZ(0);
	reg->SetFlagN(0);
    reg->SetFlagH(((reg->GetSP() & 0x0F) + (n & 0x0F)) > 0x0F);
    reg->SetFlagC(((reg->GetSP() & 0xFF) + (n & 0xFF)) > 0xFF);

	reg->SetHL(reg->GetSP() + n);

	reg->AddPC(2);
}
