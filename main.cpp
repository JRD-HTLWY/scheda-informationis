#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <wx/statline.h>
#include "MyProjectBase.h"

template <typename Handler>
static void BindScrollBarEvents(wxScrollBar* scrollbar, Handler handler) {
    scrollbar->Bind(wxEVT_SCROLL_TOP, handler);
    scrollbar->Bind(wxEVT_SCROLL_BOTTOM, handler);
    scrollbar->Bind(wxEVT_SCROLL_LINEUP, handler);
    scrollbar->Bind(wxEVT_SCROLL_LINEDOWN, handler);
    scrollbar->Bind(wxEVT_SCROLL_PAGEUP, handler);
    scrollbar->Bind(wxEVT_SCROLL_PAGEDOWN, handler);
    scrollbar->Bind(wxEVT_SCROLL_THUMBTRACK, handler);
    scrollbar->Bind(wxEVT_SCROLL_THUMBRELEASE, handler);
    scrollbar->Bind(wxEVT_SCROLL_CHANGED, handler);
}

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
        conversation->ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_NEVER);
        auto* conversationBody = new wxBoxSizer(wxHORIZONTAL);
        conversationBody->Add(conversation, 1, wxEXPAND | wxALL, 12);
        auto* conversationScrollbar = new wxScrollBar(
            conversationPanel, wxID_ANY, wxDefaultPosition, wxSize(16, -1),
            wxSB_VERTICAL);
        conversationScrollbar->SetScrollbar(0, 20, 60, 20);
        BindScrollBarEvents(conversationScrollbar, [conversation](wxScrollEvent& event) {
            conversation->Scroll(0, event.GetPosition());
        });
        conversationBody->Add(conversationScrollbar, 0,
                              wxEXPAND | wxTOP | wxRIGHT | wxBOTTOM, 12);
        conversationLayout->Add(conversationBody, 1, wxEXPAND);

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
        CallAfter([conversation, conversationScrollbar]() {
            conversation->FitInside();
            const int virtualHeight = conversation->GetVirtualSize().GetHeight();
            const int pageHeight = conversation->GetClientSize().GetHeight();
            int range = (virtualHeight + 9) / 10;
            int page = (pageHeight + 9) / 10;
            if (page < 1) page = 1;
            if (range < page) range = page;
            conversationScrollbar->SetScrollbar(0, page, range, page);
        });
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
        auto* listBodyLayout = new wxBoxSizer(wxHORIZONTAL);
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
        ticketScroll->ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_NEVER);
        listBodyLayout->Add(ticketScroll, 1, wxEXPAND | wxALL, 12);
        auto* listScrollbar = new wxScrollBar(
            listPanel, wxID_ANY, wxDefaultPosition, wxSize(16, -1), wxSB_VERTICAL);
        listScrollbar->SetScrollbar(0, 1, 10, 1);
        BindScrollBarEvents(listScrollbar, [ticketScroll](wxScrollEvent& event) {
            ticketScroll->Scroll(0, event.GetPosition());
        });
        listBodyLayout->Add(listScrollbar, 0,
                            wxEXPAND | wxTOP | wxRIGHT | wxBOTTOM, 12);
        listPanelLayout->Add(listBodyLayout, 1, wxEXPAND);
        listPanel->SetSizer(listPanelLayout);
        listPanel->SetMinSize(wxSize(620, 360));

        pageLayout->Add(listPanel, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 36);
        pageLayout->AddStretchSpacer(1);

        panel->SetSizer(pageLayout);
        auto* frameLayout = new wxBoxSizer(wxVERTICAL);
        frameLayout->Add(panel, 1, wxEXPAND);
        SetSizer(frameLayout);
        Centre(wxBOTH);
        CallAfter([ticketScroll, listScrollbar]() {
            ticketScroll->FitInside();
            const int virtualHeight = ticketScroll->GetVirtualSize().GetHeight();
            const int pageHeight = ticketScroll->GetClientSize().GetHeight();
            int range = (virtualHeight + 9) / 10;
            int page = (pageHeight + 9) / 10;
            if (page < 1) page = 1;
            if (range < page) range = page;
            listScrollbar->SetScrollbar(0, page, range, page);
        });
    }
};

class TicketsOverviewFrame : public wxFrame {
public:
    explicit TicketsOverviewFrame(wxWindow* parent = nullptr)
        : wxFrame(parent, wxID_ANY, _( "Tickets" ), wxDefaultPosition,
                  wxSize(1200, 800), wxDEFAULT_FRAME_STYLE) {
        SetMinSize(wxSize(800, 600));
        SetBackgroundColour(wxColour(190, 190, 190));

        auto* panel = new wxPanel(this, wxID_ANY);
        panel->SetBackgroundColour(wxColour(176, 176, 176));

        auto* pageLayout = new wxBoxSizer(wxVERTICAL);
        auto* header = new wxBoxSizer(wxHORIZONTAL);

        auto* title = new wxStaticText(panel, wxID_ANY, _( "Tickets" ));
        title->SetFont(wxFont(28, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                              wxFONTWEIGHT_NORMAL));
        title->SetForegroundColour(wxColour(0, 0, 0));
        header->Add(150, 0);
        header->AddStretchSpacer(1);
        header->Add(title, 0, wxALIGN_CENTER_VERTICAL);
        header->AddStretchSpacer(1);

        auto* username = new wxStaticText(panel, wxID_ANY, _( "Nutzername" ));
        username->SetMinSize(wxSize(150, -1));
        username->SetFont(wxFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                 wxFONTWEIGHT_NORMAL));
        username->SetForegroundColour(wxColour(0, 0, 0));
        header->Add(username, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        pageLayout->Add(header, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 52);
        pageLayout->AddStretchSpacer(1);

        auto* contentLayout = new wxBoxSizer(wxHORIZONTAL);
        contentLayout->AddStretchSpacer(1);

        auto* listPanel = new wxPanel(panel, wxID_ANY);
        listPanel->SetBackgroundColour(wxColour(202, 198, 198));
        auto* listPanelLayout = new wxBoxSizer(wxVERTICAL);
        auto* ticketBodyLayout = new wxBoxSizer(wxHORIZONTAL);
        auto* ticketRowsPanel = new wxPanel(listPanel, wxID_ANY);
        ticketRowsPanel->SetBackgroundColour(wxColour(202, 198, 198));
        auto* ticketRows = new wxBoxSizer(wxVERTICAL);
        ticketRows->AddSpacer(8);

        auto addTicketRow = [&]() {
            auto* row = new wxPanel(ticketRowsPanel, wxID_ANY);
            row->SetBackgroundColour(wxColour(202, 198, 198));
            row->SetMinSize(wxSize(-1, 24));
            auto* rowLayout = new wxBoxSizer(wxHORIZONTAL);

            auto makeCell = [&](const wxString& label, int width) {
                auto* cell = new wxStaticText(row, wxID_ANY, label);
                cell->SetMinSize(wxSize(width, -1));
                cell->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                     wxFONTWEIGHT_NORMAL));
                cell->SetForegroundColour(wxColour(0, 0, 0));
                return cell;
            };
            auto* idCell = makeCell(_( "ID: $Nummer" ), 112);
            auto* customerCell = makeCell(_( "Kunde:" ), 84);
            auto* dateCell = makeCell(_( "Datum" ), 82);
            auto* titleCell = makeCell(_( "Titel" ), 84);
            auto* employeeCell = makeCell(_( "Bearbeiter" ), 80);

            auto openTicket = [this](wxMouseEvent&) {
                auto* detail = new TicketDetailFrame(this);
                detail->Show(true);
            };
            row->Bind(wxEVT_LEFT_DCLICK, openTicket);
            idCell->Bind(wxEVT_LEFT_DCLICK, openTicket);
            customerCell->Bind(wxEVT_LEFT_DCLICK, openTicket);
            dateCell->Bind(wxEVT_LEFT_DCLICK, openTicket);
            titleCell->Bind(wxEVT_LEFT_DCLICK, openTicket);
            employeeCell->Bind(wxEVT_LEFT_DCLICK, openTicket);

            rowLayout->Add(idCell, 0, wxALIGN_CENTER_VERTICAL);
            rowLayout->Add(customerCell, 0, wxALIGN_CENTER_VERTICAL);
            rowLayout->Add(dateCell, 0, wxALIGN_CENTER_VERTICAL);
            rowLayout->Add(titleCell, 0, wxALIGN_CENTER_VERTICAL);
            rowLayout->Add(employeeCell, 0, wxALIGN_CENTER_VERTICAL);
            row->SetSizer(rowLayout);
            ticketRows->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT, 24);
            ticketRows->AddSpacer(6);
        };

        addTicketRow();
        addTicketRow();
        addTicketRow();
        ticketRowsPanel->SetSizer(ticketRows);

        auto* scrollbar = new wxScrollBar(listPanel, wxID_ANY, wxDefaultPosition,
                                          wxDefaultSize, wxSB_VERTICAL);
        scrollbar->SetScrollbar(0, 1, 10, 1);
        scrollbar->SetMinSize(wxSize(16, -1));
        ticketBodyLayout->Add(ticketRowsPanel, 1, wxEXPAND);
        ticketBodyLayout->Add(scrollbar, 0, wxEXPAND | wxLEFT, 8);
        listPanelLayout->Add(ticketBodyLayout, 1, wxEXPAND | wxALL, 32);
        listPanel->SetSizer(listPanelLayout);
        listPanel->SetMinSize(wxSize(620, 360));

        contentLayout->Add(listPanel, 4, wxEXPAND);
        contentLayout->AddStretchSpacer(1);
        pageLayout->Add(contentLayout, 5, wxEXPAND | wxLEFT | wxRIGHT, 24);
        pageLayout->AddStretchSpacer(1);

        panel->SetSizer(pageLayout);
        auto* frameLayout = new wxBoxSizer(wxVERTICAL);
        frameLayout->Add(panel, 1, wxEXPAND | wxALL, 10);
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

        auto* username = new wxStaticText(panel, wxID_ANY, _( "Benutzername" ));
        username->SetMinSize(wxSize(120, -1));
        username->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                 wxFONTWEIGHT_NORMAL));
        username->SetForegroundColour(wxColour(79, 95, 116));
        header->Add(username, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);

        pageLayout->Add(header, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 28);
        pageLayout->AddStretchSpacer(1);

        auto* ticketButtons = new wxBoxSizer(wxVERTICAL);
        auto* freeTicketsButton = new wxButton(panel, wxID_ANY,
                                               _( "freie Tickets anzeigen" ),
                                               wxDefaultPosition, wxSize(420, 64));
        auto* myTicketsButton = new wxButton(panel, wxID_ANY,
                                             _( "deine Tickets anzeigen" ),
                                             wxDefaultPosition, wxSize(420, 64));
        myTicketsButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            auto* tickets = new TicketsListFrame(this);
            tickets->Show(true);
        });
        auto* closedTicketsButton = new wxButton(panel, wxID_ANY,
                                                 _( "geschlossene Tickets" ),
                                                 wxDefaultPosition, wxSize(420, 64));
        auto* overviewButton = new wxButton(panel, wxID_ANY,
                                            _( "Ticketübersicht anzeigen" ),
                                            wxDefaultPosition, wxSize(420, 64));
        overviewButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            auto* overview = new TicketsOverviewFrame(this);
            overview->Show(true);
        });
        auto styleAndAddButton = [&](wxButton* button) {
            button->SetFont(wxFont(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                   wxFONTWEIGHT_BOLD));
            ticketButtons->Add(button, 0, wxEXPAND | wxBOTTOM, 16);
        };
        styleAndAddButton(freeTicketsButton);
        styleAndAddButton(myTicketsButton);
        styleAndAddButton(closedTicketsButton);
        styleAndAddButton(overviewButton);
        pageLayout->Add(ticketButtons, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 24);

        pageLayout->AddStretchSpacer(1);
        panel->SetSizer(pageLayout);

        auto* frameLayout = new wxBoxSizer(wxVERTICAL);
        frameLayout->Add(panel, 1, wxEXPAND);
        SetSizer(frameLayout);
        Centre(wxBOTH);
    }
};

class TicketActionsFrame : public wxFrame {
public:
    explicit TicketActionsFrame()
        : wxFrame(nullptr, wxID_ANY, _( "Ticket-Support" ), wxDefaultPosition,
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

        auto* username = new wxStaticText(panel, wxID_ANY, _( "Benutzername" ));
        username->SetMinSize(wxSize(120, -1));
        username->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                 wxFONTWEIGHT_NORMAL));
        username->SetForegroundColour(wxColour(79, 95, 116));
        header->Add(username, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);

        pageLayout->Add(header, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 28);
        pageLayout->AddStretchSpacer(1);

        auto* buttons = new wxBoxSizer(wxVERTICAL);
        auto* createTicketButton = new wxButton(panel, wxID_ANY, _( "Ticket erstellen" ),
                                                wxDefaultPosition, wxSize(420, 72));
        createTicketButton->SetFont(wxFont(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                           wxFONTWEIGHT_BOLD));
        buttons->Add(createTicketButton, 0, wxEXPAND | wxBOTTOM, 16);

        auto* myTicketsButton = new wxButton(panel, wxID_ANY, _( "Meine Tickets anzeigen" ),
                                             wxDefaultPosition, wxSize(420, 72));
        myTicketsButton->SetFont(wxFont(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                        wxFONTWEIGHT_BOLD));
        myTicketsButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            auto* tickets = new TicketsFrame(this);
            tickets->Show(true);
        });
        buttons->Add(myTicketsButton, 0, wxEXPAND | wxBOTTOM, 16);

        pageLayout->Add(buttons, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 24);
        pageLayout->AddStretchSpacer(1);
        panel->SetSizer(pageLayout);

        auto* frameLayout = new wxBoxSizer(wxVERTICAL);
        frameLayout->Add(panel, 1, wxEXPAND);
        SetSizer(frameLayout);
        Centre(wxBOTH);
    }
};

class LoginDialog : public wxDialog {
public:
    explicit LoginDialog(wxWindow* parent)
        : wxDialog(parent, wxID_ANY, _( "Ticket-Support" ), wxDefaultPosition,
                   wxSize(480, 440), wxDEFAULT_DIALOG_STYLE) {
        auto* panel = new wxPanel(this, wxID_ANY);
        panel->SetBackgroundColour(wxColour(248, 250, 253));

        auto* layout = new wxBoxSizer(wxVERTICAL);
        layout->AddStretchSpacer(1);

        auto* title = new wxStaticText(panel, wxID_ANY, _( "Ticket-Support" ));
        title->SetFont(wxFont(24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                              wxFONTWEIGHT_BOLD));
        title->SetForegroundColour(wxColour(29, 44, 66));
        layout->Add(title, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 24);

        auto* nameLabel = new wxStaticText(panel, wxID_ANY, _( "Name" ));
        layout->Add(nameLabel, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxBOTTOM, 6);

        auto* nameInput = new wxTextCtrl(panel, wxID_ANY, wxEmptyString,
                                         wxDefaultPosition, wxSize(320, -1));
        layout->Add(nameInput, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxBOTTOM, 16);

        auto* passwordLabel = new wxStaticText(panel, wxID_ANY, _( "Passwort" ));
        layout->Add(passwordLabel, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxBOTTOM, 6);

        auto* passwordInput = new wxTextCtrl(panel, wxID_ANY, wxEmptyString,
                                             wxDefaultPosition, wxSize(320, -1), wxTE_PASSWORD);
        layout->Add(passwordInput, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxBOTTOM, 24);

        auto* loginButton = new wxButton(panel, wxID_ANY, _( "Einloggen" ),
                                         wxDefaultPosition, wxSize(240, 56));
        loginButton->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                    wxFONTWEIGHT_BOLD));
        loginButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            EndModal(wxID_OK);
        });
        layout->Add(loginButton, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 24);

        layout->AddStretchSpacer(1);
        panel->SetSizer(layout);

        auto* frameLayout = new wxBoxSizer(wxVERTICAL);
        frameLayout->Add(panel, 1, wxEXPAND);
        SetSizer(frameLayout);
        SetMinSize(wxSize(440, 400));
        CentreOnParent();
    }
};

class HomeFrame : public MyFrame1 {
public:
    HomeFrame() : MyFrame1(nullptr) {
        m_loginButton->Bind(wxEVT_BUTTON, &HomeFrame::OnOpenLogin, this);
    }

private:
    void OnOpenLogin(wxCommandEvent&) {
        LoginDialog dialog(this);
        if (dialog.ShowModal() == wxID_OK) {
            auto* actions = new TicketActionsFrame();
            actions->Show(true);
        }
    }
};

class MyApp : public wxApp {
public:
    bool OnInit() override {
        HomeFrame* frame = new HomeFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);
