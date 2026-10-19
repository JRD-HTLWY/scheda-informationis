#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <wx/statline.h>
#include "aeroTheme.h"

// Route every scrollbar action through one handler for the associated view.
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

// Shows a ticket's details, conversation, reply input, and footer actions.
class TicketDetailFrame : public wxFrame {
public:
    explicit TicketDetailFrame(wxWindow* parent = nullptr)
        : wxFrame(parent, wxID_ANY, _( "Ticket-Support" ), wxDefaultPosition,
                  wxSize(900, 700), wxDEFAULT_FRAME_STYLE) {
        wxTopLevelWindowGTK::SetMinSize(wxSize(720, 560));

        auto* panel = new aero::Panel(this, aero::palette::skyTop,
                                      aero::palette::skyBottom, false, true);

        auto* pageLayout = new wxBoxSizer(wxVERTICAL);
        auto* header = new wxBoxSizer(wxHORIZONTAL);

        // Center the ticket title while keeping the username aligned to the right.
        auto* title = new wxStaticText(panel, wxID_ANY, _( "Ticket" ));
        title->SetFont(wxFont(24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                              wxFONTWEIGHT_BOLD));
        title->SetForegroundColour(aero::palette::ink);
        header->Add(120, 0);
        header->AddStretchSpacer(1);
        header->Add(title, 0, wxALIGN_CENTER_VERTICAL);
        header->AddStretchSpacer(1);

        auto* username = new wxStaticText(panel, wxID_ANY, _( "Benutzername" ));
        username->SetMinSize(wxSize(120, -1));
        username->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                 wxFONTWEIGHT_NORMAL));
        username->SetForegroundColour(aero::palette::mutedInk);
        header->Add(username, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        pageLayout->Add(header, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 28);

        // Group the ticket's key fields in a lightly tinted metadata panel.
        auto* detailsPanel = new aero::Panel(panel, aero::palette::glassTop,
                                             aero::palette::glassBottom);
        auto* detailsLayout = new wxBoxSizer(wxVERTICAL);
        detailsLayout->AddSpacer(12);
        auto addDetail = [&](const wxString& label) {
            auto* text = new wxStaticText(detailsPanel, wxID_ANY, label);
            text->SetForegroundColour(aero::palette::ink);
            detailsLayout->Add(text, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
        };
        addDetail(_( "Titel: $Tickettitel" ));
        addDetail(_( "Ticket-Nummer: $ID" ));

        auto* statusLine = new wxBoxSizer(wxHORIZONTAL);
        auto* statusLabel = new wxStaticText(detailsPanel, wxID_ANY, _( "Ticket Status: " ));
        statusLabel->SetForegroundColour(aero::palette::ink);
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
        addDetail(_( "Kunde: $Kunde" ));
        detailsLayout->AddSpacer(4);
        detailsPanel->SetSizer(detailsLayout);
        pageLayout->Add(detailsPanel, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP, 24);
        detailsPanel->SetMinSize(wxSize(360, -1));

        // Keep the conversation independently scrollable above the reply field.
        auto* conversationPanel = new aero::Panel(panel, aero::palette::surfaceTop,
                                                  aero::palette::surfaceBottom);
        auto* conversationLayout = new wxBoxSizer(wxVERTICAL);

        auto* conversation = new wxScrolledWindow(conversationPanel, wxID_ANY,
                                                   wxDefaultPosition, wxDefaultSize,
                                                   wxVSCROLL | wxBORDER_NONE);
        conversation->SetBackgroundColour(wxColour(246, 252, 255));
        conversation->SetScrollRate(0, 10);
        auto* messageLayout = new wxBoxSizer(wxVERTICAL);

        auto* customerBubble = new aero::Panel(conversation, aero::palette::glassTop,
                                               aero::palette::glassBottom);
        customerBubble->SetMinSize(wxSize(260, 58));
        customerBubble->SetMaxSize(wxSize(460, -1));
        auto* customerBubbleLayout = new wxBoxSizer(wxVERTICAL);
        auto* customerMessage = new wxStaticText(customerBubble, wxID_ANY,
                                                  _( "Kunde: $Inhalt" ));
        customerMessage->SetForegroundColour(aero::palette::ink);
        customerBubbleLayout->Add(customerMessage, 0, wxALL, 14);
        customerBubble->SetSizer(customerBubbleLayout);
        messageLayout->Add(customerBubble, 0, wxLEFT | wxRIGHT | wxTOP, 16);
        messageLayout->AddSpacer(170);

        auto* employeeBubble = new aero::Panel(conversation,
                                               wxColour(224, 246, 255),
                                               wxColour(203, 235, 249));
        employeeBubble->SetMinSize(wxSize(250, 72));
        employeeBubble->SetMaxSize(wxSize(420, -1));
        auto* employeeBubbleLayout = new wxBoxSizer(wxVERTICAL);
        auto* employeeMessage = new wxStaticText(employeeBubble, wxID_ANY,
                                                  _( "Bearbeiter:\n$Antwort" ));
        employeeMessage->SetForegroundColour(aero::palette::mutedInk);
        employeeMessage->Wrap(340);
        employeeBubbleLayout->Add(employeeMessage, 0, wxALL, 14);
        employeeBubble->SetSizer(employeeBubbleLayout);
        messageLayout->Add(employeeBubble, 0, wxALIGN_RIGHT | wxRIGHT, 28);
        messageLayout->AddSpacer(180);
        conversation->SetSizer(messageLayout);
        conversation->FitInside();
        // The adjacent scrollbar controls this view, so hide its built-in bars.
        conversation->ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_NEVER);
        auto* conversationBody = new wxBoxSizer(wxHORIZONTAL);
        conversationBody->Add(conversation, 1, wxEXPAND | wxALL, 12);
        auto* conversationScrollbar = new wxScrollBar(
            conversationPanel, wxID_ANY, wxDefaultPosition, wxSize(16, -1),
            wxSB_VERTICAL);
        conversationScrollbar->SetScrollbar(0, 20, 60, 20);
        BindScrollBarEvents(conversationScrollbar, [conversation](const wxScrollEvent& event) {
            conversation->Scroll(0, event.GetPosition());
        });
        conversationBody->Add(conversationScrollbar, 0,
                              wxEXPAND | wxTOP | wxRIGHT | wxBOTTOM, 12);
        conversationLayout->Add(conversationBody, 1, wxEXPAND);

        auto* replyInput = new wxTextCtrl(conversationPanel, wxID_ANY, wxEmptyString,
                                          wxDefaultPosition, wxSize(-1, 44),
                                          wxTE_PROCESS_ENTER);
        replyInput->SetHint(_( "Eingabe......" ));
        replyInput->SetBackgroundColour(wxColour(255, 255, 255));
        replyInput->SetForegroundColour(aero::palette::ink);
        conversationLayout->Add(replyInput, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
        conversationPanel->SetSizer(conversationLayout);
        pageLayout->Add(conversationPanel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP,
                        48);

        auto* backButton = new aero::Button(panel, wxID_ANY, _( "zurück" ),
                                            wxSize(180, 42));
        backButton->SetFont(wxFont(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                   wxFONTWEIGHT_BOLD));
        backButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Close(); });
        auto* sendButton = new aero::Button(panel, wxID_ANY, _( "Senden" ),
                                            wxSize(180, 42));
        sendButton->SetFont(wxFont(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                   wxFONTWEIGHT_BOLD));
        auto* actionLayout = new wxBoxSizer(wxHORIZONTAL);
        actionLayout->Add(backButton, 0);
        actionLayout->AddStretchSpacer(1);
        actionLayout->Add(sendButton, 0);
        pageLayout->Add(actionLayout, 0,
                        wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 48);

        panel->SetSizer(pageLayout);
        auto* frameLayout = new wxBoxSizer(wxVERTICAL);
        frameLayout->Add(panel, 1, wxEXPAND);
        SetSizer(frameLayout);
        Centre(wxBOTH);
        // Wait for layout sizing before matching the scrollbar thumb to the view.
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

// Displays ticket rows and opens a detail window when a row is double-clicked.
class TicketsListFrame : public wxFrame {
public:
    explicit TicketsListFrame(wxWindow* parent = nullptr)
        : wxFrame(parent, wxID_ANY, _( "Ticket-Support" ), wxDefaultPosition,
                  wxSize(900, 680), wxDEFAULT_FRAME_STYLE) {
        wxTopLevelWindowGTK::SetMinSize(wxSize(720, 540));

        auto* panel = new aero::Panel(this, aero::palette::skyTop,
                                      aero::palette::skyBottom, false, true);

        auto* pageLayout = new wxBoxSizer(wxVERTICAL);
        auto* header = new wxBoxSizer(wxHORIZONTAL);

        auto* title = new wxStaticText(panel, wxID_ANY, _( "Tickets" ));
        title->SetFont(wxFont(28, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                              wxFONTWEIGHT_BOLD));
        title->SetForegroundColour(aero::palette::ink);
        header->Add(120, 0);
        header->AddStretchSpacer(1);
        header->Add(title, 0, wxALIGN_CENTER_VERTICAL);
        header->AddStretchSpacer(1);

        auto* username = new wxStaticText(panel, wxID_ANY, _( "Nutzername" ));
        username->SetMinSize(wxSize(120, -1));
        username->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                 wxFONTWEIGHT_NORMAL));
        username->SetForegroundColour(aero::palette::mutedInk);
        header->Add(username, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        pageLayout->Add(header, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 28);

        pageLayout->AddStretchSpacer(1);

        auto* listPanel = new aero::Panel(panel, aero::palette::glassTop,
                                          aero::palette::glassBottom);
        auto* listPanelLayout = new wxBoxSizer(wxVERTICAL);
        auto* listBodyLayout = new wxBoxSizer(wxHORIZONTAL);
        auto* ticketScroll = new wxScrolledWindow(listPanel, wxID_ANY,
                                                   wxDefaultPosition, wxDefaultSize,
                                                   wxVSCROLL | wxBORDER_NONE);
        ticketScroll->SetBackgroundColour(wxColour(247, 253, 255));
        ticketScroll->SetScrollRate(0, 10);

        auto* ticketRows = new wxBoxSizer(wxVERTICAL);
        // Give each ticket consistent columns and one shared double-click action.
        auto addTicketRow = [&](const wxString& id, const wxString& customer,
                                const wxString& date, const wxString& subject) {
            auto* row = new aero::Panel(ticketScroll, aero::palette::surfaceTop,
                                        aero::palette::surfaceBottom);
            row->SetMinSize(wxSize(-1, 44));
            auto* rowLayout = new wxBoxSizer(wxHORIZONTAL);

            auto makeCell = [&](const wxString& label, int width) {
                auto* cell = new wxStaticText(row, wxID_ANY, label);
                cell->SetMinSize(wxSize(width, -1));
                cell->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                     wxFONTWEIGHT_NORMAL));
                cell->SetForegroundColour(aero::palette::ink);
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
        // Use an external scrollbar alongside the row view.
        ticketScroll->ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_NEVER);
        listBodyLayout->Add(ticketScroll, 1, wxEXPAND | wxALL, 12);
        auto* listScrollbar = new wxScrollBar(
            listPanel, wxID_ANY, wxDefaultPosition, wxSize(16, -1), wxSB_VERTICAL);
        listScrollbar->SetScrollbar(0, 1, 10, 1);
        BindScrollBarEvents(listScrollbar, [ticketScroll](const wxScrollEvent& event) {
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
        // Compute the visible page size after the frame and its children are laid out.
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

// Shows the ticket overview with its rows and scrollbar.
class TicketsOverviewFrame : public wxFrame {
public:
    explicit TicketsOverviewFrame(wxWindow* parent = nullptr)
        : wxFrame(parent, wxID_ANY, _( "Tickets" ), wxDefaultPosition,
                  wxSize(1200, 800), wxDEFAULT_FRAME_STYLE) {
        SetMinSize(wxSize(800, 600));
        auto* panel = new aero::Panel(this, aero::palette::skyTop,
                                      aero::palette::skyBottom, false, true);

        auto* pageLayout = new wxBoxSizer(wxVERTICAL);
        auto* header = new wxBoxSizer(wxHORIZONTAL);

        auto* title = new wxStaticText(panel, wxID_ANY, _( "Tickets" ));
        title->SetFont(wxFont(28, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                              wxFONTWEIGHT_NORMAL));
        title->SetForegroundColour(aero::palette::ink);
        header->Add(150, 0);
        header->AddStretchSpacer(1);
        header->Add(title, 0, wxALIGN_CENTER_VERTICAL);
        header->AddStretchSpacer(1);

        auto* username = new wxStaticText(panel, wxID_ANY, _( "Nutzername" ));
        username->SetMinSize(wxSize(150, -1));
        username->SetFont(wxFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                 wxFONTWEIGHT_NORMAL));
        username->SetForegroundColour(aero::palette::mutedInk);
        header->Add(username, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        pageLayout->Add(header, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 52);
        pageLayout->AddStretchSpacer(1);

        auto* contentLayout = new wxBoxSizer(wxHORIZONTAL);
        contentLayout->AddStretchSpacer(1);

        auto* listPanel = new aero::Panel(panel, aero::palette::glassTop,
                                          aero::palette::glassBottom);
        auto* listPanelLayout = new wxBoxSizer(wxVERTICAL);
        auto* ticketBodyLayout = new wxBoxSizer(wxHORIZONTAL);
        auto* ticketRowsPanel = new aero::Panel(listPanel, aero::palette::surfaceTop,
                                                aero::palette::surfaceBottom);
        auto* ticketRows = new wxBoxSizer(wxVERTICAL);
        ticketRows->AddSpacer(8);

        // Add a row of ticket fields and let any field open the detail window.
        auto addTicketRow = [&]() {
            auto* row = new aero::Panel(ticketRowsPanel, aero::palette::glassTop,
                                        aero::palette::glassBottom);
            row->SetMinSize(wxSize(-1, 40));
            auto* rowLayout = new wxBoxSizer(wxHORIZONTAL);

            auto makeCell = [&](const wxString& label, int width) {
                auto* cell = new wxStaticText(row, wxID_ANY, label);
                cell->SetMinSize(wxSize(width, -1));
                cell->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                     wxFONTWEIGHT_NORMAL));
                cell->SetForegroundColour(aero::palette::ink);
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

        // This overview uses the native scrollbar control beside its row panel.
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

// Main ticket menu for navigating to the ticket list and overview.
class TicketsFrame : public wxFrame {
public:
    explicit TicketsFrame(wxWindow* parent = nullptr)
        : wxFrame(parent, wxID_ANY, _( "Ticket-Support" ), wxDefaultPosition,
                  wxSize(900, 600), wxDEFAULT_FRAME_STYLE) {
        wxTopLevelWindowGTK::SetMinSize(wxSize(640, 440));

        auto* panel = new aero::Panel(this, aero::palette::skyTop,
                                      aero::palette::skyBottom, false, true);

        auto* pageLayout = new wxBoxSizer(wxVERTICAL);
        auto* header = new wxBoxSizer(wxHORIZONTAL);

        auto* title = new wxStaticText(panel, wxID_ANY, _( "Ticket-Support" ));
        title->SetFont(wxFont(24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                              wxFONTWEIGHT_BOLD));
        title->SetForegroundColour(aero::palette::ink);
        header->Add(120, 0);
        header->AddStretchSpacer(1);
        header->Add(title, 0, wxALIGN_CENTER_VERTICAL);
        header->AddStretchSpacer(1);

        auto* username = new wxStaticText(panel, wxID_ANY, _( "Benutzername" ));
        username->SetMinSize(wxSize(120, -1));
        username->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                 wxFONTWEIGHT_NORMAL));
        username->SetForegroundColour(aero::palette::mutedInk);
        header->Add(username, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);

        pageLayout->Add(header, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 28);
        pageLayout->AddStretchSpacer(1);

        auto* ticketButtons = new wxBoxSizer(wxVERTICAL);
        // Keep the navigation buttons visually consistent in size and type.
        auto* freeTicketsButton = new aero::Button(panel, wxID_ANY,
                                                   _( "freie Tickets anzeigen" ),
                                                   wxSize(420, 64));
        auto* myTicketsButton = new aero::Button(panel, wxID_ANY,
                                                 _( "deine Tickets anzeigen" ),
                                                 wxSize(420, 64));
        myTicketsButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            auto* tickets = new TicketsListFrame(this);
            tickets->Show(true);
        });
        auto* closedTicketsButton = new aero::Button(panel, wxID_ANY,
                                                     _( "geschlossene Tickets" ),
                                                     wxSize(420, 64));
        auto* overviewButton = new aero::Button(panel, wxID_ANY,
                                                _( "Ticketübersicht anzeigen" ),
                                                wxSize(420, 64));
        overviewButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            auto* overview = new TicketsOverviewFrame(this);
            overview->Show(true);
        });
        auto styleAndAddButton = [&](aero::Button* button) {
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

// Presents the ticket actions available after a successful login.
class [[maybe_unused]] TicketActionsFrame : public wxFrame {
public:
    explicit TicketActionsFrame()
        : wxFrame(nullptr, wxID_ANY, _( "Ticket-Support" ), wxDefaultPosition,
                  wxSize(900, 600), wxDEFAULT_FRAME_STYLE) {
        wxTopLevelWindowGTK::SetMinSize(wxSize(640, 440));

        auto* panel = new aero::Panel(this, aero::palette::skyTop,
                                      aero::palette::skyBottom, false, true);

        auto* pageLayout = new wxBoxSizer(wxVERTICAL);
        auto* header = new wxBoxSizer(wxHORIZONTAL);

        auto* title = new wxStaticText(panel, wxID_ANY, _( "Ticket-Support" ));
        title->SetFont(wxFont(24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                              wxFONTWEIGHT_BOLD));
        title->SetForegroundColour(aero::palette::ink);
        header->Add(120, 0);
        header->AddStretchSpacer(1);
        header->Add(title, 0, wxALIGN_CENTER_VERTICAL);
        header->AddStretchSpacer(1);

        auto* username = new wxStaticText(panel, wxID_ANY, _( "Benutzername" ));
        username->SetMinSize(wxSize(120, -1));
        username->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                 wxFONTWEIGHT_NORMAL));
        username->SetForegroundColour(aero::palette::mutedInk);
        header->Add(username, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);

        pageLayout->Add(header, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 28);
        pageLayout->AddStretchSpacer(1);

        auto* buttons = new wxBoxSizer(wxVERTICAL);
        auto* createTicketButton = new aero::Button(panel, wxID_ANY,
                                                    _( "Ticket erstellen" ),
                                                    wxSize(420, 72));
        createTicketButton->SetFont(wxFont(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                           wxFONTWEIGHT_BOLD));
        buttons->Add(createTicketButton, 0, wxEXPAND | wxBOTTOM, 16);

        auto* myTicketsButton = new aero::Button(panel, wxID_ANY,
                                                 _( "Meine Tickets anzeigen" ),
                                                 wxSize(420, 72));
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

// Collects credentials and reports failed attempts until authentication is added.
class LoginDialog : public wxDialog {
public:
    explicit LoginDialog(wxWindow* parent)
        : wxDialog(parent, wxID_ANY, _( "Ticket-Support" ), wxDefaultPosition,
                   wxSize(480, 440), wxDEFAULT_DIALOG_STYLE) {
        auto* panel = new aero::Panel(this, aero::palette::skyTop,
                                      aero::palette::skyBottom, false, true);

        auto* layout = new wxBoxSizer(wxVERTICAL);
        layout->AddStretchSpacer(1);

        auto* title = new wxStaticText(panel, wxID_ANY, _( "Ticket-Support" ));
        title->SetFont(wxFont(24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                              wxFONTWEIGHT_BOLD));
        title->SetForegroundColour(aero::palette::ink);
        layout->Add(title, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 24);

        auto* nameLabel = new wxStaticText(panel, wxID_ANY, _( "Name" ));
        nameLabel->SetForegroundColour(aero::palette::mutedInk);
        layout->Add(nameLabel, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxBOTTOM, 6);

        auto* nameInput = new wxTextCtrl(panel, wxID_ANY, wxEmptyString,
                                         wxDefaultPosition, wxSize(320, -1));
        nameInput->SetMinSize(wxSize(320, 38));
        nameInput->SetBackgroundColour(wxColour(255, 255, 255));
        nameInput->SetForegroundColour(aero::palette::ink);
        layout->Add(nameInput, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxBOTTOM, 16);

        auto* passwordLabel = new wxStaticText(panel, wxID_ANY, _( "Passwort" ));
        passwordLabel->SetForegroundColour(aero::palette::mutedInk);
        layout->Add(passwordLabel, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxBOTTOM, 6);

        auto* passwordInput = new wxTextCtrl(panel, wxID_ANY, wxEmptyString,
                                             wxDefaultPosition, wxSize(320, -1), wxTE_PASSWORD);
        passwordInput->SetMinSize(wxSize(320, 38));
        passwordInput->SetBackgroundColour(wxColour(255, 255, 255));
        passwordInput->SetForegroundColour(aero::palette::ink);
        layout->Add(passwordInput, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxBOTTOM, 24);

        auto* loginButton = new aero::Button(panel, wxID_ANY, _( "Einloggen" ),
                                             wxSize(240, 56));
        loginButton->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                    wxFONTWEIGHT_BOLD));
        // Authentication is not implemented yet, so each attempt shows the error.
        loginButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            ShowInvalidCredentialsMessage();
        });
        layout->Add(loginButton, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 24);

        layout->AddStretchSpacer(1);
        panel->SetSizer(layout);

        auto* frameLayout = new wxBoxSizer(wxVERTICAL);
        frameLayout->Add(panel, 1, wxEXPAND);
        SetSizer(frameLayout);
        wxTopLevelWindowGTK::SetMinSize(wxSize(440, 400));
        CentreOnParent();
    }

private:
    // Display the login error without closing the dialog.
    void ShowInvalidCredentialsMessage() {
        wxDialog errorDialog(this, wxID_ANY, _( "Login fehlgeschlagen" ),
                             wxDefaultPosition, wxSize(380, 190),
                             wxDEFAULT_DIALOG_STYLE);
        auto* panel = new aero::Panel(&errorDialog, aero::palette::skyTop,
                                      aero::palette::skyBottom, false, true);
        auto* layout = new wxBoxSizer(wxVERTICAL);
        auto* message = new wxStaticText(panel, wxID_ANY,
                                         _( "falsche Anmeldedaten!" ));
        message->SetForegroundColour(aero::palette::ink);
        layout->AddStretchSpacer(1);
        layout->Add(message, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT | wxBOTTOM, 18);
        auto* okButton = new aero::Button(panel, wxID_OK, _( "OK" ), wxSize(120, 42));
        okButton->Bind(wxEVT_BUTTON, [&errorDialog](wxCommandEvent&) {
            errorDialog.EndModal(wxID_OK);
        });
        layout->Add(okButton, 0, wxALIGN_CENTER | wxBOTTOM, 18);
        layout->AddStretchSpacer(1);
        panel->SetSizer(layout);

        auto* frameLayout = new wxBoxSizer(wxVERTICAL);
        frameLayout->Add(panel, 1, wxEXPAND);
        errorDialog.SetSizer(frameLayout);
        errorDialog.CentreOnParent();
        errorDialog.ShowModal();
    }
};

class HomeFrame : public wxFrame {
public:
    HomeFrame()
        : wxFrame(nullptr, wxID_ANY, _( "Ticket-Support" ), wxDefaultPosition,
                  wxSize(640, 440), wxDEFAULT_FRAME_STYLE) {
        SetMinSize(wxSize(480, 360));

        auto* panel = new aero::Panel(this, aero::palette::skyTop,
                                      aero::palette::skyBottom, false, true);
        auto* layout = new wxBoxSizer(wxVERTICAL);
        layout->AddStretchSpacer(1);

        auto* title = new wxStaticText(panel, wxID_ANY, _( "Ticket-Support" ));
        title->SetFont(wxFont(28, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                              wxFONTWEIGHT_BOLD));
        title->SetForegroundColour(aero::palette::ink);
        layout->Add(title, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 32);

        auto* loginButton = new aero::Button(panel, wxID_ANY, _( "Einloggen" ),
                                             wxSize(280, 72));
        loginButton->SetFont(wxFont(17, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                    wxFONTWEIGHT_BOLD));
        loginButton->Bind(wxEVT_BUTTON, &HomeFrame::OnOpenLogin, this);
        layout->Add(loginButton, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 24);
        layout->AddStretchSpacer(1);
        panel->SetSizer(layout);

        auto* frameLayout = new wxBoxSizer(wxVERTICAL);
        frameLayout->Add(panel, 1, wxEXPAND);
        SetSizer(frameLayout);
        Centre(wxBOTH);
    }

private:
    // Open the modal login dialog and continue only if it reports success.
    void OnOpenLogin(wxCommandEvent&) {
        LoginDialog dialog(this);
        dialog.ShowModal();
    }
};

class MyApp : public wxApp {
public:
    // Create and display the FormBuilder-generated home window at startup.
    bool OnInit() override {
        const auto frame = new HomeFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);
