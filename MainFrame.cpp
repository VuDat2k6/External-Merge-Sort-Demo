#include "MainFrame.h"
#include "ExternalSorter.h"
#include "DemoFrame.h"

#include <wx/statline.h>
#include <wx/datetime.h>
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/sizer.h>
#include <wx/artprov.h>

#include <fstream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <random>
#include <exception>

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(980, 680))
{
    CreateStatusBar(2);
    SetStatusText("Ready", 0);
    SetStatusText("Binary doubles (.bin)", 1);

    BuildUI();
    Centre();
}

void MainFrame::BuildUI()
{
    auto* panel = new wxPanel(this);
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* header = new wxBoxSizer(wxHORIZONTAL);
    auto* title = new wxStaticText(panel, wxID_ANY, "External Merge Sort");
    wxFont f = title->GetFont();
    f.SetPointSize(f.GetPointSize() + 4);
    f.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(f);
    header->Add(title, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    root->Add(header, 0, wxEXPAND | wxTOP | wxBOTTOM, 8);

    root->Add(new wxStaticLine(panel), 0, wxEXPAND | wxLEFT | wxRIGHT, 8);

    auto* fileBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Files");

    inputPicker = new wxFilePickerCtrl(
        panel, wxID_ANY, wxEmptyString, "Choose input file",
        "Binary files (*.bin)|*.bin|All files (*.*)|*.*",
        wxDefaultPosition, wxDefaultSize,
        wxFLP_OPEN | wxFLP_FILE_MUST_EXIST | wxFLP_USE_TEXTCTRL);

    outputPicker = new wxFilePickerCtrl(
        panel, wxID_ANY, wxEmptyString, "Choose output file",
        "Binary files (*.bin)|*.bin|All files (*.*)|*.*",
        wxDefaultPosition, wxDefaultSize,
        wxFLP_SAVE | wxFLP_OVERWRITE_PROMPT | wxFLP_USE_TEXTCTRL);

    fileBox->Add(new wxStaticText(panel, wxID_ANY, "Input (.bin):"), 0, wxLEFT | wxRIGHT | wxTOP, 6);
    fileBox->Add(inputPicker, 0, wxEXPAND | wxALL, 6);

    fileBox->Add(new wxStaticText(panel, wxID_ANY, "Output (.bin):"), 0, wxLEFT | wxRIGHT, 6);
    fileBox->Add(outputPicker, 0, wxEXPAND | wxALL, 6);

    root->Add(fileBox, 0, wxEXPAND | wxALL, 8);

    auto* row = new wxBoxSizer(wxHORIZONTAL);

    auto* optBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Options");
    auto* grid = new wxFlexGridSizer(2, 2, 8, 10);
    grid->AddGrowableCol(1);

    grid->Add(new wxStaticText(panel, wxID_ANY, "Chunk size (MB):"), 0, wxALIGN_CENTER_VERTICAL);
    chunkSpinMB = new wxSpinCtrl(panel, wxID_ANY);
    chunkSpinMB->SetRange(1, 1024);
    chunkSpinMB->SetValue(10);
    grid->Add(chunkSpinMB, 1, wxEXPAND);

    grid->Add(new wxStaticText(panel, wxID_ANY, "Sample count (doubles):"), 0, wxALIGN_CENTER_VERTICAL);
    sampleCountSpin = new wxSpinCtrl(panel, wxID_ANY);
    sampleCountSpin->SetRange(5, 5000000);
    sampleCountSpin->SetValue(12);
    grid->Add(sampleCountSpin, 1, wxEXPAND);

    optBox->Add(grid, 0, wxEXPAND | wxALL, 6);

    auto* actBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Actions");

    btnGenerate = new wxButton(panel, wxID_ANY, " Generate Sample");
    btnPreviewInput = new wxButton(panel, wxID_ANY, " Preview Input");
    btnPreviewOutput = new wxButton(panel, wxID_ANY, " Preview Output");
    btnStart = new wxButton(panel, wxID_ANY, " Start");
    btnClearLog = new wxButton(panel, wxID_ANY, " Clear Log");

    btnGenerate->SetBitmap(wxArtProvider::GetBitmap(wxART_NEW, wxART_BUTTON, wxSize(16, 16)));
    btnPreviewInput->SetBitmap(wxArtProvider::GetBitmap(wxART_FIND, wxART_BUTTON, wxSize(16, 16)));
    btnPreviewOutput->SetBitmap(wxArtProvider::GetBitmap(wxART_FIND, wxART_BUTTON, wxSize(16, 16)));
    btnStart->SetBitmap(wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_BUTTON, wxSize(16, 16)));
    btnClearLog->SetBitmap(wxArtProvider::GetBitmap(wxART_DELETE, wxART_BUTTON, wxSize(16, 16)));

    actBox->Add(btnGenerate, 0, wxEXPAND | wxALL, 4);
    actBox->Add(btnPreviewInput, 0, wxEXPAND | wxALL, 4);
    actBox->Add(btnPreviewOutput, 0, wxEXPAND | wxALL, 4);
    actBox->Add(btnStart, 0, wxEXPAND | wxALL, 4);
    actBox->Add(btnClearLog, 0, wxEXPAND | wxALL, 4);

    row->Add(optBox, 1, wxEXPAND | wxRIGHT, 8);
    row->Add(actBox, 0, wxEXPAND);

    root->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    progressBar = new wxGauge(panel, wxID_ANY, 100);
    progressBar->SetValue(0);

    root->Add(new wxStaticText(panel, wxID_ANY, "Progress:"), 0, wxLEFT | wxRIGHT, 8);
    root->Add(progressBar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    logCtrl = new wxTextCtrl(panel, wxID_ANY, "",
        wxDefaultPosition, wxSize(-1, 260),
        wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);

    root->Add(new wxStaticText(panel, wxID_ANY, "Log:"), 0, wxLEFT | wxRIGHT, 8);
    root->Add(logCtrl, 1, wxEXPAND | wxALL, 8);

    panel->SetSizer(root);

    btnGenerate->Bind(wxEVT_BUTTON, &MainFrame::OnGenerateSample, this);
    btnPreviewInput->Bind(wxEVT_BUTTON, &MainFrame::OnPreviewInput, this);
    btnPreviewOutput->Bind(wxEVT_BUTTON, &MainFrame::OnPreviewOutput, this);
    btnStart->Bind(wxEVT_BUTTON, &MainFrame::OnStart, this);
    btnClearLog->Bind(wxEVT_BUTTON, &MainFrame::OnClearLog, this);

    btnPreviewInput->Enable(false);
    btnPreviewOutput->Enable(false);
    btnStart->Enable(false);

    inputPicker->Bind(wxEVT_FILEPICKER_CHANGED, [this](wxFileDirPickerEvent&) {
        const bool hasInput = !inputPicker->GetPath().IsEmpty();
        btnPreviewInput->Enable(hasInput);
        btnStart->Enable(hasInput && !outputPicker->GetPath().IsEmpty());
        });

    outputPicker->Bind(wxEVT_FILEPICKER_CHANGED, [this](wxFileDirPickerEvent&) {
        const bool hasOutput = !outputPicker->GetPath().IsEmpty();
        btnPreviewOutput->Enable(hasOutput);
        btnStart->Enable(hasOutput && !inputPicker->GetPath().IsEmpty());
        });

    Log("UI ready.\n");
}

void MainFrame::Log(const wxString& msg)
{
    wxDateTime now = wxDateTime::Now();
    wxString line = wxString::Format("[%s] %s", now.FormatISOTime(), msg);
    logCtrl->AppendText(line);
    if (!line.EndsWith("\n")) logCtrl->AppendText("\n");
}

std::vector<double> MainFrame::ReadAllDoubles(const wxString& path)
{
    wxFileName fn(path);
    if (!fn.FileExists()) throw std::runtime_error("Input file does not exist.");

    const wxULongLong sz = fn.GetSize();
    const unsigned long long bytes = sz.GetValue();

    if (bytes % sizeof(double) != 0)
        throw std::runtime_error("Invalid file format (size is not multiple of 8 bytes).");

    const size_t count = static_cast<size_t>(bytes / sizeof(double));
    std::vector<double> values(count);

    std::ifstream in(path.ToStdString(), std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open file.");

    in.read(reinterpret_cast<char*>(values.data()),
        static_cast<std::streamsize>(count * sizeof(double)));
    if (!in && count > 0) throw std::runtime_error("Read error.");

    return values;
}

void MainFrame::PreviewFile(const wxString& path, const wxString& label)
{
    if (path.IsEmpty()) {
        Log("Please choose " + label + " file first.\n");
        return;
    }

    try {
        auto values = ReadAllDoubles(path);

        Log(wxString::Format("Preview %s file: %s", label, path));
        Log(wxString::Format("Total doubles: %zu", values.size()));

        const size_t k = std::min<size_t>(20, values.size());

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3);
        oss << "First " << k << " doubles:\n";
        for (size_t i = 0; i < k; ++i) {
            oss << "[" << i << "] " << values[i] << "\n";
        }

        bool sorted = std::is_sorted(values.begin(), values.begin() + k);
        oss << "Sorted (first " << k << ")? " << (sorted ? "YES" : "NO") << "\n";

        Log(wxString::FromUTF8(oss.str().c_str()));
    }
    catch (const std::exception& ex) {
        Log(wxString::Format("Preview error: %s", ex.what()));
    }
}

void MainFrame::OnClearLog(wxCommandEvent&)
{
    logCtrl->Clear();
    Log("Log cleared.\n");
}

void MainFrame::OnGenerateSample(wxCommandEvent&)
{
    if (inputPicker->GetPath().IsEmpty()) {
        wxFileDialog dlg(this, "Save sample input", "", "",
            "Binary files (*.bin)|*.bin",
            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() != wxID_OK) return;
        inputPicker->SetPath(dlg.GetPath());
    }

    const wxString path = inputPicker->GetPath();
    const int n = sampleCountSpin->GetValue();

    Log(wxString::Format("Generating sample: %d doubles -> %s", n, path));
    progressBar->SetValue(0);

    std::ofstream out(path.ToStdString(), std::ios::binary);
    if (!out) {
        Log("Cannot create file.\n");
        return;
    }

    std::mt19937_64 rng(12345);
    std::uniform_real_distribution<double> dist(-1000.0, 1000.0);

    for (int i = 0; i < n; ++i) {
        double x = dist(rng);
        out.write(reinterpret_cast<const char*>(&x), sizeof(double));
        if (!out) {
            Log("Write error while generating sample.\n");
            return;
        }

        if (n >= 100 && i % (std::max(1, n / 100)) == 0) {
            progressBar->SetValue((i * 100) / n);
            wxYield();
        }
    }

    progressBar->SetValue(100);
    Log("Sample generation done.\n");
}

void MainFrame::OnPreviewInput(wxCommandEvent&)
{
    PreviewFile(inputPicker->GetPath(), "Input");
}

void MainFrame::OnPreviewOutput(wxCommandEvent&)
{
    PreviewFile(outputPicker->GetPath(), "Output");
}

void MainFrame::OnStart(wxCommandEvent&)
{
    const wxString in = inputPicker->GetPath();
    const wxString out = outputPicker->GetPath();
    const int chunkMB = chunkSpinMB->GetValue();

    if (in.IsEmpty() || out.IsEmpty()) {
        Log("Please choose input and output file.\n");
        return;
    }

    progressBar->SetValue(0);

    try {
        auto values = ReadAllDoubles(in);

        Log(wxString::Format("Start sorting...\nInput=%s\nOutput=%s\nCount=%zu\n",
            in, out, values.size()));

        if (values.size() <= 20) {
            Log("Small file detected -> opening K-way demo window.\n");
            auto* demoWin = new DemoFrame(this, values, out);
            demoWin->Show();
            return;
        }

        Log("Large file -> using normal external sort mode.\n");

        ExternalSorter sorter;
        sorter.sortDoublesBinary(
            in.ToStdString(),
            out.ToStdString(),
            static_cast<std::size_t>(chunkMB),
            [&](const std::string& s) {
                Log(wxString::FromUTF8(s.c_str()));
                wxYield();
            },
            [&](int p) {
                progressBar->SetValue(p);
                wxYield();
            }
        );

        Log("Sorting finished.\n");
    }
    catch (const std::exception& ex) {
        Log(wxString::Format("EXCEPTION: %s\n", ex.what()));
    }
    catch (...) {
        Log("EXCEPTION: unknown error\n");
    }
}