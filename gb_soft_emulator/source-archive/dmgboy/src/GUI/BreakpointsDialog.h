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

#ifndef __BREAKPOINTSDIALOG_H__
#define __BREAKPOINTSDIALOG_H__

#include <wx/wx.h>

class Debugger;
class wxListView;

/*******************************************************************************
 // BreakpointsDialog Class
 *******************************************************************************/

class BreakpointsDialog : public wxDialog {
public:
	
    BreakpointsDialog(wxWindow *parent, Debugger *debugger);
	~BreakpointsDialog();
    
protected:
	DECLARE_EVENT_TABLE()
    
private:
    Debugger *m_debugger;
    wxFont *m_font;
    wxListView *m_breakpointsView;
    wxTextCtrl *m_addressCtrl;
    
    void UpdateUI();
    
    void OnItemSelected(wxListEvent &);
    void OnAddBreakpoint(wxCommandEvent &);
    void OnDelBreakpoint(wxCommandEvent &);
};

#endif
