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

#ifndef __REGISTERS_H__
#define __REGISTERS_H__

#include "Def.h"
#include <iostream>

enum e_registers {
	A = 0x00, B, C, D, E, F, H, L,	//registros simples
	AF = 0x10, BC, DE, HL,			//registros dobles
	f_Z = 0x20, f_N, f_H, f_C,		//flags
	PC = 0x30, SP, $, c_$$,
	c_BC = 0x40, c_DE, c_HL,		//memoria apuntada por el registro doble
};

union u_register{
	WORD doble;
	BYTE simple[2];
};

class Registers
{
private:
	u_register m_af, m_bc, m_de, m_hl;
	WORD m_pc; //Program Counter
	WORD m_sp; //Stack Pointer
	bool m_IME;
	bool m_pendingIME;
	bool m_pendingIMEvalue;
	bool m_halt;
	bool m_stop;
    bool m_conditionalTaken;
public:
	Registers();
	~Registers();

	Registers *GetPtrRegisters();

	inline BYTE GetA()				{ return m_af.simple[1]; }
	inline void SetA(BYTE value)	{ m_af.simple[1] = value; }
	inline BYTE GetB()				{ return m_bc.simple[1]; }
	inline void SetB(BYTE value)	{ m_bc.simple[1] = value; }
	inline BYTE GetC()				{ return m_bc.simple[0]; }
	inline void SetC(BYTE value)	{ m_bc.simple[0] = value; }
	inline BYTE GetD()				{ return m_de.simple[1]; }
	inline void SetD(BYTE value)	{ m_de.simple[1] = value; }
	inline BYTE GetE()				{ return m_de.simple[0]; }
	inline void SetE(BYTE value)	{ m_de.simple[0] = value; }
	inline BYTE GetF()				{ return m_af.simple[0]; }
	inline void SetF(BYTE value)	{ m_af.simple[0] = value & 0xF0; }
	inline BYTE GetH()				{ return m_hl.simple[1]; }
	inline void SetH(BYTE value)	{ m_hl.simple[1] = value; }
	inline BYTE GetL()				{ return m_hl.simple[0]; }
	inline void SetL(BYTE value)	{ m_hl.simple[0] = value; }

	inline WORD GetAF()             { return m_af.doble; }
	inline void SetAF(WORD value)	{ m_af.doble = value & 0xFFF0; }
	inline WORD GetBC()             { return m_bc.doble; }
	inline void SetBC(WORD value)	{ m_bc.doble = value; }
	inline WORD GetDE()             { return m_de.doble; }
	inline void SetDE(WORD value)	{ m_de.doble = value; }
	inline WORD GetHL()             { return m_hl.doble; }
	inline void SetHL(WORD value)	{ m_hl.doble = value; }

	inline WORD GetPC()             { return m_pc; }
	inline void SetPC(WORD value)   { m_pc = value; }
	inline void AddPC(int value)    { m_pc += value; };
	inline WORD GetSP()             { return m_sp; }
	inline void SetSP(WORD value)   { m_sp = value; }
	inline void AddSP(int value)    { m_sp += value; };

	inline bool GetIME()			{return m_IME;}
	inline void SetIME(bool value, bool immediately=true)
	{
		if (immediately)
		{
			m_IME = value;
			m_pendingIME = false;
		}
		else
		{
			m_pendingIME = true;
			m_pendingIMEvalue = value;
		}
	}
	
	inline void Set_PendingIME()
	{
		if (m_pendingIME)
		{
			m_IME = m_pendingIMEvalue;
			m_pendingIME = false;
		}
	}

	inline bool GetHalt()				{ return m_halt; }
	inline void SetHalt(bool value)	{ m_halt = value; }

	inline bool GetStop()				{ return m_stop; }
	inline void SetStop(bool value)	{ m_stop = value; }

	WORD GetReg(e_registers reg);
	void SetReg(e_registers reg, WORD value);

	inline BYTE GetFlagZ()              { return (m_af.simple[0] >> 7);}
	inline void SetFlagZ(BYTE value)    { m_af.simple[0] = (m_af.simple[0] & 0x7F) | (value << 7); }
	inline BYTE GetFlagN()              { return ((m_af.simple[0] & 0x40) >> 6); }
	inline void SetFlagN(BYTE value)    { m_af.simple[0] = (m_af.simple[0] & 0xBF) | (value << 6); }
	inline BYTE GetFlagH()              { return ((m_af.simple[0] & 0x20) >> 5); }
	inline void SetFlagH(BYTE value)    { m_af.simple[0] = (m_af.simple[0] & 0xDF) | (value << 5); }
	inline BYTE GetFlagC()              { return ((m_af.simple[0] & 0x10) >> 4); }
	inline void SetFlagC(BYTE value)    { m_af.simple[0] = (m_af.simple[0] & 0xEF) | (value << 4); }
	
	BYTE GetFlag(e_registers flag);
	void SetFlag(e_registers flag, BYTE value);
    
    void SetConditionalTaken(bool value) { m_conditionalTaken = value; }
    bool GetConditionalTaken() { return m_conditionalTaken; }

	void ResetRegs();
	void SaveRegs(std::ofstream *file);
	void LoadRegs(std::ifstream *file);
	
	std::string ToString();
};

#endif
