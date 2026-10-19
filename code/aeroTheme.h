#pragma once

#include <algorithm>

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/wx.h>

namespace aero {

namespace palette {
static const wxColour skyTop(182, 228, 250);
static const wxColour skyBottom(246, 252, 255);
static const wxColour glassTop(255, 255, 255);
static const wxColour glassBottom(226, 244, 253);
static const wxColour surfaceTop(251, 254, 255);
static const wxColour surfaceBottom(237, 249, 254);
static const wxColour blueTop(190, 239, 255);
static const wxColour blueBottom(78, 168, 218);
static const wxColour ink(29, 57, 82);
static const wxColour mutedInk(75, 105, 128);
static const wxColour border(153, 204, 230);
}

class Panel final : public wxPanel {
public:
    Panel(wxWindow* parent,
          const wxColour& top = palette::glassTop,
          const wxColour& bottom = palette::glassBottom,
          bool glass = true,
          bool atmosphere = false)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                  wxBORDER_NONE | wxTAB_TRAVERSAL),
          m_top(top),
          m_bottom(bottom),
          m_glass(glass),
          m_atmosphere(atmosphere) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &Panel::OnPaint, this);
    }

private:
    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        const wxSize size = GetClientSize();
        if (size.x <= 0 || size.y <= 0) {
            return;
        }

        dc.GradientFillLinear(wxRect(0, 0, size.x, size.y), m_top, m_bottom,
                              wxSOUTH);

        if (m_atmosphere) {
            wxGraphicsContext* graphics = wxGraphicsContext::Create(dc);
            if (graphics) {
                graphics->SetPen(*wxTRANSPARENT_PEN);
                graphics->SetBrush(wxBrush(wxColour(255, 255, 255, 74)));
                graphics->DrawEllipse(size.x - 250, 24, 190, 190);
                graphics->SetBrush(wxBrush(wxColour(105, 214, 247, 42)));
                graphics->DrawEllipse(28, size.y - 190, 154, 154);
                graphics->SetBrush(wxBrush(wxColour(255, 255, 255, 40)));
                graphics->DrawEllipse(size.x - 118, size.y - 128, 82, 82);
                delete graphics;
            }
        }

        if (m_glass) {
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.SetPen(wxPen(palette::border, 1));
            dc.DrawRoundedRectangle(wxRect(0, 0, size.x - 1, size.y - 1), 9);
            dc.SetPen(wxPen(wxColour(255, 255, 255), 1));
            dc.DrawLine(10, 1, std::max(10, size.x - 11), 1);
        }
    }

    wxColour m_top;
    wxColour m_bottom;
    bool m_glass;
    bool m_atmosphere;
};

class Button final : public wxControl {
public:
    Button(wxWindow* parent, wxWindowID id, const wxString& label,
           const wxSize& size = wxDefaultSize)
        : wxControl(parent, id, wxDefaultPosition, size,
                    wxBORDER_NONE | wxWANTS_CHARS),
          m_pressed(false),
          m_hovered(false) {
        SetLabel(label);
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetForegroundColour(palette::ink);
        SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                       wxFONTWEIGHT_BOLD));
        SetCursor(wxCursor(wxCURSOR_HAND));
        SetCanFocus(true);

        Bind(wxEVT_PAINT, &Button::OnPaint, this);
        Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) {
            m_hovered = true;
            Refresh();
        });
        Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) {
            m_hovered = false;
            if (!HasCapture()) {
                m_pressed = false;
            }
            Refresh();
        });
        Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) {
            m_pressed = true;
            SetFocus();
            CaptureMouse();
            Refresh();
        });
        Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& event) {
            const bool activate = m_pressed &&
                GetClientRect().Contains(event.GetPosition()) && IsEnabled();
            m_pressed = false;
            if (HasCapture()) {
                ReleaseMouse();
            }
            Refresh();
            if (activate) {
                SendClick();
            }
        });
        Bind(wxEVT_SET_FOCUS, [this](wxFocusEvent& event) {
            Refresh();
            event.Skip();
        });
        Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& event) {
            Refresh();
            event.Skip();
        });
        Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& event) {
            if (event.GetKeyCode() == WXK_SPACE || event.GetKeyCode() == WXK_RETURN) {
                m_pressed = true;
                Refresh();
            } else {
                event.Skip();
            }
        });
        Bind(wxEVT_KEY_UP, [this](wxKeyEvent& event) {
            if ((event.GetKeyCode() == WXK_SPACE || event.GetKeyCode() == WXK_RETURN) &&
                m_pressed) {
                m_pressed = false;
                Refresh();
                if (IsEnabled()) {
                    SendClick();
                }
            } else {
                event.Skip();
            }
        });
    }

private:
    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        const wxSize size = GetClientSize();
        if (size.x <= 0 || size.y <= 0) {
            return;
        }

        wxColour top = palette::blueTop;
        wxColour bottom = palette::blueBottom;
        wxColour outline(69, 143, 188);
        if (m_pressed) {
            top = wxColour(85, 171, 215);
            bottom = wxColour(170, 231, 250);
            outline = wxColour(54, 125, 171);
        } else if (m_hovered) {
            top = wxColour(221, 249, 255);
            bottom = wxColour(106, 194, 232);
            outline = wxColour(63, 151, 199);
        }
        if (!IsEnabled()) {
            top = wxColour(222, 235, 242);
            bottom = wxColour(194, 216, 229);
            outline = wxColour(167, 193, 208);
        }

        const wxRect bounds(1, 1, size.x - 2, size.y - 2);
        dc.GradientFillLinear(bounds, top, bottom, m_pressed ? wxNORTH : wxSOUTH);
        const int shineHeight = std::max(1, bounds.height / 3);
        dc.GradientFillLinear(
            wxRect(bounds.x + 2, bounds.y + 2, std::max(1, bounds.width - 4), shineHeight),
            m_hovered ? wxColour(255, 255, 255) : wxColour(248, 254, 255),
            m_pressed ? wxColour(205, 240, 252) : wxColour(213, 242, 252),
            wxSOUTH);

        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetPen(wxPen(outline, 1));
        dc.DrawRoundedRectangle(bounds, 7);
        dc.SetPen(wxPen(wxColour(255, 255, 255), 1));
        dc.DrawLine(8, 2, std::max(8, size.x - 9), 2);

        if (HasFocus()) {
            dc.SetPen(wxPen(wxColour(255, 255, 255), 1, wxPENSTYLE_SHORT_DASH));
            dc.DrawRoundedRectangle(wxRect(5, 5, size.x - 10, size.y - 10), 5);
        }

        dc.SetFont(GetFont());
        dc.SetTextForeground(IsEnabled() ? palette::ink : palette::mutedInk);
        dc.DrawLabel(GetLabel(), bounds, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
    }

    void SendClick() {
        wxCommandEvent event(wxEVT_BUTTON, GetId());
        event.SetEventObject(this);
        GetEventHandler()->ProcessEvent(event);
    }

    bool m_pressed;
    bool m_hovered;
};

} // namespace aero
