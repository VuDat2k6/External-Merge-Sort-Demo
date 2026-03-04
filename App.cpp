#include "App.h"
#include "MainFrame.h"

wxIMPLEMENT_APP(App);

bool App::OnInit()
{
    MainFrame* frame = new MainFrame("External Sort GUI");
    frame->SetClientSize(800, 600);
    frame->Center();
    frame->Show();
    return true;
}