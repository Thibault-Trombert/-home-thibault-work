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

#ifndef __CPU_H__
#define __CPU_H__

#include "Registers.h"
#include "Memory.h"

class QueueLog;
class Video;
class Instructions;
class Cartridge;
class Pad;

class CPU: public Registers, public Memory
{
private:
	unsigned long m_numInstructions;
	BYTE m_lastCycles;
	int  m_cyclesLCD;
	WORD m_cyclesTimer;
	WORD m_cyclesDIV;
	WORD m_cyclesSerial;
	int  m_bitSerial;
    
    int m_lcdMode0;
    int m_lcdMode1;
    int m_lcdMode2;
    int m_lcdMode3;
    int m_cyclesFrame;
	Video *m_v;
    Pad *m_p;
#ifdef MAKEGBLOG
	QueueLog *m_log;
#endif
	bool m_VBlankIntPending;
    bool m_newInterrupt;
public:
	CPU(Video *v, Pad *p, Sound *s);
	CPU(Video *v, Pad *p, Cartridge *c, Sound *s);
	~CPU();
	
    int  Execute(int cyclesToExecute);
	int  ExecuteOneFrame();
	void UpdatePad(bool buttonsState[8]);
    void OnWriteLCDC(BYTE value);
    BYTE TACChanged(BYTE newValue);
    BYTE DIVChanged(BYTE newValue);
    BYTE P1Changed(BYTE newValue);
    void StatChanged(BYTE newValue);
    void AddCycles(int cycles);
    void CheckLYC();
	void Reset();
#ifdef MAKEGBLOG
	void SaveLog();
#endif
	void SaveState(std::string saveDirectory, int numSlot);
	void LoadState(std::string loadDirectory, int numSlot);
private:
	void Init(Video *v, Pad *p);
    void ResetGlobalVariables();
	void OpCodeCB(Instructions *inst);
	void UpdateStateLCD(int cycles);
    void UpdateStateLCDOn();
	void UpdateTimer(int cycles);
	void UpdateSerial(int cycles);
    void SetIntFlag(int bit);
	void Interrupts(Instructions * inst);
	void OnEndFrame();
    void ChangeSpeed();
};

#endif
