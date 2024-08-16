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
#include <wx/imaglist.h>
#include "IDControls.h"
#include "DebuggerDialog.h"
#include "BreakpointsDialog.h"
#include "../Debugger.h"

#include "Xpm/currentRow.xpm"
#include "Xpm/breakPoint.xpm"
#include "Xpm/currentBreak.xpm"

using namespace std;

BEGIN_EVENT_TABLE(DebuggerDialog, wxDialog)
EVT_BUTTON(ID_DEBUG_RESET, DebuggerDialog::OnReset)
EVT_BUTTON(ID_DEBUG_STEPINTO, DebuggerDialog::OnStepInto)
EVT_BUTTON(ID_DEBUG_ONEFRAME, DebuggerDialog::OnOneFrame)
EVT_BUTTON(ID_DEBUG_ONESECOND, DebuggerDialog::OnOneSecond)
EVT_BUTTON(ID_DEBUG_BREAKPOINTS, DebuggerDialog::OnBreakpoints)
EVT_TEXT(ID_DEBUG_MEMADDRESS, DebuggerDialog::OnMemAddressChange)
EVT_LIST_ITEM_ACTIVATED(ID_DEBUG_DISASSEMBLER, DebuggerDialog::OnActivated)
END_EVENT_TABLE()

DebuggerDialog::DebuggerDialog(wxWindow *parent, Debugger *debugger)
{
    this->Create(parent, ID_DEBUGGERDIALOG, wxT("Debugger"), wxDefaultPosition,
           wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
    
    m_debugger = debugger;
    
    wxButton *resetButton = new wxButton(this, ID_DEBUG_RESET, wxT("Reset"));
    wxButton *stepIntoButton = new wxButton(this, ID_DEBUG_STEPINTO, wxT("Step Into"));
    wxButton *oneFrameButton = new wxButton(this, ID_DEBUG_ONEFRAME, wxT("Run one frame"));
    wxButton *oneSecondButton = new wxButton(this, ID_DEBUG_ONESECOND, wxT("Run one second"));
    wxButton *breakpointsButton = new wxButton(this, ID_DEBUG_BREAKPOINTS, wxT("Breakpoints"));
    
    stepIntoButton->SetToolTip("Step Into (F7)");
#ifdef __WXMAC__
    resetButton->SetToolTip("Reset (Cmd+R)");
    oneFrameButton->SetToolTip("Run one frame (Cmd+O)");
    oneSecondButton->SetToolTip("Run one second (F9)");
    breakpointsButton->SetToolTip("Breakpoints (Cmd+B)");
#else
    resetButton->SetToolTip("Reset (Ctrl+R)");
    oneFrameButton->SetToolTip("Run one frame (Ctrl+O)");
    oneSecondButton->SetToolTip("Run one second (F9)");
    breakpointsButton->SetToolTip("Breakpoints (Ctrl+B)");
#endif
    
    
    wxAcceleratorEntry entries[5];
    entries[0].Set(wxACCEL_CTRL, (int) 'R', ID_DEBUG_RESET);
    entries[1].Set(wxACCEL_NORMAL, WXK_F7, ID_DEBUG_STEPINTO);
    entries[2].Set(wxACCEL_CTRL, (int) 'O', ID_DEBUG_ONEFRAME);
    entries[3].Set(wxACCEL_NORMAL, WXK_F9, ID_DEBUG_ONESECOND);
    entries[4].Set(wxACCEL_CTRL, (int) 'B', ID_DEBUG_BREAKPOINTS);
    wxAcceleratorTable accel(5, entries);
    SetAcceleratorTable(accel);
    
    wxStaticText *regsText = new wxStaticText(this, -1, wxT("Registers:"));
    
#ifdef __WXMSW__
	m_font = new wxFont(8, wxTELETYPE, wxNORMAL, wxNORMAL);
    int height1 = 152;
    int height2 = 360;
#endif
#ifdef __WXGTK__
	m_font = new wxFont(8, wxTELETYPE, wxNORMAL, wxNORMAL);
    int height1 = 166;
    int height2 = 420;
#endif
#ifdef __WXMAC__
    m_font = new wxFont(12, wxTELETYPE, wxNORMAL, wxNORMAL);
    int height1 = 132;
    int height2 = 340;
#endif
    
    m_regsView = new wxListView(this, wxID_ANY, wxDefaultPosition, wxSize(82, height1), wxLC_REPORT);
    m_regsView->InsertColumn (0, "Name");
    m_regsView->SetColumnWidth (0, 41);
    m_regsView->InsertColumn (1, "Value");
    m_regsView->SetColumnWidth (1, 41);
    
    // --- Dissassembler ---
    wxStaticText *disassemblerText = new wxStaticText(this, -1, wxT("Disassembler:"));
    
    m_disassemblerView = new wxListView(this, ID_DEBUG_DISASSEMBLER, wxDefaultPosition, wxSize(298, height1), wxLC_REPORT);
    
    wxImageList *imageList = new wxImageList(16, 14);
    wxBitmap bmpCurrentRow(currentRow_xpm);
    imageList->Add(bmpCurrentRow);
    wxBitmap bmpBreakpoint(breakPoint_xpm);
    imageList->Add(bmpBreakpoint);
    wxBitmap bmpCurrentBreak(currentBreak_xpm);
    imageList->Add(bmpCurrentBreak);
    m_disassemblerView->SetImageList(imageList, wxIMAGE_LIST_SMALL);
    
    m_disassemblerView->InsertColumn (0, "");
    m_disassemblerView->SetColumnWidth (0, 26);
    m_disassemblerView->InsertColumn (1, "Address");
    m_disassemblerView->SetColumnWidth (1, 52);
    m_disassemblerView->InsertColumn (2, "Name");
    m_disassemblerView->SetColumnWidth (2, 130);
    m_disassemblerView->InsertColumn (3, "Data");
    m_disassemblerView->SetColumnWidth (3, 90);
    
    wxStaticText *videoText = new wxStaticText(this, -1, wxT("Video registers:"));
    m_videoView = new wxListView(this, wxID_ANY, wxDefaultPosition, wxSize(130, height2), wxLC_REPORT);
    m_videoView->InsertColumn (0, "Name");
    m_videoView->SetColumnWidth (0, 44);
    m_videoView->InsertColumn (1, "Address");
    m_videoView->SetColumnWidth (1, 49);
    m_videoView->InsertColumn (2, "Value");
    m_videoView->SetColumnWidth (2, 37);
    
    wxStaticText *othersText = new wxStaticText(this, -1, wxT("Other registers:"));
    m_othersView = new wxListView(this, wxID_ANY, wxDefaultPosition, wxSize(130, height2), wxLC_REPORT);
    m_othersView->InsertColumn (0, "Name");
    m_othersView->SetColumnWidth (0, 44);
    m_othersView->InsertColumn (1, "Address");
    m_othersView->SetColumnWidth (1, 49);
    m_othersView->InsertColumn (2, "Value");
    m_othersView->SetColumnWidth (2, 37);
    
    wxTextValidator *validator = new wxTextValidator(wxFILTER_INCLUDE_CHAR_LIST);
    validator->SetCharIncludes(wxT("0123456789ABCDEFabcdef"));
    
    wxStaticText *memText = new wxStaticText(this, -1, wxT("Memory:"));
    m_addressMemCtrl = new wxTextCtrl(this, ID_DEBUG_MEMADDRESS, wxEmptyString, wxDefaultPosition, wxSize(40, 20), 0, *validator);
    m_addressMemCtrl->SetFont(*m_font);
    m_addressMemCtrl->SetValue(wxT("0000"));
    m_addressMemCtrl->SetMaxLength(4);
    
    m_memCtrl = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxSize(398, 146), wxTE_MULTILINE | wxTE_READONLY);
    m_memCtrl->SetFont(*m_font);
    
    wxSizer *buttonsSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonsSizer->Add(resetButton);
    buttonsSizer->AddSpacer(5);
    buttonsSizer->Add(stepIntoButton, 0, wxLEFT, 2);
    buttonsSizer->Add(oneFrameButton, 0, wxLEFT, 2);
    buttonsSizer->Add(oneSecondButton, 0, wxLEFT, 2);
    buttonsSizer->AddStretchSpacer();
    buttonsSizer->Add(breakpointsButton);
    
    wxSizer *regsSizer = new wxBoxSizer(wxVERTICAL);
    regsSizer->Add(regsText, 0, wxBOTTOM, 5);
    regsSizer->Add(m_regsView);
    
    wxSizer *disassemblerSizer = new wxBoxSizer(wxVERTICAL);
    disassemblerSizer->Add(disassemblerText, 0, wxBOTTOM, 5);
    disassemblerSizer->Add(m_disassemblerView);
    
    wxSizer *RegsPlusDisSizer = new wxBoxSizer(wxHORIZONTAL);
    RegsPlusDisSizer->Add(regsSizer);
    RegsPlusDisSizer->AddStretchSpacer();
    RegsPlusDisSizer->Add(disassemblerSizer);
    
    wxSizer *memSizer = new wxBoxSizer(wxVERTICAL);
    memSizer->Add(memText, 0, wxBOTTOM, 5);
    memSizer->Add(m_addressMemCtrl, 0, wxBOTTOM, 5);
    memSizer->Add(m_memCtrl);
    
    wxSizer *leftSizer = new wxBoxSizer(wxVERTICAL);
    leftSizer->Add(RegsPlusDisSizer, 0, wxEXPAND);
    leftSizer->AddStretchSpacer();
    leftSizer->Add(memSizer, 0, wxTOP, 10);
    
    wxSizer *videoSizer = new wxBoxSizer(wxVERTICAL);
    videoSizer->Add(videoText, 0, wxBOTTOM, 5);
    videoSizer->Add(m_videoView);
    
    wxSizer *othersSizer = new wxBoxSizer(wxVERTICAL);
    othersSizer->Add(othersText, 0, wxBOTTOM, 5);
    othersSizer->Add(m_othersView);
    
    wxSizer *hSizer = new wxBoxSizer(wxHORIZONTAL);
    hSizer->Add(leftSizer, 0, wxEXPAND);
    hSizer->Add(videoSizer, 0, wxLEFT, 10);
    hSizer->Add(othersSizer, 0, wxLEFT, 10);
    
    wxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(buttonsSizer, 0, wxEXPAND|wxALL, 10);
    mainSizer->Add(hSizer, 0, wxALL, 10);
    
    SetSizerAndFit(mainSizer);
    
    UpdateUI();
}

DebuggerDialog::~DebuggerDialog()
{
	
}

void DebuggerDialog::UpdateUI() {
    UpdateRegisters();
    UpdateMemory();
    UpdateDissassembler();
    UpdateVideoRegs();
    UpdateOtherRegs();
}

void DebuggerDialog::UpdateMemory() {
    wxString address = m_addressMemCtrl->GetValue();
    long value;
    if(address.ToLong(&value, 16)) {
        value = value & 0xFFF0;
        int numLines = 9;
        WORD maxStart = 0x10000 - (0x10*numLines);
        if (value > maxStart)
            value = maxStart;
        string mem = "      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n";
        mem += m_debugger->GetMem(value, (value + (0x10*numLines)-1));
        m_memCtrl->SetValue(mem);
    }
}

void DebuggerDialog::UpdateRegisters() {
    const char *names[] = { "PC", "AF", "BC", "DE", "HL", "SP" };
    
    m_regsView->DeleteAllItems();
    
    for (int i=0; i<6; i++) {
        m_regsView->InsertItem(i, "");
        m_regsView->SetItem(i, 0, names[i]);
        m_regsView->SetItemFont(i, *m_font);
    }
    
    m_regsView->SetItem(0, 1, m_debugger->GetRegPC());
    m_regsView->SetItem(1, 1, m_debugger->GetRegAF());
    m_regsView->SetItem(2, 1, m_debugger->GetRegBC());
    m_regsView->SetItem(3, 1, m_debugger->GetRegDE());
    m_regsView->SetItem(4, 1, m_debugger->GetRegHL());
    m_regsView->SetItem(5, 1, m_debugger->GetRegSP());
}

void DebuggerDialog::UpdateVideoRegs() {
    const char *names[]    = { "LCDC", "STAT", "SCY", "SCX", "LY", "LYC", "DMA", "BGP", "OBP0", "OBP1", "WY", "WX", "BGPI", "BGPD", "OBPI", "OBPD" };
    const WORD addresses[] = {  LCDC,   STAT,   SCY,   SCX,   LY,   LYC,   DMA,   BGP,   OBP0,   OBP1,   WY,   WX,   BGPI,   BGPD,   OBPI,   OBPD  };
    
    m_videoView->DeleteAllItems();
    
    for (int i=0; i<16; i++) {
        m_videoView->InsertItem(i, "");
        m_videoView->SetItem(i, 0, names[i]);
        m_videoView->SetItem(i, 1, m_debugger->ToHex(addresses[i], 4, '0'));
        m_videoView->SetItem(i, 2, m_debugger->GetMem(addresses[i]));
        m_videoView->SetItemFont(i, *m_font);
    }
}

void DebuggerDialog::UpdateOtherRegs() {
    const char *names[]    = { "P1", "SB", "SC", "DIV", "TIMA", "TMA", "TAC", "KEY1", "VBK", "HDMA1", "HDMA2", "HDMA3", "HDMA4", "HDMA5", "SVBK", "IF", "IE" };
    const WORD addresses[] = {  P1,   SB,   SC,   DIV,   TIMA,   TMA,   TAC,   KEY1,   VBK,   HDMA1,   HDMA2,   HDMA3,   HDMA4,   HDMA5,   SVBK,   IF,   IE  };
    
    m_othersView->DeleteAllItems();
    
    for (int i=0; i<17; i++) {
        m_othersView->InsertItem(i, "");
        m_othersView->SetItem(i, 0, names[i]);
        m_othersView->SetItem(i, 1, m_debugger->ToHex(addresses[i], 4, '0'));
        m_othersView->SetItem(i, 2, m_debugger->GetMem(addresses[i]));
        m_othersView->SetItemFont(i, *m_font);
    }
}

void DebuggerDialog::UpdateDissassembler() {
    WORD currentAddress, nextAddress;
    string address, name, data;
    
    m_debugger->DisassembleNext(currentAddress, nextAddress, name, data);
    
    m_disassemblerView->DeleteAllItems();
    for (int i=0; i<6; i++) {
        address = m_debugger->ToHex(currentAddress, 4, '0');
        
        m_disassemblerView->InsertItem(i, "");
        if (i == 0) {
            if (m_debugger->HasBreakpoint(currentAddress))
                m_disassemblerView->SetItemColumnImage(i, 0, 2);
            else
                m_disassemblerView->SetItemColumnImage(i, 0, 0);
        }
        else if (m_debugger->HasBreakpoint(currentAddress))
            m_disassemblerView->SetItemColumnImage(i, 0, 1);
		else
			m_disassemblerView->SetItemColumnImage(i, 0, -1);
            
        m_disassemblerView->SetItem(i, 1, address);
        m_disassemblerView->SetItem(i, 2, name);
        m_disassemblerView->SetItem(i, 3, data);
        m_disassemblerView->SetItemFont(i, *m_font);
        
        if (nextAddress < currentAddress)
            break;
        else {
            currentAddress = nextAddress;
            m_debugger->DisassembleOne(currentAddress, nextAddress, name, data);
        }
    }
}

void DebuggerDialog::OnReset(wxCommandEvent &event) {
    m_debugger->Reset();
    UpdateUI();
}

void DebuggerDialog::OnStepInto(wxCommandEvent &event) {
    m_debugger->StepInto();
    UpdateUI();
}

void DebuggerDialog::OnOneFrame(wxCommandEvent &event) {
    m_debugger->ExecuteOneFrame();
    UpdateUI();
}

void DebuggerDialog::OnOneSecond(wxCommandEvent &event) {
    for (int i=0; i<60; i++) {
        if (!m_debugger->ExecuteOneFrame())
            break;
    }
    
    UpdateUI();
}

void DebuggerDialog::OnBreakpoints(wxCommandEvent &event) {
    BreakpointsDialog breakpoints(this, m_debugger);
    breakpoints.ShowModal();
    UpdateUI();
}

void DebuggerDialog::OnMemAddressChange(wxCommandEvent &event) {
    if(!m_addressMemCtrl->IsModified())
        return;
    
    long insertionPoint = m_addressMemCtrl->GetInsertionPoint();
    wxString address = m_addressMemCtrl->GetValue().Upper();
    m_addressMemCtrl->ChangeValue(address);
    m_addressMemCtrl->SetInsertionPoint(insertionPoint);
    UpdateUI();
}

void DebuggerDialog::OnActivated(wxListEvent &event) {
    long index = event.GetIndex();
    wxString address = m_disassemblerView->GetItemText(index, 1);
    long value;
    if(address.ToLong(&value, 16)) {
        value = value & 0xFFFF;
        if (m_debugger->HasBreakpoint(value))
            m_debugger->DelBreakpoint(value);
        else
            m_debugger->AddBreakpoint(value);
        UpdateDissassembler();
    }
}
