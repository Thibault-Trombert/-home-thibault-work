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

#include <sstream>
#include <iomanip>
#include "Def.h"
#include "Sound.h"
#include "Video.h"
#include "CPU.h"
#include "Cartridge.h"
#include "InstructionsDef.h"
#include "Debugger.h"

using namespace std;

struct BreakpointNode {
    WORD value;
    BreakpointNode *prev;
    BreakpointNode *next;
};

Debugger::Debugger(Sound *sound, Video *video, CPU *cpu, Cartridge *cartridge)
{
    m_sound = sound;
    m_video = video;
	m_cpu = cpu;
	m_cartridge = cartridge;
    m_firstBreakpoint = NULL;
    m_lastBreakpoint = NULL;
}

Debugger::~Debugger()
{
	ClearBreakpoints();
}

std::string Debugger::GetRegAF()
{
    return ToHex(m_cpu->GetAF(), 4, '0');
}

std::string Debugger::GetRegBC()
{
    return ToHex(m_cpu->GetBC(), 4, '0');
}

std::string Debugger::GetRegDE()
{
    return ToHex(m_cpu->GetDE(), 4, '0');
}

std::string Debugger::GetRegHL()
{
    return ToHex(m_cpu->GetHL(), 4, '0');
}

std::string Debugger::GetRegSP()
{
    return ToHex(m_cpu->GetSP(), 4, '0');
}

std::string Debugger::GetRegPC()
{
    return ToHex(m_cpu->GetPC(), 4, '0');
}

std::string Debugger::GetRegs()
{
    string regs;
    regs += "AF: " + GetRegAF() + "\n";
    regs += "BC: " + GetRegBC() + "\n";
    regs += "DE: " + GetRegDE() + "\n";
    regs += "HL: " + GetRegHL() + "\n";
    regs += "PC: " + GetRegPC() + "\n";
    regs += "SP: " + GetRegSP();
    
    return regs;
}

std::string Debugger::GetMem(WORD address) {
    stringstream ss;
    
    BYTE value = m_cpu->MemR(address);
    AppendHex(ss, value, 2, '0');
    return ss.str();
}

std::string Debugger::GetMem(WORD start, WORD end)
{
    start &= 0xFFF0;
    end = (end & 0xFFF0)+0x000F;
    
    stringstream ss;
    unsigned int row = start;
    while (row <= end)
    {
        AppendHex(ss, row, 4, '0');
        ss << ": ";
        for (int i=0x0; i<0xF; i++)
        {
            BYTE value = m_cpu->MemR(row+i);
            AppendHex(ss, value, 2, '0');
            ss << ' ';
        }
        
        BYTE value = m_cpu->MemR(row+0xF);
        AppendHex(ss, value, 2, '0');
        if (row < (end & 0xFFF0))
            ss << '\n';
        row += 0x10;
    }
    
    return ss.str();
}

std::string Debugger::Disassemble(WORD start, int numInstructions) {
    stringstream ss;
    
    int processed = 0;
    unsigned int address = start;
    while ((processed < numInstructions) && (address <= 0xFFFF))
    {
        ss << "0x";
        AppendHex(ss, address, 4, '0');
        ss << ": ";
        BYTE opcode = m_cpu->MemR(address);
        if (opcode != 0xCB) {
            ss << GetInstructionName(opcode);
            address += GetInstructionLength(opcode);
        }
        else {
            opcode = m_cpu->MemR(address+1);
            ss << GetInstructionCBName(opcode);
            address += 2;
        }
        
        processed++;
        
        if (processed < numInstructions)
            ss << '\n';
    }
    
    return ss.str();
}

std::string Debugger::Disassemble(int numInstructions) {
    return Disassemble(m_cpu->GetPC(), numInstructions);
}

void Debugger::DisassembleNext(WORD &currentAddress, WORD &nextAddress, std::string &name, std::string &data) {
    currentAddress = m_cpu->GetPC();
    DisassembleOne(currentAddress, nextAddress, name, data);
}

void Debugger::DisassembleOne(WORD address, WORD &nextAddress, std::string &name, std::string &data) {
    stringstream ss1, ss2;
    
    int length = 0;
    BYTE opcode = m_cpu->MemR(address);
    if (opcode != 0xCB) {
        ss1 << GetInstructionName(opcode);
        length += GetInstructionLength(opcode);
    }
    else {
        opcode = m_cpu->MemR(address+1);
        ss1 << GetInstructionCBName(opcode);
        length += 2;
    }
    
    for (int i=0; i<length; i++) {
        WORD nextData = m_cpu->MemR(address + i);
        AppendHex(ss2, nextData, 2, '0');
        if (i < length-1)
            ss2 << " ";
    }
    
    name = ss1.str();
    data = ss2.str();
    nextAddress = address + length;
}

std::string Debugger::GetMemVRam(WORD start, WORD end, int slot)
{
    start &= 0xFFF0;
    end = (end & 0xFFF0)+0x000F;
    
    stringstream ss;
    unsigned int row = start;
    while (row <= end)
    {
        ss << "0x";
        AppendHex(ss, row, 4, '0');
        ss << ": ";
        for (int i=0x0; i<0xF; i++)
        {
            BYTE value = m_cpu->MemRVRam(row+i, slot);
            AppendHex(ss, value, 2, '0');
            ss << ' ';
        }
        
        BYTE value = m_cpu->MemRVRam(row+0xF, slot);
        AppendHex(ss, value, 2, '0');
        if (row < end)
            ss << '\n';
        row += 0x10;
    }
    
    return ss.str();
}

std::string Debugger::GetMemPalette(int sprite, int number)
{
    int address = BGP_OFFSET;
    stringstream ss;
    
    number &= 0x07;
    if (sprite)
        address = OBP_OFFSET;
    
    address += number*8;
    
    ss << "Palette " << number << ": ";
    
    for (int i=0; i<4; i++)
    {
        WORD *value = (WORD *)((void *)&m_cpu->memory[address]);
        AppendHex(ss, *value, 4, '0');
        if (i < 3)
            ss << ", ";
        else
            ss << "\n";
        address += 2;
    }
    
    return ss.str();
}

void Debugger::GetColorPalette(int sprite, int number, BYTE palette[4][3])
{
    int address = BGP_OFFSET;
    
    number &= 0x07;
    if (sprite)
        address = OBP_OFFSET;
    
    address += number*8;
    
    m_video->GetColorPalette(palette, address);
}

void Debugger::GetBG(BYTE *buffer)
{
    
}

void Debugger::GetWindow(BYTE *buffer)
{
    
}

void Debugger::GetTiles(BYTE *buffer, int width, int height)
{
    int x, y, tile, slot;
    BYTE *tmpBuffer;
    int widthSize = width*3;
    int tilesInX = width / 8;
    int tilesInY = height / 8;
    
    y = 0;
    tile = 0;
    slot = 0;
    while ((y < tilesInY))
    {
        x = 0;
        tmpBuffer = buffer + (widthSize*y*8);
        while ((x < tilesInX) && (tile < 384))
        {
            m_video->GetTile(tmpBuffer, widthSize, tile, slot);
            tmpBuffer += 8*3;
            tile++;
            if ((slot == 0) && (tile >= 384))
            {
                tile = 0;
                slot = 1;
            }
            x++;
        }
        y++;
    }
}

void Debugger::Reset() {
    m_cpu->Reset();
}

void Debugger::StepInto() {
    m_cpu->Execute(1);
}

bool Debugger::ExecuteOneFrame() {
    int cycles = 0;
    while(cycles < FRAME_CYCLES) {
        cycles += m_cpu->Execute(1);
        if (GetBreakpointNode(m_cpu->GetPC()))
            return false;
    }
    return true;
}

std::string Debugger::ToHex(int value, int width, char fill)
{
    stringstream ss;
    ss << setfill(fill) << setw(width) << uppercase << hex << value;
    return ss.str();
}

void Debugger::AppendHex(stringstream &ss, int value, int width, char fill)
{
    ss << setfill(fill) << setw(width) << uppercase << hex << (int)value;
}

void Debugger::AddBreakpoint(WORD address) {
    if (GetBreakpointNode(address))
        return;
    
    BreakpointNode *node = new BreakpointNode;
    node->value = address;
    node->prev = m_lastBreakpoint;
    node->next = NULL;
    if (m_lastBreakpoint)
        m_lastBreakpoint->next = node;
    if (m_firstBreakpoint == NULL)
        m_firstBreakpoint = node;
    m_lastBreakpoint = node;
}

void Debugger::DelBreakpoint(WORD address) {
    BreakpointNode *node = GetBreakpointNode(address);
    if (node) {
        if (node->prev)
            node->prev->next = node->next;
        if (node->next)
            node->next->prev = node->prev;
        if (m_firstBreakpoint == node)
            m_firstBreakpoint = node->next;
        if (m_lastBreakpoint == node)
            m_lastBreakpoint = node->prev;
        
        delete node;
    }
}

int Debugger::GetNumBreakpoints() {
    int count = 0;
    BreakpointNode *node = m_firstBreakpoint;
    
    while (node) {
        count++;
        node = node->next;
    }
    
    return count;
}

WORD Debugger::GetBreakpoint(int i) {
    int count = 0;
    WORD value = 0;
    BreakpointNode *node = m_firstBreakpoint;
    
    while (node) {
        if (count == i)
            value = node->value;
        count++;
        node = node->next;
    }
    
    return value;
}

bool Debugger::HasBreakpoint(WORD address) {
    return GetBreakpointNode(address) ? true : false;
}

BreakpointNode *Debugger::GetBreakpointNode(WORD address) {
    BreakpointNode *node = m_firstBreakpoint;
    while (node) {
        if (node->value == address)
            return node;
        
        node = node->next;
    }
    
    return NULL;
}

void Debugger::ClearBreakpoints() {
    BreakpointNode *node = m_firstBreakpoint;
    BreakpointNode *next;
    
    while (node) {
        next = node->next;
        delete node;
        node = next;
    }
    
    m_firstBreakpoint = NULL;
    m_lastBreakpoint  = NULL;
}
