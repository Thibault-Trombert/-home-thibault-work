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

#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__

#include <vector>

class wxJoystick;

class Joystick
{
public:
	Joystick();
    ~Joystick();
    
    void UpdateButtonsState(bool buttonsState[8]);
    
private:
    int m_numJoysticks;
    wxJoystick *m_joystick;
    std::vector<wxJoystick *> m_allJoysCreated;
    
    void CheckChanges();
    void UpdateXYAxes(bool buttonsState[8]);
    void UpdatePOV(bool buttonsState[8]);
    void UpdateGeneric(bool buttonsState[8]);
    void UpdateXbox360(bool buttonsState[8]);
};

#endif