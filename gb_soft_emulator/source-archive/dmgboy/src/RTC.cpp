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


#include <ctime>
#include <stdio.h>
#include <fstream>
#include "RTC.h"

#define RTC_S   0
#define RTC_M   1
#define RTC_H   2
#define RTC_DL  3
#define RTC_DH  4

#define DAY_H        0x01
#define HALT         0x40
#define DAY_OVERFLOW 0x80

using namespace std;

RTC::RTC() {
    for (int i=0; i<5; i++) {
        m_latchedRegs[i] = 0;
        m_refRegs[i] = 0;
    }
    m_selectedReg = 0;
    m_latchData = 0;
    m_refTime = time(NULL);
}


void RTC::Update(int *regs) {
    int elapsed = 0;
    if ((m_refRegs[RTC_DH] & HALT) == 0) {
        time_t now = time(NULL);
        elapsed = (int)difftime(now, m_refTime);
    }
    Add(elapsed, regs);
}

void RTC::Add(int value, int *regs) {
    int days = value / 86400;
    value %= 86400;
    int hours = value / 3600;
    value %= 3600;
    int minutes = value / 60;
    value %= 60;
    int seconds = value;
    
    regs[RTC_S] = m_refRegs[RTC_S] + seconds;
    while (regs[RTC_S] >= 60) {
        regs[RTC_S] -= 60;
        minutes++;
    }
    
    regs[RTC_M] = m_refRegs[RTC_M] + minutes;
    while (regs[RTC_M] >= 60) {
        regs[RTC_M] -= 60;
        hours++;
    }
    
    regs[RTC_H] = m_refRegs[RTC_H] + hours;
    while (regs[RTC_H] >= 24) {
        regs[RTC_H] -= 24;
        days++;
    }
    
    int daysInRegs = ((m_refRegs[RTC_DH] & 0x01) << 8) | m_refRegs[RTC_DL];
    
    days += daysInRegs;
    if (days > 511)
        regs[RTC_DH] |= DAY_OVERFLOW;
    days = days % 512;
    regs[RTC_DL] = days & 0xFF;
    regs[RTC_DH] = regs[RTC_DH] | ((days&0x100) >> 8);
}

void RTC::RegSelect(int numReg) {
    m_selectedReg = numReg;
}

int RTC::RegRead() {
    return m_latchedRegs[m_selectedReg];
}

void RTC::RegWrite(int value) {
    Update(m_refRegs);
    m_refRegs[m_selectedReg] = value;
    m_refTime = time(NULL);
}

void RTC::SetLatchData(int value) {
    if ((m_latchData == 0) && (value == 1))
        Update(m_latchedRegs);
    
    m_latchData = value;
}

void RTC::SetFileData(int *data) {
    m_refRegs[RTC_S]  = data[0];
    m_refRegs[RTC_M]  = data[1];
    m_refRegs[RTC_H]  = data[2];
    m_refRegs[RTC_DL] = data[3];
    m_refRegs[RTC_DH] = data[4];
    
    m_latchedRegs[RTC_S]  = data[5];
    m_latchedRegs[RTC_M]  = data[6];
    m_latchedRegs[RTC_H]  = data[7];
    m_latchedRegs[RTC_DL] = data[8];
    m_latchedRegs[RTC_DH] = data[9];
    
    m_refTime = data[10];
}

void RTC::GetFileData(int *data) {
    time_t now = time(NULL);
    int regs[5] = {0};
    Update(regs);
    
    data[0] = regs[RTC_S];
    data[1] = regs[RTC_M];
    data[2] = regs[RTC_H];
    data[3] = regs[RTC_DL];
    data[4] = regs[RTC_DH];
    
    data[5] = m_latchedRegs[RTC_S];
    data[6] = m_latchedRegs[RTC_M];
    data[7] = m_latchedRegs[RTC_H];
    data[8] = m_latchedRegs[RTC_DL];
    data[9] = m_latchedRegs[RTC_DH];
    
    data[10] = now;
    data[11] = 0;
}

void RTC::SaveState(ofstream *file) {
    file->write((char *)m_latchedRegs,  sizeof(int)*5);
    file->write((char *)&m_selectedReg, sizeof(int));
    file->write((char *)&m_latchData,   sizeof(int));
    file->write((char *)m_refRegs,      sizeof(int)*5);
    file->write((char *)&m_refTime,     sizeof(time_t));
}

void RTC::LoadState(ifstream *file) {
    file->read((char *)m_latchedRegs,  sizeof(int)*5);
    file->read((char *)&m_selectedReg, sizeof(int));
    file->read((char *)&m_latchData,   sizeof(int));
    file->read((char *)m_refRegs,      sizeof(int)*5);
    file->read((char *)&m_refTime,     sizeof(time_t));
}
