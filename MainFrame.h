#pragma once

#include <wx/wx.h>
#include <wx/filepicker.h>
#include <wx/spinctrl.h>
#include <wx/gauge.h>
#include <vector>

class MainFrame : public wxFrame
{
public:
    MainFrame(const wxString& title);

private:
    wxFilePickerCtrl* inputPicker{};
    wxFilePickerCtrl* outputPicker{};

    wxSpinCtrl* chunkSpinMB{};
    wxSpinCtrl* sampleCountSpin{};

    wxGauge* progressBar{};
    wxTextCtrl* logCtrl{};

    wxButton* btnGenerate{};
    wxButton* btnPreviewInput{};
    wxButton* btnPreviewOutput{};
    wxButton* btnStart{};
    wxButton* btnClearLog{};

private:
    void BuildUI();
    void Log(const wxString& msg);
    void PreviewFile(const wxString& path, const wxString& label);
    std::vector<double> ReadAllDoubles(const wxString& path);

    void OnGenerateSample(wxCommandEvent& event);
    void OnPreviewInput(wxCommandEvent& event);
    void OnPreviewOutput(wxCommandEvent& event);
    void OnStart(wxCommandEvent& event);
    void OnClearLog(wxCommandEvent& event);
};