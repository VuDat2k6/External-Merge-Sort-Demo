#include "VisualizerPanel.h"
#include "DemoController.h"

#include <wx/dcbuffer.h>
#include <sstream>
#include <iomanip>
#include <cmath>

wxBEGIN_EVENT_TABLE(VisualizerPanel, wxPanel)
EVT_PAINT(VisualizerPanel::OnPaint)
wxEND_EVENT_TABLE()

VisualizerPanel::VisualizerPanel(wxWindow* parent)
    : wxPanel(parent), animTimer(this)
{
    SetMinSize(wxSize(780, 540));
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(wxColour(245, 245, 245));

    Bind(wxEVT_TIMER, &VisualizerPanel::OnAnimTimer, this);
}

void VisualizerPanel::SetState(const DemoState& s)
{
    if (!state.input.empty() || !state.currentRuns.empty() || !state.finalOutput.empty()) {
        prevState = state;
        hasPrevState = true;
    }

    state = s;
    StartAnimationIfNeeded();
    Refresh();
}

void VisualizerPanel::ClearState()
{
    animTimer.Stop();
    animating = false;
    animT = 0.0;
    state = DemoState{};
    prevState = DemoState{};
    hasPrevState = false;
    Refresh();
}

wxColour VisualizerPanel::RunColor(std::size_t idx) const
{
    static const wxColour palette[] = {
        wxColour(255, 235, 205),
        wxColour(220, 235, 255),
        wxColour(225, 250, 225),
        wxColour(255, 230, 240),
        wxColour(240, 235, 255),
        wxColour(245, 245, 210)
    };
    return palette[idx % (sizeof(palette) / sizeof(palette[0]))];
}

void VisualizerPanel::DrawSectionTitle(wxDC& dc, const wxRect& r, const wxString& text, const wxColour& bg)
{
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(bg));
    dc.DrawRectangle(r);

    dc.SetFont(wxFontInfo(12).Bold());
    dc.SetTextForeground(*wxBLACK);
    dc.DrawLabel(text, r, wxALIGN_CENTER);
}

void VisualizerPanel::DrawPanelBox(wxDC& dc, const wxRect& r, const wxString& label, const wxColour& bg)
{
    dc.SetPen(wxPen(wxColour(90, 90, 90), 2));
    dc.SetBrush(wxBrush(bg));
    dc.DrawRoundedRectangle(r, 8);

    dc.SetFont(wxFontInfo(10).Bold());
    dc.SetTextForeground(wxColour(40, 40, 40));
    dc.DrawText(label, r.x + 8, r.y + 6);
}

void VisualizerPanel::DrawCell(wxDC& dc, const wxRect& r, const wxString& text, const wxColour& fill, bool highlight)
{
    dc.SetPen(wxPen(highlight ? wxColour(220, 80, 60) : wxColour(120, 120, 120), highlight ? 2 : 1));
    dc.SetBrush(wxBrush(highlight ? wxColour(255, 220, 120) : fill));
    dc.DrawRoundedRectangle(r, 5);

    dc.SetFont(wxFontInfo(9).Bold());
    dc.SetTextForeground(*wxBLACK);
    dc.DrawLabel(text, r, wxALIGN_CENTER);
}

wxPoint VisualizerPanel::GetFrameRecordCenter(std::size_t frameIdx, std::size_t slotIdx) const
{
    const wxSize sz = GetClientSize();
    const int titleH = 34;
    const int splitX = sz.GetWidth() / 2;
    wxRect leftBox(18, titleH + 16, splitX - 36, sz.GetHeight() - titleH - 34);

    const int frameW = 98;
    const int frameH = 90;
    const int gap = 14;

    const int x0 = leftBox.x + 24;
    const int y0 = leftBox.y + 48;

    int fx = x0 + static_cast<int>(frameIdx) * (frameW + gap);
    int fy = y0;

    const int cellW = frameW - 28;
    const int cellH = 24;
    const int cx = fx + 14;
    const int cy = fy + 30 + static_cast<int>(slotIdx) * (cellH + 4);

    return wxPoint(cx + cellW / 2, cy + cellH / 2);
}

wxPoint VisualizerPanel::GetOutCellCenter(std::size_t outputIndex) const
{
    const wxSize sz = GetClientSize();
    const int titleH = 34;
    const int splitX = sz.GetWidth() / 2;
    wxRect leftBox(18, titleH + 16, splitX - 36, sz.GetHeight() - titleH - 34);

    wxRect fout(leftBox.x + 70, leftBox.y + 190, 300, 150);

    const int cellW = 36;
    const int cellH = 24;
    const int perRow = 6;

    const int row = static_cast<int>(outputIndex / perRow);
    const int col = static_cast<int>(outputIndex % perRow);

    const int x = fout.x + 10 + col * (cellW + 4);
    const int y = fout.y + 34 + row * (cellH + 4);

    return wxPoint(x + cellW / 2, y + cellH / 2);
}

void VisualizerPanel::StartAnimationIfNeeded()
{
    animTimer.Stop();
    animating = false;
    animT = 0.0;

    if (!hasPrevState) return;

    if (state.hasPending && !prevState.hasPending) {
        if (state.pendingFrame >= 0 &&
            state.pendingFrame < static_cast<int>(state.frames.size())) {

            const auto& fr = state.frames[static_cast<std::size_t>(state.pendingFrame)];
            if (fr.recordIndex >= 0) {
                animValue = state.pendingValue;
                animFrom = GetFrameRecordCenter(
                    static_cast<std::size_t>(state.pendingFrame),
                    static_cast<std::size_t>(fr.recordIndex));
                animTo = GetOutCellCenter(state.outputFrame.size());
                animColor = RunColor(static_cast<std::size_t>(state.pendingFrame));

                animating = true;
                animT = 0.0;
                animTimer.Start(16);
            }
        }
    }
}

void VisualizerPanel::OnAnimTimer(wxTimerEvent&)
{
    if (!animating) {
        animTimer.Stop();
        return;
    }

    animT += 0.08;
    if (animT >= 1.0) {
        animT = 1.0;
        animating = false;
        animTimer.Stop();
    }

    Refresh();
}

void VisualizerPanel::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();

    const wxSize sz = GetClientSize();
    const int titleH = 34;
    const int splitX = sz.GetWidth() / 2;

    DrawSectionTitle(dc, wxRect(0, 0, splitX, titleH), "MAIN MEMORY", wxColour(230, 236, 248));
    DrawSectionTitle(dc, wxRect(splitX, 0, sz.GetWidth() - splitX, titleH), "SECONDARY STORAGE", wxColour(232, 242, 232));

    // =========================================================
    // LEFT: BUFFER
    // =========================================================
    wxRect leftBox(18, titleH + 16, splitX - 36, sz.GetHeight() - titleH - 34);
    DrawPanelBox(
        dc,
        leftBox,
        wxString::Format("BUFFER  (Pass %d, Group %d)", state.passNumber, state.groupNumber),
        wxColour(240, 245, 255));

    const int frameW = 98;
    const int frameH = 90;
    const int gap = 14;

    const int x0 = leftBox.x + 24;
    const int y0 = leftBox.y + 48;

    wxRect f1(x0 + 0 * (frameW + gap), y0, frameW, frameH);
    wxRect f2(x0 + 1 * (frameW + gap), y0, frameW, frameH);
    wxRect f3(x0 + 2 * (frameW + gap), y0, frameW, frameH);
    wxRect fout(leftBox.x + 70, leftBox.y + 190, 300, 150);

    DrawPanelBox(dc, f1, "F1", wxColour(255, 255, 255));
    DrawPanelBox(dc, f2, "F2", wxColour(255, 255, 255));
    DrawPanelBox(dc, f3, "F3", wxColour(255, 255, 255));
    DrawPanelBox(dc, fout, "OUT", wxColour(250, 250, 250));

    auto drawFrame = [&](const wxRect& frRect, int frameIndex) {
        if (frameIndex < 0 || frameIndex >= static_cast<int>(state.frames.size())) return;

        const auto& cursor = state.frames[static_cast<std::size_t>(frameIndex)];
        if (cursor.runIndex < 0 ||
            cursor.runIndex >= static_cast<int>(state.currentRuns.size()) ||
            cursor.pageIndex < 0) {
            DrawCell(dc, wxRect(frRect.x + 14, frRect.y + 34, frRect.width - 28, 24),
                "-", wxColour(235, 235, 235), false);
            return;
        }

        const auto& run = state.currentRuns[static_cast<std::size_t>(cursor.runIndex)];
        if (cursor.pageIndex >= static_cast<int>(run.pages.size())) {
            DrawCell(dc, wxRect(frRect.x + 14, frRect.y + 34, frRect.width - 28, 24),
                "-", wxColour(235, 235, 235), false);
            return;
        }

        dc.SetFont(wxFontInfo(7).Bold());
        dc.SetTextForeground(wxColour(70, 70, 70));
        dc.DrawText(wxString::Format("R%d-P%d",
            cursor.runIndex + 1, cursor.pageIndex + 1),
            frRect.x + 6, frRect.y + 20);

        const auto& page = run.pages[static_cast<std::size_t>(cursor.pageIndex)];
        const int cellW = frRect.width - 28;
        const int cellH = 24;

        for (std::size_t i = 0; i < page.size(); ++i) {
            wxRect cell(frRect.x + 14,
                frRect.y + 34 + static_cast<int>(i) * (cellH + 4),
                cellW, cellH);

            std::ostringstream oss;
            oss << std::fixed << std::setprecision(0) << page[i];

            bool isPointer = (static_cast<int>(i) == cursor.recordIndex);
            bool highlight = isPointer;
            if (state.hasPending) {
                highlight = (state.pendingFrame == frameIndex && isPointer);
            }

            DrawCell(dc, cell,
                wxString::FromUTF8(oss.str().c_str()),
                RunColor(static_cast<std::size_t>(cursor.runIndex)),
                highlight);
        }
        };

    drawFrame(f1, 0);
    drawFrame(f2, 1);
    drawFrame(f3, 2);

    // OUT frame
    {
        const int cellW = 36;
        const int cellH = 24;
        const int perRow = 6;

        for (std::size_t i = 0; i < state.outputFrame.size(); ++i) {
            const int row = static_cast<int>(i / perRow);
            const int col = static_cast<int>(i % perRow);

            const int y = fout.y + 34 + row * (cellH + 4);
            if (y + cellH > fout.GetBottom() - 8) break;

            wxRect c(fout.x + 10 + col * (cellW + 4), y, cellW, cellH);

            std::ostringstream oss;
            oss << std::fixed << std::setprecision(0) << state.outputFrame[i];
            DrawCell(dc, c, wxString::FromUTF8(oss.str().c_str()), wxColour(255, 255, 255), false);
        }

        if (state.outputFrame.size() > 24) {
            dc.SetFont(wxFontInfo(8));
            dc.SetTextForeground(wxColour(70, 70, 70));
            dc.DrawText(wxString::Format("Total: %zu values", state.outputFrame.size()),
                fout.x + 12, fout.GetBottom() - 16);
        }
    }

    // =========================================================
    // RIGHT: SECONDARY STORAGE
    // =========================================================
    wxRect rightBox(splitX + 18, titleH + 16, sz.GetWidth() - splitX - 36, sz.GetHeight() - titleH - 34);
    DrawPanelBox(dc, rightBox, "SECONDARY STORAGE VIEW", wxColour(240, 250, 240));

    // -------- ACTIVE RUNS 
    dc.SetFont(wxFontInfo(9).Bold());
    dc.SetTextForeground(*wxBLACK);
    dc.DrawText("ACTIVE RUNS (selected in current group):", rightBox.x + 18, rightBox.y + 48);

    const int infoBoxW = 120;
    const int infoBoxH = 135;
    const int infoGap = 14;
    const int infoY = rightBox.y + 72;
    const int infoX0 = rightBox.x + 18;

    for (int f = 0; f < DemoController::K; ++f) {
        wxRect box(infoX0 + f * (infoBoxW + infoGap), infoY, infoBoxW, infoBoxH);
        DrawPanelBox(dc, box, wxString::Format("F%d", f + 1), wxColour(252, 252, 252));

        if (f >= static_cast<int>(state.frames.size())) continue;
        const auto& cursor = state.frames[static_cast<std::size_t>(f)];

        if (cursor.runIndex < 0 ||
            cursor.runIndex >= static_cast<int>(state.currentRuns.size())) {
            dc.SetFont(wxFontInfo(8));
            dc.SetTextForeground(wxColour(90, 90, 90));
            dc.DrawText("No run", box.x + 34, box.y + 52);
            continue;
        }

        const auto& run = state.currentRuns[static_cast<std::size_t>(cursor.runIndex)];

        dc.SetFont(wxFontInfo(8).Bold());
        dc.SetTextForeground(*wxBLACK);
        dc.DrawText(wxString::Format("Run: R%d", cursor.runIndex + 1), box.x + 10, box.y + 28);
        dc.DrawText(wxString::Format("Page: %d / %zu",
            cursor.pageIndex + 1, run.pages.size()),
            box.x + 10, box.y + 46);

        if (cursor.pageIndex >= 0 && cursor.pageIndex < static_cast<int>(run.pages.size())) {
            const auto& page = run.pages[static_cast<std::size_t>(cursor.pageIndex)];
            const int cellW = box.width - 20;
            const int cellH = 20;

            for (std::size_t i = 0; i < page.size(); ++i) {
                wxRect cell(box.x + 10,
                    box.y + 68 + static_cast<int>(i) * (cellH + 4),
                    cellW, cellH);

                std::ostringstream oss;
                oss << std::fixed << std::setprecision(0) << page[i];

                bool highlight = (static_cast<int>(i) == cursor.recordIndex);
                if (state.hasPending) {
                    highlight = (state.pendingFrame == f && static_cast<int>(i) == cursor.recordIndex);
                }

                DrawCell(dc, cell,
                    wxString::FromUTF8(oss.str().c_str()),
                    RunColor(static_cast<std::size_t>(cursor.runIndex)),
                    highlight);
            }
        }
    }

    // -------- NEXT RUNS --------
    const int nextTitleY = rightBox.y + 240;
    dc.SetFont(wxFontInfo(9).Bold());
    dc.SetTextForeground(*wxBLACK);
    dc.DrawText("NEW RUNS (created in this pass):", rightBox.x + 18, nextTitleY);

    const int nextBoxW = 120;
    const int nextBoxH = 105;
    const int nextGap = 14;
    const int nextY = nextTitleY + 24;
    const int nextX0 = rightBox.x + 18;

    for (std::size_t i = 0; i < state.nextRuns.size(); ++i) {
        wxRect box(nextX0 + static_cast<int>(i) * (nextBoxW + nextGap), nextY, nextBoxW, nextBoxH);
        DrawPanelBox(dc, box, wxString::Format("N%zu", i + 1), wxColour(252, 252, 252));

        const auto& run = state.nextRuns[i];
        const auto flat = FlattenRun(run);

        dc.SetFont(wxFontInfo(8).Bold());
        dc.DrawText(wxString::Format("Pages: %zu", run.pages.size()), box.x + 10, box.y + 28);

        std::ostringstream oss;
        const std::size_t show = std::min<std::size_t>(flat.size(), 6);
        for (std::size_t k = 0; k < show; ++k) {
            if (k) oss << " ";
            oss << std::fixed << std::setprecision(0) << flat[k];
        }
        if (flat.size() > show) oss << " ...";

        dc.SetFont(wxFontInfo(8));
        dc.DrawText(wxString::FromUTF8(oss.str().c_str()), box.x + 10, box.y + 50);
    }

    // -------- FINAL OUTPUT --------
    if (!state.finalOutput.empty()) {
        dc.SetFont(wxFontInfo(9).Bold());
        dc.SetTextForeground(*wxBLACK);
        dc.DrawText("FINAL OUTPUT:", rightBox.x + 18, rightBox.GetBottom() - 60);

        std::ostringstream outss;
        outss << "[ ";
        for (double v : state.finalOutput) {
            outss << std::fixed << std::setprecision(0) << v << " ";
        }
        outss << "]";

        dc.SetFont(wxFontInfo(8));
        dc.DrawText(wxString::FromUTF8(outss.str().c_str()),
            rightBox.x + 120, rightBox.GetBottom() - 60);
    }

    // -------- Animation token --------
    if (animating) {
        const int x = static_cast<int>(std::round(animFrom.x + (animTo.x - animFrom.x) * animT));
        const int y = static_cast<int>(std::round(animFrom.y + (animTo.y - animFrom.y) * animT));

        dc.SetPen(wxPen(wxColour(180, 60, 40), 2));
        dc.SetBrush(wxBrush(animColor));
        dc.DrawCircle(wxPoint(x, y), 12);

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(0) << animValue;
        const wxString txt = wxString::FromUTF8(oss.str().c_str());

        dc.SetFont(wxFontInfo(8).Bold());
        dc.SetTextForeground(*wxBLACK);
        wxSize ts = dc.GetTextExtent(txt);
        dc.DrawText(txt, x - ts.GetWidth() / 2, y - ts.GetHeight() / 2);
    }

}
