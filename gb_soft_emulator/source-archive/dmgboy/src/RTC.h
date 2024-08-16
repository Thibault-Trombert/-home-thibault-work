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

#ifndef __RTC_H__
#define __RTC_H__

class RTC {
public:
    RTC();
    void RegSelect(int numReg);
    int  RegRead();
    void RegWrite(int value);
    void SetLatchData(int value);
    
    void SetFileData(int *data);
    void GetFileData(int *data);
    void SaveState(std::ofstream *file);
    void LoadState(std::ifstream *file);
    
private:
    void Update(int *regs);
    void Add(int seconds, int *regs);
    
    int  m_latchedRegs[5];
    int  m_selectedReg;
    int  m_latchData;
    
    int  m_refRegs[5];
    time_t m_refTime;
};

#endif
