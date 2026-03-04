#include "DemoFrame.h"
#include <wx/artprov.h>
#include <fstream>

DemoFrame::DemoFrame(wxWindow* parent,
    const std::vector<double>& inputValues,
    const wxString& outputPath)
    : wxFrame(parent, wxID_ANY, "K-way External Merge Sort Demo",
        wxDefaultPosition, wxSize(1080, 780)),
    autoTimer(this),
    outputFilePath(outputPath)
{
    auto* panel = new wxPanel(this);
    auto* root = new wxBoxSizer(wxVERTICAL);

    descLabel = new wxStaticText(panel, wxID_ANY, "Initializing demo...");
    wxFont f = descLabel->GetFont();
    f.SetPointSize(f.GetPointSize() + 1);
    f.SetWeight(wxFONTWEIGHT_BOLD);
    descLabel->SetFont(f);

    root->Add(descLabel, 0, wxEXPAND | wxALL, 8);

    vizPanel = new VisualizerPanel(panel);
    root->Add(vizPanel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);

    btnPrev = new wxButton(panel, wxID_ANY, " Prev");
    btnNext = new wxButton(panel, wxID_ANY, " Next");
    btnAuto = new wxButton(panel, wxID_ANY, " Auto");
    btnClose = new wxButton(panel, wxID_ANY, " Close");

    btnPrev->SetBitmap(wxArtProvider::GetBitmap(wxART_GO_BACK, wxART_BUTTON, wxSize(16, 16)));
    btnNext->SetBitmap(wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_BUTTON, wxSize(16, 16)));
    btnAuto->SetBitmap(wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_BUTTON, wxSize(16, 16)));
    btnClose->SetBitmap(wxArtProvider::GetBitmap(wxART_QUIT, wxART_BUTTON, wxSize(16, 16)));

    btnSizer->Add(btnPrev, 0, wxRIGHT, 6);
    btnSizer->Add(btnNext, 0, wxRIGHT, 6);
    btnSizer->Add(btnAuto, 0, wxRIGHT, 6);
    btnSizer->AddStretchSpacer(1);
    btnSizer->Add(btnClose, 0);

    root->Add(btnSizer, 0, wxEXPAND | wxALL, 8);

    panel->SetSizer(root);

    demo.Build(inputValues, 4);

    // Hiện ngay bước đầu tiên
    if (demo.CanNext()) {
        demo.Next();
    }

    RefreshView();

    btnPrev->Bind(wxEVT_BUTTON, &DemoFrame::OnPrev, this);
    btnNext->Bind(wxEVT_BUTTON, &DemoFrame::OnNext, this);
    btnAuto->Bind(wxEVT_BUTTON, &DemoFrame::OnAuto, this);
    btnClose->Bind(wxEVT_BUTTON, &DemoFrame::OnCloseWindow, this);
    Bind(wxEVT_TIMER, &DemoFrame::OnAutoTimer, this);

    Centre();
}

void DemoFrame::RefreshView()
{
    vizPanel->SetState(demo.GetState());
    descLabel->SetLabel(wxString::FromUTF8(demo.GetDescription().c_str()));

    btnPrev->Enable(demo.CanPrev());
    btnNext->Enable(demo.CanNext());
}

void DemoFrame::WriteOutputIfDone()
{
    const auto& s = demo.GetState();
    if (s.finalOutput.empty()) return;
    if (s.finalOutput.size() != s.input.size()) return;
    if (outputFilePath.IsEmpty()) return;

    std::ofstream out(outputFilePath.ToStdString(), std::ios::binary);
    if (!out) return;

    out.write(reinterpret_cast<const char*>(s.finalOutput.data()),
        static_cast<std::streamsize>(s.finalOutput.size() * sizeof(double)));
}

void DemoFrame::OnPrev(wxCommandEvent&)
{
    demo.Prev();
    RefreshView();
}

void DemoFrame::OnNext(wxCommandEvent&)
{
    demo.Next();
    RefreshView();
    WriteOutputIfDone();
}

void DemoFrame::OnAuto(wxCommandEvent&)
{
    autoRunning = !autoRunning;

    if (autoRunning) {
        btnAuto->SetLabel(" Stop");
        autoTimer.Start(650);
    }
    else {
        btnAuto->SetLabel(" Auto");
        autoTimer.Stop();
    }
}

void DemoFrame::OnCloseWindow(wxCommandEvent&)
{
    autoTimer.Stop();
    Close();
}

void DemoFrame::OnAutoTimer(wxTimerEvent&)
{
    if (!autoRunning) return;

    if (demo.CanNext()) {
        demo.Next();
        RefreshView();
        WriteOutputIfDone();
    }
    else {
        autoRunning = false;
        autoTimer.Stop();
        btnAuto->SetLabel(" Auto");
    }
}