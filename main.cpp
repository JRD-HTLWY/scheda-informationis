#include <wx/wx.h>
#include "MyProjectBase.h"

class MyApp : public wxApp {
public:
    bool OnInit() override {
        MyFrame1* frame = new MyFrame1(nullptr);
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);
