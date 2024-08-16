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

#include <wx/wx.h>
#include <wx/joystick.h>
#include "Joystick.h"

#if !wxUSE_JOYSTICK
#   error You must set wxUSE_JOYSTICK to 1 in setup.h
#endif

int Xbox360Buttons[] = { 0x800, 0x1000, 0x2000, 0x4000, 0x01, 0x02, 0x200, 0x100 };

using namespace std;

Joystick::Joystick() {
    m_numJoysticks = 0;
    m_joystick = NULL;
}

// Se eliminan las instacias de wxJoystick creadas. Aun así estas
// instancias han creado hilos que no serán cerrados
Joystick::~Joystick() {
    for (int i=0; i<m_allJoysCreated.size(); i++)
        delete m_allJoysCreated[i];
}

// Esta funcion comprueba la conexion/desconexion de joysticks. La
// unica manera de usar el nuevo joystick es crear una nueva instancia.
// El problema es que las antiguas instancias no se pueden eliminar en
// el mismo momento porque hace que se cuelgue la aplicación.
void Joystick::CheckChanges() {
    int numJoysticks = wxJoystick::GetNumberJoysticks();
    if (numJoysticks != m_numJoysticks) {
        if (numJoysticks > 0) {
            m_joystick = new wxJoystick();
            wxPrintf("%s (0x%X, 0x%X)\n", m_joystick->GetProductName(), m_joystick->GetManufacturerId(), m_joystick->GetProductId());
            m_allJoysCreated.push_back(m_joystick);
        } else
            m_joystick = NULL;
        m_numJoysticks = numJoysticks;
    }
}

void Joystick::UpdateButtonsState(bool buttonsState[8]) {
    CheckChanges();

    if (m_joystick) {
        if ((m_joystick->GetManufacturerId() == 0x45E) && (m_joystick->GetProductId() == 0x28E))
            UpdateXbox360(buttonsState);
        else
            UpdateGeneric(buttonsState);
    }
}

void Joystick::UpdateXYAxes(bool buttonsState[8]) {
    wxPoint xy = m_joystick->GetPosition();
    
    buttonsState[0] |= (xy.y < m_joystick->GetYMin()/2);
    buttonsState[1] |= (xy.y > m_joystick->GetYMax()/2);
    buttonsState[2] |= (xy.x < m_joystick->GetXMin()/2);
    buttonsState[3] |= (xy.x > m_joystick->GetXMax()/2);
}

void Joystick::UpdatePOV(bool buttonsState[8]) {
    const int offset = 3000;
    int pov = m_joystick->GetPOVCTSPosition();
    if (pov >= 0) {
        buttonsState[0] |= ((pov < ( 9000-offset)) || (pov >= (27000+offset)));
        buttonsState[1] |= ((pov < (27000-offset)) || (pov >= ( 9000+offset)));
        buttonsState[2] |= ((pov < (36000-offset)) || (pov >= (18000+offset)));
        buttonsState[3] |= ((pov < (18000-offset)) || (pov >= (    0+offset)));
    }
}

void Joystick::UpdateGeneric(bool buttonsState[8]) {
    UpdateXYAxes(buttonsState);
    UpdatePOV(buttonsState);
    
    int pressed = m_joystick->GetButtonState();
    for (int i=0; i<4; i++)
        buttonsState[i+4] |= (pressed & (1 << i)) != 0;
}

void Joystick::UpdateXbox360(bool buttonsState[8]) {
    UpdateXYAxes(buttonsState);
    
    int pressed = m_joystick->GetButtonState();
    for (int i=0; i<8; i++)
        buttonsState[i] |= (pressed & Xbox360Buttons[i]) != 0;
}
