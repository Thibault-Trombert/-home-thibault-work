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

#include <fstream>
#include <iostream>
#include <string>
#include <string.h>
#include <iomanip>
#include <assert.h>
#include "Cartridge.h"
#include "Def.h"
#include "GBException.h"
#include "MBC.h"

using namespace std;

/*
 * Constructor que recibe un fichero, lo carga en memoria y lo procesa
 */
Cartridge::Cartridge(string fileName, string batteriesPath)
{
	m_memCartridge = NULL;
	ifstream::pos_type size;
	ifstream file (fileName.c_str(), ios::in|ios::binary|ios::ate);
	if (file.is_open())
	{
		size = file.tellg();
		m_romSize = (unsigned long)size;
		m_memCartridge = new BYTE [size];
		file.seekg (0, ios::beg);
		file.read((char *)m_memCartridge, (streamsize)size);
		file.close();

		cout << fileName << ":\nFile loaded in memory correctly" << endl;
		
		CheckCartridge(batteriesPath);
		
		m_isLoaded = true;
	}
	else
	{
		cerr << fileName << ": Error trying to open the file" << endl;
		m_isLoaded = false;
	}
}

/*
 * Constructor que recibe un buffer y su tamaño y lo procesa
 */
Cartridge::Cartridge(BYTE *cartridgeBuffer, unsigned long size, string batteriesPath)
{
	m_romSize = size;
	m_memCartridge = cartridgeBuffer;
	
	CheckCartridge(batteriesPath);
	
	m_isLoaded = true;
}

Cartridge::~Cartridge(void)
{
	DestroyMBC();
	if (m_memCartridge)
		delete [] m_memCartridge;
}

string Trim(const string &str) {
    size_t endpos = str.find_last_not_of(" ");
    if( string::npos != endpos )
        return str.substr( 0, endpos+1 );
    else
        return str;
}

string Cartridge::GetGoodName(const char *name) {
    string allow = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz ._-'!";
    
    int numChars = 16;
    if ((m_memCartridge[CART_COLOR] == 0x80) || (m_memCartridge[CART_COLOR] == 0xC0))
        numChars = 11;
    string tmp = string(name, numChars);
    
    size_t found = 0;
    while (found != string::npos) {
        found = tmp.find_first_not_of(allow);
        if (found != string::npos)
            tmp[found] = ' ';
    }
    
    return Trim(tmp);
}

/*
 * Comprueba el buffer de la rom, extrae el nombre, compara el tamaño e inicializa el MBC
 */
void Cartridge::CheckCartridge(string batteriesPath)
{
	MBCPathBatteries(batteriesPath);
	m_name = GetGoodName((char *)&m_memCartridge[CART_NAME]);
	
	CheckRomSize((int)m_memCartridge[CART_ROM_SIZE], m_romSize);
    m_hasRTC = false;
	
	switch(m_memCartridge[CART_TYPE])
	{
		case 0x00:						//ROM ONLY
		case 0x08:						//ROM+RAM
		case 0x09:						//ROM+RAM+BATTERY
			ptrRead = &NoneRead;
			ptrWrite = &NoneWrite;
			InitMBCNone(m_name, m_memCartridge, m_romSize);
			break;
		case 0x01:						//ROM+MBC1 
		case 0x02:						//ROM+MBC1+RAM 
		case 0x03:						//ROM+MBC1+RAM+BATT
			ptrRead = &MBC1Read;
			ptrWrite = &MBC1Write;
			InitMBC1(m_name, m_memCartridge, m_romSize, m_memCartridge[CART_RAM_SIZE]);
			break;
		case 0x05:						//ROM+MBC2 
		case 0x06:						//ROM+MBC2+BATTERY
			ptrRead = &MBC2Read;
			ptrWrite = &MBC2Write;
			InitMBC2(m_name, m_memCartridge, m_romSize);
			break;
			/*
			 case 0x0B:						//ROM+MMM01
			 case 0x0C:						//ROM+MMM01+SRAM
			 case 0x0D: mbc = MMM01; break;	//ROM+MMM01+SRAM+BATT*/
		case 0x0F:						//ROM+MBC3+TIMER+BATT
		case 0x10:						//ROM+MBC3+TIMER+RAM+BATT
            m_hasRTC = true;
		case 0x11:						//ROM+MBC3
		case 0x12:						//ROM+MBC3+RAM
		case 0x13:						//ROM+MBC3+RAM+BATT
			ptrRead = &MBC3Read;
			ptrWrite = &MBC3Write;
			InitMBC3(m_name, m_memCartridge, m_romSize, m_memCartridge[CART_RAM_SIZE], m_hasRTC);
			break;
		case 0x19:						//ROM+MBC5
		case 0x1A:						//ROM+MBC5+RAM
		case 0x1B:						//ROM+MBC5+RAM+BATT
		case 0x1C:						//ROM+MBC5+RUMBLE
		case 0x1D:						//ROM+MBC5+RUMBLE+SRAM
		case 0x1E:						//ROM+MBC5+RUMBLE+SRAM+BATT
			ptrRead = &MBC5Read;
			ptrWrite = &MBC5Write;
			InitMBC5(m_name, m_memCartridge, m_romSize, m_memCartridge[CART_RAM_SIZE]);
			break;
			/*case 0x1F:						//Pocket Camera
			 case 0xFD:						//Bandai TAMA5
			 case 0xFE: mbc = Other; break;	//Hudson HuC-3
			 case 0xFF: mbc = HuC1; break;	//Hudson HuC-1*/
		default: throw GBException("MBC not implemented yet");
	}
}

/*
 * Compara el tamaño de la rom con el valor de la cabecera
 */
int Cartridge::CheckRomSize(int numHeaderSize, int fileSize)
{
	int headerSize = 32768 << (numHeaderSize & 0x0F);
	if (numHeaderSize & 0xF0)
		headerSize += (32768 << ((numHeaderSize & 0xF0) >> 0x04));
	assert(headerSize == fileSize);
	if (headerSize != fileSize)
	{
		cout << "The header does not match with the file size" << endl;
		return 0;
	}
	else
		return 1;
}

BYTE *Cartridge::GetData()
{
	return m_memCartridge;
}

unsigned int Cartridge::GetSize()
{
	return m_romSize;
}

string Cartridge::GetName()
{
	return m_name;
}

bool Cartridge::IsLoaded()
{
	return m_isLoaded;
}

void Cartridge::SaveStateMBC(ofstream * file)
{
	MBCSaveState(file);
}

void Cartridge::LoadStateMBC(ifstream * file)
{
	MBCLoadState(file);
}

void Cartridge::Extract() {
    MBCExtract();
}
