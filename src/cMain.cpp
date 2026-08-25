#include "pch.h"
#include "cMain.h"
#include "FixFilePerms.h"
#include "inject.h"
#include "cApp.h"
#include "config.h"
#include "icon/icon.xpm"

wxBEGIN_EVENT_TABLE(cMain, wxFrame)
EVT_BUTTON(101, cMain::OnInjectButton)
EVT_BUTTON(102, cMain::OnHideButton)
EVT_BUTTON(103, cMain::OnSelectButton)

EVT_CHECKBOX(201, cMain::OnCustomCheckBox)
EVT_CHECKBOX(202, cMain::OnAutoCheckBox)

EVT_TIMER(501, cMain::OnAutoInjectTimer)
EVT_CLOSE(cMain::OnClose)

wxEND_EVENT_TABLE();

cMain::cMain() : wxFrame(nullptr, wxID_ANY, "Fate Client Injector", wxDefaultPosition, wxSize(297.5, 162.5), wxMINIMIZE_BOX | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN),
                 autoInjectTimer(this, 501) {
    wxIcon icon(icon_xpm);
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
    txt_Delay->SetValue(delaystr);
    txt_Path->SetValue(dllPath);
    if (customProcName) {
        txt_Name->SetValue(titleName);
    }
    else {
        txt_Name->SetValue("Minecraft");
        txt_Name->Disable();
    }
    if (check_Auto->IsChecked()) {
        txt_Name->Disable();
        txt_Path->Disable();
        txt_Delay->Disable();
        btn_Select->Disable();
        check_Custom->Disable();
        startAutoInject();
    }
    
    taskBarControl.SetIcon(icon, "Double-Click to show Fate Injector");

    notification = new wxNotificationMessage("Fate Client Injector", "Fate Client Injector is now hidden in your system tray", this, 0);
    notification->UseTaskBarIcon(&taskBarControl);
    openDialog = new wxFileDialog(this, "Select the .dll file", working_dir, "Fate.Client.dll", "Dynamic link library (*.dll)|*.dll", wxFD_OPEN);
}

cMain::~cMain() {
    stopAutoInject();
    if (openDialog) {
        openDialog->Destroy();
    }
    if (notification) {
        notification->Close();
        delete notification;
        notification = nullptr;
    }
}

void cMain::OnClose(wxCloseEvent& evt) {
    stopAutoInject();
    saveConfigFromUi();
    evt.Skip();
}

void cMain::OnInjectButton(wxCommandEvent& evt) {
    cMain::OnInjectButtonExecute(evt, this);
}

void cMain::OnInjectButtonExecute(wxCommandEvent& evt, cMain* ref) {
    std::string debug;

    std::string targetTitle = ref->txt_Name->GetValue().ToStdString();
    DWORD procId = GetProcId(targetTitle.c_str(), false);

    if (procId == 0) {
        ref->SetStatusText("Can't find process!", 0);
        return;
    }

    if (lastInjectedPid != 0 && lastInjectedPid == procId && IsProcessAlive(lastInjectedPid)) {
        ref->SetStatusText("Process found! | " + std::to_string(procId) + " | Already Injected!", 0);
        ref->saveConfigFromUi();
        return;
    }

    wxString wxStrPath = ref->txt_Path->GetValue();
    std::wstring wStrPath = wxStrPath.ToStdWstring(); // converting wxstr to wstr
    std::ifstream test(wStrPath.c_str()); // test if file path is valid
    if (!test) {
        debug = "Process found! | " + std::to_string(procId) + " | invalid file path";
        ref->SetStatusText(debug, 0);
        return;
    }

    SetAccessControl(wStrPath, L"S-1-15-2-1");
    int res = performInjection(procId, wStrPath.c_str());
    if (res == 0) {
        lastInjectedPid = procId;
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
	evt.Skip();
}

void cMain::OnSelectButton(wxCommandEvent& evt) {
    if (openDialog->ShowModal() == wxID_OK)
    {
        txt_Path->SetValue(openDialog->GetPath());
        saveConfigFromUi();
    }

	evt.Skip();
}

void cMain::OnCustomCheckBox(wxCommandEvent& evt) {
    if (check_Custom->IsChecked()) {
        txt_Name->Enable(true);
    }
    else {
        txt_Name->Enable(false);
        txt_Name->SetValue("Minecraft");
    }
    saveConfigFromUi();
    evt.Skip();
}

void cMain::OnAutoCheckBox(wxCommandEvent& evt) {
    if (check_Auto->IsChecked()) {
        txt_Name->Disable();
        txt_Path->Disable();
        txt_Delay->Disable();
        btn_Select->Disable();
        check_Custom->Disable();
        saveConfigFromUi();
        startAutoInject();
        checkAndAutoInject();
    }
    else {
        stopAutoInject();
        disableAutoInject();
        saveConfigFromUi();
    }

    evt.Skip();
}

void cMain::startAutoInject() {
    autoInject = true;
    long delayVal = 5;
    txt_Delay->GetValue().ToLong(&delayVal);
    if (delayVal < 1) delayVal = 1;
    autoInjectTimer.Start(static_cast<int>(delayVal * 1000));
}

void cMain::stopAutoInject() {
    autoInject = false;
    if (autoInjectTimer.IsRunning()) {
        autoInjectTimer.Stop();
    }
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

void cMain::OnAutoInjectTimer(wxTimerEvent& evt) {
    if (!autoInject) {
        stopAutoInject();
        return;
    }
    checkAndAutoInject();
}

void cMain::checkAndAutoInject() {
    std::string targetTitle;
    if (check_Custom->IsChecked()) {
        targetTitle = txt_Name->GetValue().ToStdString();
    } else {
        targetTitle = "Minecraft";
    }

    if (targetTitle.empty()) {
        SetStatusText("AutoInject: Target name is empty!", 0);
        return;
    }

    // Check if previously injected process is still alive
    if (lastInjectedPid != 0 && IsProcessAlive(lastInjectedPid)) {
        SetStatusText("AutoInject: Already Injected! | " + std::to_string(lastInjectedPid), 0);
        return;
    }

    // If previously injected process has exited, reset lastInjectedPid
    if (lastInjectedPid != 0 && !IsProcessAlive(lastInjectedPid)) {
        lastInjectedPid = 0;
    }

    DWORD procId = GetProcId(targetTitle.c_str(), false);
    if (procId == 0) {
        SetStatusText("AutoInject: Can't find process!", 0);
        return;
    }

    if (procId == lastInjectedPid) {
        SetStatusText("AutoInject: Already Injected! | " + std::to_string(procId), 0);
        return;
    }

    wxString wxStrPath = txt_Path->GetValue();
    std::wstring wStrPath = wxStrPath.ToStdWstring();
    std::ifstream test(wStrPath.c_str());
    if (!test) {
        SetStatusText("AutoInject: Invalid DLL file path!", 0);
        return;
    }

    SetAccessControl(wStrPath, L"S-1-15-2-1");

    int res = performInjection(procId, wStrPath.c_str());
    if (res == 0) {
        lastInjectedPid = procId;
        SetStatusText("AutoInject: Injected successfully! | PID: " + std::to_string(procId), 0);
    }
    else {
        SetStatusText("AutoInject: Failed (Code: " + std::to_string(res) + ") | Retrying... | PID: " + std::to_string(procId), 0);
    }
}

void cMain::saveConfigState() {
    config cfg;
    cfg.saveConfig();
}

void cMain::saveConfigFromUi() {
    customProcName = check_Custom->GetValue();
    autoInject = check_Auto->GetValue();
    delaystr = txt_Delay->GetValue().ToStdWstring();
    dllPath = txt_Path->GetValue().ToStdWstring();
    titleName = txt_Name->GetValue().ToStdWstring();
    saveConfigState();
}

void cMain::setHiddenState(bool hidden) {
    hideMenu = hidden;
    saveConfigState();
}

