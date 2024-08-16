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

#ifndef __PAD_H__
#define __PAD_H__

#include "Def.h"

class Pad {
public:
    Pad();
    
    int  SetButtonsState(bool buttonsState[8], BYTE *valueP1);
    BYTE Update(BYTE valueP1);
    
private:
    bool m_buttonsState[8];
};

#endif
