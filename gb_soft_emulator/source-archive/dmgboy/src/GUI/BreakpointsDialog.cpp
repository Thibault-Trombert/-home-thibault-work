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


#include <wx/listctrl.h>
#include "IDControls.h"
#include "BreakpointsDialog.h"
#include "../Debugger.h"

using namespace std;

BEGIN_EVENT_TABLE(BreakpointsDialog, wxDialog)
EVT_BUTTON(ID_BREAK_ADD, BreakpointsDialog::OnAddBreakpoint)
EVT_BUTTON(ID_BREAK_DEL, BreakpointsDialog::OnDelBreakpoint)
EVT_LIST_ITEM_SELECTED(ID_BREAK_LIST, BreakpointsDialog::OnItemSelected)
END_EVENT_TABLE()

BreakpointsDialog::BreakpointsDialog(wxWindow *parent, Debugger *debugger)
{
    this->Create(parent, wxID_ANY, wxT("Breakpoints"), wxDefaultPosition,
           wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
    
    m_debugger = debugger;
    
    wxButton *addButton = new wxButton(this, ID_BREAK_ADD, wxT("Add"));
    wxButton *delButton = new wxButton(this, ID_BREAK_DEL, wxT("Delete"));
    
    m_breakpointsView = new wxListView(this, ID_BREAK_LIST, wxDefaultPosition, wxSize(170, 132), wxLC_REPORT);
    m_breakpointsView->InsertColumn (0, "Breakpoints");
    m_breakpointsView->SetColumnWidth (0, 170);
    
    wxTextValidator *validator = new wxTextValidator(wxFILTER_INCLUDE_CHAR_LIST);
    validator->SetCharIncludes(wxT("0123456789ABCDEFabcdef"));
    
#ifdef __WXMSW__
	m_font = new wxFont(8, wxTELETYPE, wxNORMAL, wxNORMAL);
#else
    m_font = new wxFont(12, wxTELETYPE, wxNORMAL, wxNORMAL);
#endif
    
    wxStaticText *addressText = new wxStaticText(this, -1, wxT("Address:"));
    m_addressCtrl = new wxTextCtrl(this, ID_BREAK_ADDRESS, wxEmptyString, wxDefaultPosition, wxSize(40, 20), 0, *validator);
    m_addressCtrl->SetFont(*m_font);
    m_addressCtrl->SetMaxLength(4);
    
    wxSizer *buttonsSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonsSizer->Add(addButton, 0, wxRIGHT, 5);
    buttonsSizer->Add(delButton);
    
    wxSizer *addressSizer = new wxBoxSizer(wxHORIZONTAL);
    addressSizer->AddStretchSpacer();
    addressSizer->Add(addressText, 0, wxRIGHT, 5);
    addressSizer->Add(m_addressCtrl);
    
    wxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(m_breakpointsView, 0, wxALL, 5);
    mainSizer->Add(addressSizer, 0, wxEXPAND|wxALL, 5);
    mainSizer->Add(buttonsSizer, 0, wxALL, 5);
    
    SetSizerAndFit(mainSizer);
    
    UpdateUI();
}

BreakpointsDialog::~BreakpointsDialog()
{
	
}

void BreakpointsDialog::UpdateUI() {
    
    m_breakpointsView->DeleteAllItems();
    int numBreakpoints = m_debugger->GetNumBreakpoints();
    for (int i=0; i<numBreakpoints; i++) {
        WORD address = m_debugger->GetBreakpoint(i);
        string value = m_debugger->ToHex(address, 4, '0');
        m_breakpointsView->InsertItem(i, "");
        m_breakpointsView->SetItem(i, 0, value);
        m_breakpointsView->SetItemFont(i, *m_font);
    }
}

void BreakpointsDialog::OnItemSelected(wxListEvent &event) {
    wxString text = event.GetText();
    m_addressCtrl->SetValue(text);
}

void BreakpointsDialog::OnAddBreakpoint(wxCommandEvent &) {
    wxString address = m_addressCtrl->GetValue();
    long value;
    if(address.ToLong(&value, 16)) {
        value = value & 0xFFFF;
        m_debugger->AddBreakpoint(value);
        UpdateUI();
    }
}

void BreakpointsDialog::OnDelBreakpoint(wxCommandEvent &) {
    wxString address = m_addressCtrl->GetValue();
    long value;
    if(address.ToLong(&value, 16)) {
        value = value & 0xFFFF;
        m_debugger->DelBreakpoint(value);
        UpdateUI();
    }
}
