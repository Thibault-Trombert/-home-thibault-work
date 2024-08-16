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

#include <string.h>
#include "Cartridge.h"
#include "Sound.h"
#include "Pad.h"
#include "Memory.h"
#include "CPU.h"

using namespace std;

Memory::Memory(CPU *cpu, Sound *s)
{
    m_cpu = cpu;
	m_c = NULL;
	m_s = s;
	ResetMem();
}

Memory::~Memory()
{
}

Memory *Memory::GetPtrMemory() { return this; }

void Memory::LoadCartridge(Cartridge *c)
{
	m_c = c;
}

void Memory::ResetMem()
{
	memset(&memory, 0x00, SIZE_MEM);

	memory[TIMA] = 0x00;
    memory[TMA]  = 0x00;
    memory[TAC]  = 0xF8;
	
	memory[NR10]  = 0x80;
	memory[NR11]  = 0xBF;
	memory[NR12]  = 0xF3;
	memory[NR14]  = 0xBF;
	memory[NR21]  = 0x3F;
	memory[NR22]  = 0x00;
	memory[NR24]  = 0xBF;
	memory[NR30]  = 0x7F;
	memory[NR31]  = 0xFF;
	memory[NR32]  = 0x9F;
	memory[NR33]  = 0xBF;
	memory[NR41]  = 0xFF;
	memory[NR42]  = 0x00;
	memory[NR43]  = 0x00;
	memory[NR30]  = 0xBF;
	memory[NR50]  = 0x77;
	memory[NR51]  = 0xF3;
	memory[NR52]  = 0xF1;
	
	if (m_s)
	{
		m_s->WriteRegister(NR10, 0x80);
		m_s->WriteRegister(NR11, 0xBF);
		m_s->WriteRegister(NR12, 0xF3);
		m_s->WriteRegister(NR14, 0xBF);
		m_s->WriteRegister(NR21, 0x3F);
		m_s->WriteRegister(NR22, 0x00);
		m_s->WriteRegister(NR24, 0xBF);
		m_s->WriteRegister(NR30, 0x7F);
		m_s->WriteRegister(NR31, 0xFF);
		m_s->WriteRegister(NR32, 0x9F);
		m_s->WriteRegister(NR33, 0xBF);
		m_s->WriteRegister(NR41, 0xFF);
		m_s->WriteRegister(NR42, 0x00);
		m_s->WriteRegister(NR43, 0x00);
		m_s->WriteRegister(NR30, 0xBF);
		m_s->WriteRegister(NR50, 0x77);
		m_s->WriteRegister(NR51, 0xF3);
		m_s->WriteRegister(NR52, 0xF1);
	}
	
    memory[LCDC] = 0x91;
    memory[STAT] = 0x01;
    memory[SCY]  = 0x00;
    memory[SCX]  = 0x00;
    memory[LY]   = 0x90;
    memory[LYC]  = 0x00;
    memory[BGP]  = 0xFC;
    memory[OBP0] = 0xFF;
    memory[OBP1] = 0xFF;
    memory[WY]   = 0x00;
    memory[WX]   = 0x00;
    memory[IF]   = 0xE1;
    memory[IE]   = 0x00;
    
    if (m_colorMode)
    {
        m_wRam = &memory[WRAM_OFFSET+0x1000];
        m_vRam = &memory[VRAM_OFFSET];
        
        for (int i=BGP_OFFSET; i<BGP_OFFSET+SIZE_BGPCOLOR; i+=2)
        {
            memory[i+0] = 0xFF;
            memory[i+1] = 0x7F;
        }
        
        memory[HDMA5] = 0xFF;
        m_hdmaActive = false;
    }
}

void Memory::MemW(WORD address, BYTE value)
{
	if ((address < 0x8000) || ((address >= 0xA000)&&(address < 0xC000)))
	{
		m_c->Write(address, value);
		return;
	}
    else if ((address >= 0x8000) && (address < 0xA000) && m_colorMode) //VRAM
    {
		m_vRam[address - 0x8000] = value;
        return;
    }
    else if ((address >= 0xD000) && (address < 0xE000) && m_colorMode) //WRAM
    {
		m_wRam[address - 0xD000] = value;
        return;
    }
	else if ((address >= 0xC000) && (address < 0xDE00))//C000-DDFF
		memory[address + 0x2000] = value;
	else if ((address >= 0xE000) && (address < 0xFE00))//E000-FDFF
		memory[address - 0x2000] = value;
	else if ((address >= 0xFF10) && (address <= 0xFF3F))
        m_s->WriteRegister(address, value);
	else
	{
		switch (address)
		{
			case DMA:
				OamDmaTransfer(value);
				break;
			case P1:
				value = m_cpu->P1Changed(value);
				break;
            case TAC:
                value = m_cpu->TACChanged(value);
                break;
			case STAT:
                m_cpu->StatChanged(value);
                return;
			case LY:
                memory[LY] = 0;
                m_cpu->CheckLYC();
                return;
            case LYC:
                memory[LYC] = value;
                m_cpu->CheckLYC();
                return;
			case DIV:
                value = m_cpu->DIVChanged(value);
                break;
            case LCDC:
                m_cpu->OnWriteLCDC(value);
                return;
            case VBK:
                if (m_colorMode) {
                    //if (!hdmaActive)   // Si no esta activo el HDMA
                    //{
                        value &= 0x01;
                        m_vRam = &memory[VRAM_OFFSET+(value*0x2000)];
                    //}
                }
                break;
            case HDMA5:
                if (m_colorMode)
                    value = VRamDmaTransfer(value);
                break;
            case KEY1:
                if (m_colorMode)
                    value = ((memory[KEY1] & 0x80) | (value & 0x01));
                break;
            case SVBK:
                if (m_colorMode) {
                    value &= 0x07;
                    if (value == 0)
                        value = 1;
                    m_wRam = &memory[WRAM_OFFSET+(value*0x1000)];
                }
                break;
            case BGPI:
                if (m_colorMode)
                    value &= 0xBF;
                break;
            case BGPD:
                if (m_colorMode) {
                    BYTE index = memory[BGPI] & 0x3F;
                    memory[BGP_OFFSET + index] = value;
                    
                    if (BIT7(memory[BGPI]))
                        memory[BGPI] = 0x80 | ((index+1) & 0x3F);
                }
                break;
            case OBPI:
                if (m_colorMode)
                    value &= 0xBF;
                break;
            case OBPD:
                if (m_colorMode) {
                    BYTE index = memory[OBPI] & 0x3F;
                    memory[OBP_OFFSET + index] = value;
                    
                    if (BIT7(memory[OBPI]))
                        memory[OBPI] = 0x80 | ((index+1) & 0x3F);
                }
                break;
            case IF:
                value = 0xE0 | (value & 0x1F);
                break;
		}
	}

	memory[address] = value;
}

BYTE Memory::MemRVRam(WORD address, int slot)
{
    return memory[(address-0x8000) + VRAM_OFFSET + (slot*0x2000)];
}

BYTE Memory::MemRWRam(WORD address, int slot)
{
    return memory[(address-0xD000) + WRAM_OFFSET + (slot*0x1000)];
}

void Memory::OamDmaTransfer(BYTE address)
{
	BYTE i;

	for (i=0; i<0xA0; i++)
		MemWNoCheck(0xFE00 + i, MemR((address << 8) + i));
}

BYTE Memory::VRamDmaTransfer(BYTE value)
{
    WORD mode = value & 0x80;
    
    if (m_hdmaActive && mode == 0)    // Se quiere parar el hdma manualmente
    {
        m_hdmaActive = false;
        value = memory[HDMA5] | 0x80;  // Se pone a 1 el bit 7
    }
    else
    {
        if (mode == 0)  // Todo de golpe
        {
            WORD src = (memory[HDMA1] << 8) | (memory[HDMA2] & 0xF0);           // valores entre 0000-7FF0 o A000-DFF0
            WORD dst = ((memory[HDMA3] & 0x1F) << 8) | (memory[HDMA4] & 0xF0);  // Valores entre 0x0000-0x1FF0
            WORD length = ((value & 0x7F) + 1) * 0x10;                  // Valores entre 0x10-0x800
            
            for (int i= 0; i<length; i++)
                m_vRam[dst+i] = MemR(src+i);
            
            WORD srcEnd = src + length;
            WORD dstEnd = dst + length;
            memory[HDMA1] = srcEnd >> 8;
            memory[HDMA2] = srcEnd & 0xF0;
            memory[HDMA3] = dstEnd >> 8;
            memory[HDMA4] = dstEnd & 0xF0;
            value = 0xFF; // Se especifica que se ha terminado la copia
            
            if (memory[KEY1] & 0x80)
                m_cpu->AddCycles(HDMA_CYCLES*length/0x10*2);
            else
                m_cpu->AddCycles(HDMA_CYCLES*length/0x10);
            
            m_hdmaActive = false;
        }
        else
        {
            m_hdmaActive = true;
            value = value & 0x7F;  // Se pone a 0 el bit 7
        }
    }
    
    return value;
}

void Memory::UpdateHDMA()
{
    if (m_hdmaActive)
    {
        WORD src = (memory[HDMA1] << 8) | (memory[HDMA2] & 0xF0);           // valores entre 0000-7FF0 o A000-DFF0
        WORD dst = ((memory[HDMA3] & 0x1F) << 8) | (memory[HDMA4] & 0xF0);  // Valores entre 0x0000-0x1FF0
        
        for (int i=0; i<0x10; i++)
            m_vRam[dst+i] = MemR(src+i);
        
        WORD srcEnd = src + 0x10;
        WORD dstEnd = dst + 0x10;
        memory[HDMA1] = srcEnd >> 8;
        memory[HDMA2] = srcEnd & 0xF0;
        memory[HDMA3] = dstEnd >> 8;
        memory[HDMA4] = dstEnd & 0xF0;
        
        memory[HDMA5] -= 1;
        if ((memory[HDMA5] & 0x7F) == 0x7F)
        {
            m_hdmaActive = false;
            memory[HDMA5] = 0xFF;
        }
        
        if (memory[KEY1] & 0x80)
            m_cpu->AddCycles(HDMA_CYCLES*2);
        else
            m_cpu->AddCycles(HDMA_CYCLES);
    }
}

void Memory::SaveMemory(ofstream * file)
{
	file->write((char *)&memory[0x8000], 0x8000);
    if (m_colorMode) {
        file->write((char *)&m_hdmaActive, sizeof(m_hdmaActive));
        file->write((char *)&memory[WRAM_OFFSET], SIZE_MEM - WRAM_OFFSET);
    }
}

void Memory::LoadMemory(ifstream * file)
{
	file->read((char *)&memory[0x8000], 0x8000);
    if (m_colorMode) {
        file->read((char *)&m_hdmaActive, sizeof(m_hdmaActive));
        file->read((char *)&memory[WRAM_OFFSET], SIZE_MEM - WRAM_OFFSET);
    }
	if (m_s)
	{
		for (int dir=0xFF10; dir<0xFF40; dir++)
			m_s->WriteRegister(dir, memory[dir]);
	}
	
}
