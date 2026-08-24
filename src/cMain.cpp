#include "pch.h"
#include "cMain.h"
#include"FixFilePerms.h"
#include "inject.h"
#include "cApp.h"
#include "config.h"
#include "icon/icon.xpm"

wxBEGIN_EVENT_TABLE(cMain, wxFrame)
EVT_BUTTON(101, OnInjectButton)
EVT_BUTTON(102, OnHideButton)
EVT_BUTTON(103, OnSelectButton)

EVT_CHECKBOX(201, OnCustomCheckBox)
EVT_CHECKBOX(202, OnAutoCheckBox)

EVT_CHECKBOX(301, cMain::OnTimer)

wxEND_EVENT_TABLE();

bool cheapThreadFix = false;

cMain::cMain() : wxFrame(nullptr, wxID_ANY, "Fate Client Injector", wxDefaultPosition, wxSize(297.5, 162.5), wxMINIMIZE_BOX | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN) {
    wxIcon icon(icon_xpm); // icons are shit to do but the technike is actually quite nice
    this->SetIcon(icon);
    this->SetBackgroundColour(wxColour(255, 255, 255, 255));

    mainPanel = new wxPanel(this, wxID_ANY);
	btn_Inject = new wxButton(mainPanel, 101, "Inject", wxPoint(5, 5), wxSize(100, 40));
   	btn_Hide = new wxButton(mainPanel, 102, "Hide Menu", wxPoint(5, 50), wxSize(100, 20));
    btn_Select = new wxButton(mainPanel, 103, "Select", wxPoint(5, 75), wxSize(60, 20));
	txt_Name = new wxTextCtrl(mainPanel, wxID_ANY, "Minecraft", wxPoint(110, 5), wxSize(165, 20));
	check_Custom = new wxCheckBox(mainPanel, 201, "Custom Target", wxPoint(110, 30), wxSize(165, 20));
	check_Auto = new wxCheckBox(mainPanel, 202, "Auto Inject", wxPoint(110, 50), wxSize(130, 20));
	txt_Delay = new wxTextCtrl(mainPanel, wxID_ANY, "5", wxPoint(245, 50), wxSize(30, 20), wxTE_CENTRE, wxTextValidator(wxFILTER_NUMERIC));
    txt_Delay->SetMaxLength(2);
	txt_Path = new wxTextCtrl(mainPanel, wxID_ANY, "Click \"Select\" to select the dll file", wxPoint(70, 75), wxSize(205, 20));
    CreateStatusBar(1);
    SetStatusText("Version 1.0 | Made by youtube.com/fligger", 0);

    
    check_Custom->SetValue(customProcName);
    check_Auto->SetValue(autoInject);
    txt_Delay->SetLabel(delaystr);
    txt_Path->SetLabel(dllPath);
    if (customProcName) {
        txt_Name->SetLabel(titleName);
    }
    else {
        txt_Name->Disable();
    }
    if (check_Auto->IsChecked()) {
        txt_Name->Disable();
        txt_Path->Disable();
        txt_Delay->Disable();
        btn_Select->Disable();
        check_Custom->Disable();
        if (!cheapThreadFix) {
            std::thread loopthread(&cMain::loopInject, this); // for autoinject
            loopthread.detach();
        }
    }
    
    taskBarControl.SetIcon(icon, "Double-Click to show Fate Injector");

    notification = new wxNotificationMessage("Fate Client Injector", "Fate Client Injector is now hidden in your system tray", this, 0);
    notification->UseTaskBarIcon(&taskBarControl);
    openDialog = new wxFileDialog(this, "Select the .dll file", working_dir, "Fate.Client.dll" ,"Dynamic link library (*.dll)|*.dll", wxFD_OPEN);
}


cMain::~cMain() {
    openDialog->Destroy();
    notification->Close();
    delete(notification);
}


void cMain::OnInjectButton(wxCommandEvent& evt) {

    cMain::OnInjectButtonExecute(evt, this);
}

void cMain::OnInjectButtonExecute(wxCommandEvent& evt, cMain* ref) {
    std::string debug;

    DWORD procId = GetProcId(ref->txt_Name->GetValue().mb_str());

    if (procId == 0) {
        ref->SetStatusText("Can't find process!", 0);
        return;
    }

    wxString wxStrPath = ref->txt_Path->GetValue();
    std::wstring wStrPath = wxStrPath.ToStdWstring(); // converting wxstr to wstr
    std::ifstream test(wStrPath.c_str()); // test if file path is valid
    if (!test) {
        debug = "Process found! | " + std::to_string(procId) + " | invalid file path";
        ref->SetStatusText("Process found! | " + std::to_string(procId) + " | invalid file path", 0);
        return;
    }

    SetAccessControl(wStrPath, L"S-1-15-2-1");
    int res = performInjection(procId, wStrPath.c_str());
    if (res == 0) {
        debug = "Process found! | " + std::to_string(procId) + " | Injected!";
    } else {
        debug = "Injection failed! Error Code: " + std::to_string(res) + " | PID: " + std::to_string(procId);
    }
    
    ref->SetStatusText(debug, 0);
    ref->saveConfigFromUi();

	evt.Skip();
}


void cMain::OnHideButton(wxCommandEvent& evt) {

    notification->Show();
    this->Hide();
    setHiddenState(true);

    // Fate Client Injector is now hidden in system tray
	evt.Skip();
}
void cMain::OnSelectButton(wxCommandEvent& evt) {


    if (openDialog->ShowModal() == wxID_OK) // if the user click "Open" instead of "Cancel"
    {
        txt_Path->SetLabel(openDialog->GetPath());
    }

	evt.Skip();
}

void cMain::OnCustomCheckBox(wxCommandEvent& evt) {


    if (check_Custom->IsChecked()) {
        txt_Name->Enable(true);
    }
    else {
        txt_Name->Enable(false);
        txt_Name->SetLabel("Minecraft");
    }
    evt.Skip();
}


void cMain::OnAutoCheckBox(wxCommandEvent& evt) {
    if (check_Auto->IsChecked()) {
        txt_Name->Disable();
        txt_Path->Disable();
        txt_Delay->Disable();
        btn_Select->Disable();
        check_Custom->Disable();
        if (!cheapThreadFix) {
            std::thread loopthread(&cMain::loopInject, this); // for autoinject
            loopthread.detach();
        }
    }
    else {
        m_timer.Stop();
        disableAutoInject();
    }
    saveConfigFromUi();

    evt.Skip();
}

void cMain::disableAutoInject() {
    check_Custom->Enable();
    if (check_Custom->IsChecked()) {
        txt_Name->Enable();
    }
    txt_Path->Enable();
    txt_Delay->Enable();
    btn_Select->Enable();
}

void cMain::saveConfigState() {
    config cfg;
    cfg.saveConfig();
}

void cMain::saveConfigFromUi() {
    customProcName = check_Custom->GetValue();
    autoInject = check_Auto->GetValue();
    delaystr = txt_Delay->GetValue();
    dllPath = txt_Path->GetValue();
    titleName = txt_Name->GetValue();
    saveConfigState();
}

void cMain::setHiddenState(bool hidden) {
    hideMenu = hidden;
    saveConfigState();
}

bool cMain::loopInject() {
    cheapThreadFix = true;
    std::string debug;

    int delay = atoi(txt_Delay->GetValue().mb_str());
    if (delay <= 1) delay = 1;

        txt_Delay->SetLabel("1");
        debug = "AutoInject: Enabled | trying every second";
    }
    else {
        debug = "AutoInject: Enabled | trying every " + std::to_string(delay) + " seconds";
    }
    SetStatusText(debug, 0);

    DWORD procId = 0;
    DWORD oldProcId = 0;
    int delayMs = delay * 1000;

    while (autoInject) {
        while (procId == oldProcId || procId == 0) {
            Sleep(delayMs);
            if (!autoInject) {
                cheapThreadFix = false;
                return false;
            }

            std::string targetTitle(titleName.begin(), titleName.end());
            procId = GetProcId(targetTitle.c_str());

            if (procId == 0) {
                SetStatusText("AutoInject: Can't find process!", 0);
            }
            else if (procId == oldProcId) {
                SetStatusText("AutoInject: Already Injected! | " + std::to_string(procId), 0);
            }
        }

        wxString wxStrPath = dllPath;
        std::ifstream test(wStrPath.c_str()); // test if file path is valid
        if (!test) {
            SetStatusText("AutoInject: Invalid DLL file path!", 0);
            cheapThreadFix = false;
            return true;
        }

        SetAccessControl(wStrPath, L"S-1-15-2-1");

        int res = performInjection(procId, wStrPath.c_str());
        if (res == 0) {
            oldProcId = procId;
            SetStatusText("AutoInject: Injected successfully! | PID: " + std::to_string(procId), 0);
        }
        else {
            SetStatusText("AutoInject: Failed (Code: " + std::to_string(res) + ") | Retrying... | PID: " + std::to_string(procId), 0);
            procId = 0;
        }
    }

    cheapThreadFix = false;
    return false;
}

