#pragma once
#include <wx/wx.h>
#include <wx/timer.h>
#include "DemoState.h"

class VisualizerPanel : public wxPanel {
public:
    explicit VisualizerPanel(wxWindow* parent);

    void SetState(const DemoState& s);
    void ClearState();

private:
    DemoState prevState;
    DemoState state;
    bool hasPrevState = false;

    wxTimer animTimer;

    bool animating = false;
    double animT = 0.0;
    double animValue = 0.0;
    wxPoint animFrom;
    wxPoint animTo;
    wxColour animColor = wxColour(255, 180, 80);

private:
    void StartAnimationIfNeeded();
    void OnPaint(wxPaintEvent& evt);
    void OnAnimTimer(wxTimerEvent& evt);

    void DrawSectionTitle(wxDC& dc, const wxRect& r, const wxString& text, const wxColour& bg);
    void DrawPanelBox(wxDC& dc, const wxRect& r, const wxString& label, const wxColour& bg);
    void DrawCell(wxDC& dc, const wxRect& r, const wxString& text, const wxColour& fill, bool highlight);
    wxColour RunColor(std::size_t idx) const;

    wxPoint GetFrameRecordCenter(std::size_t frameIdx, std::size_t slotIdx) const;
    wxPoint GetOutCellCenter(std::size_t outputIndex) const;

    wxDECLARE_EVENT_TABLE();
};