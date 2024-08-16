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

#ifndef __EMULATIONTHREAD_H__
#define __EMULATIONTHREAD_H__

#include <wx/thread.h>
#include <wx/stopwatch.h>

enum enumEmuStates { NotStartedYet, Stopped, Paused, Playing };

class Cartridge;
class Video;
class Sound;
class Pad;
class CPU;
class Debugger;
class Joystick;
class IGBScreenDrawable;
class wxMutex;

class EmulationThread : public wxThread
{
public:
	EmulationThread();
    ~EmulationThread();
    
    virtual ExitCode Entry();
    
    bool ChangeFile(wxString fileName);
    void LoadState(std::string fileName, int id);
    void SaveState(std::string fileName, int id);
    void ApplySettings();
    void SetScreen(IGBScreenDrawable * screen);
    void UpdatePad();
    Debugger *GetDebugger();
    
    enumEmuStates GetState();
    void SetState(enumEmuStates state);
    bool Finished();
    
private:
	Video *video;
	Sound *sound;
    Pad *pad;
	Cartridge *cartridge;
    CPU *cpu;
    Debugger *debugger;
    Joystick *joystick;
    wxMutex *mutex;
    wxStopWatch swFrame;
    wxKeyCode keysUsed[8];
    bool m_finished;
    
	enumEmuStates emuState;
    
    void LoadZip(const wxString zipPath, BYTE ** buffer, unsigned long * size);
    void PadSetKeys(int* keys);
};

#endif
