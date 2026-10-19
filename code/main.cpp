#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <wx/statline.h>
#include "MyProjectBase.h"

class TicketDetailFrame : public wxFrame {
public:
    explicit TicketDetailFrame(wxWindow* parent = nullptr)
        : wxFrame(parent, wxID_ANY, _( "Ticket-Support" ), wxDefaultPosition,
                  wxSize(900, 700), wxDEFAULT_FRAME_STYLE) {
        SetMinSize(wxSize(720, 560));

        auto* panel = new wxPanel(this, wxID_ANY);
        panel->SetBackgroundColour(wxColour(248, 250, 253));

        auto* pageLayout = new wxBoxSizer(wxVERTICAL);
        auto* header = new wxBoxSizer(wxHORIZONTAL);

        auto* title = new wxStaticText(panel, wxID_ANY, _( "Ticket" ));
        title->SetFont(wxFont(24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                              wxFONTWEIGHT_BOLD));
        title->SetForegroundColour(wxColour(29, 44, 66));
        header->Add(120, 0);
        header->AddStretchSpacer(1);
        header->Add(title, 0, wxALIGN_CENTER_VERTICAL);
        header->AddStretchSpacer(1);

        auto* username = new wxStaticText(panel, wxID_ANY, _( "Benutzername" ));
        username->SetMinSize(wxSize(120, -1));
        username->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                 wxFONTWEIGHT_NORMAL));
        username->SetForegroundColour(wxColour(79, 95, 116));
        header->Add(username, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        pageLayout->Add(header, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 28);

        auto* detailsPanel = new wxPanel(panel, wxID_ANY);
        detailsPanel->SetBackgroundColour(wxColour(237, 243, 250));
        auto* detailsLayout = new wxBoxSizer(wxVERTICAL);
        detailsLayout->AddSpacer(12);
        auto addDetail = [&](const wxString& label) {
            auto* text = new wxStaticText(detailsPanel, wxID_ANY, label);
            text->SetForegroundColour(wxColour(29, 44, 66));
            detailsLayout->Add(text, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
        };
        addDetail(_( "Titel: $Tickettitel" ));
        addDetail(_( "Ticket-Nummer: $ID" ));

        auto* statusLine = new wxBoxSizer(wxHORIZONTAL);
        auto* statusLabel = new wxStaticText(detailsPanel, wxID_ANY, _( "Ticket Status: " ));
        statusLabel->SetForegroundColour(wxColour(29, 44, 66));
        auto* closedStatus = new wxStaticText(detailsPanel, wxID_ANY, _( "geschlossen" ));
        closedStatus->SetForegroundColour(wxColour(190, 62, 62));
        auto* statusSlash = new wxStaticText(detailsPanel, wxID_ANY, _( "/" ));
        auto* openStatus = new wxStaticText(detailsPanel, wxID_ANY, _( "offen" ));
        openStatus->SetForegroundColour(wxColour(38, 145, 82));
        statusLine->Add(statusLabel);
        statusLine->Add(closedStatus);
        statusLine->Add(statusSlash, 0, wxLEFT | wxRIGHT, 4);
        statusLine->Add(openStatus);
        detailsLayout->Add(statusLine, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        addDetail(_( "Datum: $date" ));
        addDetail(_( "Bearbeiter: $employee" ));
        detailsLayout->AddSpacer(4);
        detailsPanel->SetSizer(detailsLayout);
        pageLayout->Add(detailsPanel, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP, 24);
        detailsPanel->SetMinSize(wxSize(360, -1));

        auto* conversationPanel = new wxPanel(panel, wxID_ANY);
        conversationPanel->SetBackgroundColour(wxColour(255, 255, 255));
        auto* conversationLayout = new wxBoxSizer(wxVERTICAL);

        auto* conversation = new wxScrolledWindow(conversationPanel, wxID_ANY,
                                                   wxDefaultPosition, wxDefaultSize,
                                                   wxVSCROLL | wxBORDER_NONE);
        conversation->SetBackgroundColour(wxColour(255, 255, 255));
        conversation->SetScrollRate(0, 10);
        auto* messageLayout = new wxBoxSizer(wxVERTICAL);

        auto* customerMessage = new wxStaticText(conversation, wxID_ANY,
                                                  _( "Kunde: $Inhalt" ));
        customerMessage->SetForegroundColour(wxColour(29, 44, 66));
        messageLayout->Add(customerMessage, 0, wxLEFT | wxTOP, 16);
        messageLayout->AddSpacer(170);

        auto* employeeMessage = new wxStaticText(conversation, wxID_ANY,
                                                  _( "Bearbeiter:\n$Antwort" ));
        employeeMessage->SetForegroundColour(wxColour(79, 95, 116));
        employeeMessage->Wrap(200);
        messageLayout->Add(employeeMessage, 0, wxALIGN_RIGHT | wxRIGHT, 28);
        messageLayout->AddSpacer(180);
        conversation->SetSizer(messageLayout);
        conversation->FitInside();
        conversation->ShowScrollbars(wxSHOW_SB_DEFAULT, wxSHOW_SB_ALWAYS);
        conversationLayout->Add(conversation, 1, wxEXPAND | wxALL, 12);

        auto* replyInput = new wxTextCtrl(conversationPanel, wxID_ANY, wxEmptyString,
                                          wxDefaultPosition, wxSize(-1, 44),
                                          wxTE_PROCESS_ENTER);
        replyInput->SetHint(_( "Eingabe......" ));
        conversationLayout->Add(replyInput, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
        conversationPanel->SetSizer(conversationLayout);
        pageLayout->Add(conversationPanel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP,
                        48);

        auto* backButton = new wxButton(panel, wxID_ANY, _( "zurück" ),
                                        wxDefaultPosition, wxSize(180, 42));
        backButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Close(); });
        pageLayout->Add(backButton, 0, wxLEFT | wxTOP | wxBOTTOM, 48);

        panel->SetSizer(pageLayout);
        auto* frameLayout = new wxBoxSizer(wxVERTICAL);
        frameLayout->Add(panel, 1, wxEXPAND);
        SetSizer(frameLayout);
        Centre(wxBOTH);
    }
};

class TicketsListFrame : public wxFrame {
public:
    explicit TicketsListFrame(wxWindow* parent = nullptr)
        : wxFrame(parent, wxID_ANY, _( "Ticket-Support" ), wxDefaultPosition,
                  wxSize(900, 680), wxDEFAULT_FRAME_STYLE) {
        SetMinSize(wxSize(720, 540));

        auto* panel = new wxPanel(this, wxID_ANY);
        panel->SetBackgroundColour(wxColour(248, 250, 253));

        auto* pageLayout = new wxBoxSizer(wxVERTICAL);
        auto* header = new wxBoxSizer(wxHORIZONTAL);

        auto* title = new wxStaticText(panel, wxID_ANY, _( "Tickets" ));
        title->SetFont(wxFont(28, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                              wxFONTWEIGHT_BOLD));
        title->SetForegroundColour(wxColour(29, 44, 66));
        header->Add(120, 0);
        header->AddStretchSpacer(1);
        header->Add(title, 0, wxALIGN_CENTER_VERTICAL);
        header->AddStretchSpacer(1);

        auto* username = new wxStaticText(panel, wxID_ANY, _( "Nutzername" ));
        username->SetMinSize(wxSize(120, -1));
        username->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                 wxFONTWEIGHT_NORMAL));
        username->SetForegroundColour(wxColour(79, 95, 116));
        header->Add(username, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        pageLayout->Add(header, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 28);

        pageLayout->AddStretchSpacer(1);

        auto* listPanel = new wxPanel(panel, wxID_ANY);
        listPanel->SetBackgroundColour(wxColour(255, 255, 255));
        auto* listPanelLayout = new wxBoxSizer(wxVERTICAL);
        auto* ticketScroll = new wxScrolledWindow(listPanel, wxID_ANY,
                                                   wxDefaultPosition, wxDefaultSize,
                                                   wxVSCROLL | wxBORDER_NONE);
        ticketScroll->SetBackgroundColour(wxColour(255, 255, 255));
        ticketScroll->SetScrollRate(0, 10);

        auto* ticketRows = new wxBoxSizer(wxVERTICAL);
        auto addTicketRow = [&](const wxString& id, const wxString& customer,
                                const wxString& date, const wxString& subject) {
            auto* row = new wxPanel(ticketScroll, wxID_ANY);
            row->SetBackgroundColour(wxColour(255, 255, 255));
            auto* rowLayout = new wxBoxSizer(wxHORIZONTAL);

            auto makeCell = [&](const wxString& label, int width) {
                auto* cell = new wxStaticText(row, wxID_ANY, label);
                cell->SetMinSize(wxSize(width, -1));
                cell->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                     wxFONTWEIGHT_NORMAL));
                cell->SetForegroundColour(wxColour(29, 44, 66));
                return cell;
            };
            auto* idCell = makeCell(id, 140);
            auto* customerCell = makeCell(customer, 130);
            auto* dateCell = makeCell(date, 110);
            auto* subjectCell = makeCell(subject, 180);
            auto openTicket = [this](wxMouseEvent&) {
                auto* detail = new TicketDetailFrame(this);
                detail->Show(true);
            };
            row->Bind(wxEVT_LEFT_DCLICK, openTicket);
            idCell->Bind(wxEVT_LEFT_DCLICK, openTicket);
            customerCell->Bind(wxEVT_LEFT_DCLICK, openTicket);
            dateCell->Bind(wxEVT_LEFT_DCLICK, openTicket);
            subjectCell->Bind(wxEVT_LEFT_DCLICK, openTicket);
            rowLayout->Add(idCell, 0, wxALIGN_CENTER_VERTICAL);
            rowLayout->Add(customerCell, 0, wxALIGN_CENTER_VERTICAL);
            rowLayout->Add(dateCell, 0, wxALIGN_CENTER_VERTICAL);
            rowLayout->Add(subjectCell, 1, wxALIGN_CENTER_VERTICAL | wxEXPAND);
            row->SetSizer(rowLayout);
            ticketRows->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 16);
            ticketRows->Add(new wxStaticLine(ticketScroll), 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP,
                            16);
        };

        addTicketRow(_( "ID: $Nummer" ), _( "Kunde: $Kunde" ), _( "Datum: $date" ),
                     _( "Titel: $Tickettitel" ));
        addTicketRow(_( "ID: $Nummer" ), _( "Kunde: $Kunde" ), _( "Datum: $date" ),
                     _( "Titel: $Tickettitel" ));
        addTicketRow(_( "ID: $Nummer" ), _( "Kunde: $Kunde" ), _( "Datum: $date" ),
                     _( "Titel: $Tickettitel" ));

        ticketScroll->SetSizer(ticketRows);
        ticketScroll->FitInside();
        ticketScroll->ShowScrollbars(wxSHOW_SB_DEFAULT, wxSHOW_SB_ALWAYS);
        listPanelLayout->Add(ticketScroll, 1, wxEXPAND | wxALL, 12);
        listPanel->SetSizer(listPanelLayout);
        listPanel->SetMinSize(wxSize(620, 360));

        pageLayout->Add(listPanel, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 36);
        pageLayout->AddStretchSpacer(1);

        panel->SetSizer(pageLayout);
        auto* frameLayout = new wxBoxSizer(wxVERTICAL);
        frameLayout->Add(panel, 1, wxEXPAND);
        SetSizer(frameLayout);
        Centre(wxBOTH);
    }
};

class TicketsFrame : public wxFrame {
public:
    explicit TicketsFrame(wxWindow* parent = nullptr)
        : wxFrame(parent, wxID_ANY, _( "Ticket-Support" ), wxDefaultPosition,
                  wxSize(900, 600), wxDEFAULT_FRAME_STYLE) {
        SetMinSize(wxSize(640, 440));

        auto* panel = new wxPanel(this, wxID_ANY);
        panel->SetBackgroundColour(wxColour(248, 250, 253));

        auto* pageLayout = new wxBoxSizer(wxVERTICAL);
        auto* header = new wxBoxSizer(wxHORIZONTAL);

        auto* title = new wxStaticText(panel, wxID_ANY, _( "Ticket-Support" ));
        title->SetFont(wxFont(24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                              wxFONTWEIGHT_BOLD));
        title->SetForegroundColour(wxColour(29, 44, 66));
        header->Add(120, 0);
        header->AddStretchSpacer(1);
        header->Add(title, 0, wxALIGN_CENTER_VERTICAL);
        header->AddStretchSpacer(1);

using namespace std;

int main(int argc, char **argv)
{
  wxPrintf(wxT("A wxWidgets console application\n"));

  return 0;
}
