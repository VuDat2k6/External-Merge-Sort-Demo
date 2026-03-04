#pragma once

#include <wx/wx.h>
#include <wx/timer.h>
#include "VisualizerPanel.h"
#include "DemoController.h"

class DemoFrame : public wxFrame
{
public:
    DemoFrame(wxWindow* parent,
        const std::vector<double>& inputValues,
        const wxString& outputPath);

private:
    VisualizerPanel* vizPanel{};
    wxStaticText* descLabel{};

    wxButton* btnPrev{};
    wxButton* btnNext{};
    wxButton* btnAuto{};
    wxButton* btnClose{};

    DemoController demo;
    wxTimer autoTimer;
    bool autoRunning = false;

    wxString outputFilePath;

private:
    void RefreshView();
    void WriteOutputIfDone();

    void OnPrev(wxCommandEvent& event);
    void OnNext(wxCommandEvent& event);
    void OnAuto(wxCommandEvent& event);
    void OnCloseWindow(wxCommandEvent& event);
    void OnAutoTimer(wxTimerEvent& event);
};