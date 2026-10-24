#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <wx/statline.h>
#include <wx/listctrl.h>
#include <wx/weakref.h>
#include "aeroTheme.h"
#include "netBridge.h"

#include <exception>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

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

// Owns the TCP adapter and pumps its callbacks from wxWidgets' event loop.
class TicketSession : public wxEvtHandler {
public:
    using TicketsHandler = std::function<void(std::vector<Ticket>)>;
    using FailureHandler = std::function<void(const std::string&)>;

    TicketSession() : pollTimer_(this), responseTimer_(this) {
        Bind(wxEVT_TIMER, &TicketSession::OnPollTimer, this, pollTimer_.GetId());
        Bind(wxEVT_TIMER, &TicketSession::OnResponseTimeout, this, responseTimer_.GetId());
        manager_.setResponseHandler([this](const std::string& response) {
            try {
                auto tickets = netBridge::parseTickets(response);
                responseTimer_.Stop();
                auto callback = std::move(ticketsHandler_);
                ticketsHandler_ = {};
                failureHandler_ = {};
                if (callback) callback(std::move(tickets));
            } catch (...) {
                Fail(std::current_exception());
            }
        });
        manager_.setErrorHandler([this](networkManager::ExceptionPtr error) {
            Fail(error);
        });
    }

    void Login(const std::string& serverAddress, const LoginData& credentials,
               TicketsHandler onTickets, FailureHandler onFailure) {
        credentials_ = credentials;
        if (!Configure(serverAddress, std::move(onFailure))) return;
        ticketsHandler_ = std::move(onTickets);
        try {
            netBridge::sendFetchTickets(manager_, credentials_);
            responseTimer_.Start(15000, true);
        } catch (...) {
            Fail(std::current_exception());
        }
    }

    void FetchTickets(TicketsHandler onTickets, FailureHandler onFailure) {
        failureHandler_ = std::move(onFailure);
        ticketsHandler_ = std::move(onTickets);
        try {
            netBridge::sendFetchTickets(manager_, credentials_);
            responseTimer_.Start(15000, true);
        } catch (...) {
            Fail(std::current_exception());
        }
    }

    bool CreateTicket(const std::string& title, const std::string& question,
                      FailureHandler onFailure) {
        return SendMutation([&] { netBridge::sendCreateTicket(manager_, credentials_, title, question); },
                            std::move(onFailure));
    }

    bool AnswerTicket(const int id, const std::string& answer, FailureHandler onFailure) {
        return SendMutation([&] { netBridge::sendAnswerTicket(manager_, credentials_, id, answer); },
                            std::move(onFailure));
    }

    bool DeleteTicket(const int id, FailureHandler onFailure) {
        return SendMutation([&] { netBridge::sendDeleteTicket(manager_, credentials_, id); },
                            std::move(onFailure));
    }

    const LoginData& Credentials() const { return credentials_; }

private:
    networkManager manager_;
    wxTimer pollTimer_;
    wxTimer responseTimer_;
    LoginData credentials_;
    FailureHandler failureHandler_;
    TicketsHandler ticketsHandler_;
    bool listening_ = false;

    bool Configure(const std::string& serverAddress, FailureHandler onFailure) {
        failureHandler_ = std::move(onFailure);
        try {
            manager_.connectTo(asio::ip::make_address(serverAddress), 8300);
            if (!listening_) {
                manager_.startListening(8300);
                listening_ = true;
                pollTimer_.Start(15);
            }
            return true;
        } catch (...) {
            Fail(std::current_exception());
            return false;
        }
    }

    bool SendMutation(const std::function<void()>& send, FailureHandler onFailure) {
        try {
            send();
            return true;
        } catch (...) {
            if (onFailure) onFailure(ErrorMessage(std::current_exception()));
            return false;
        }
    }

    void Fail(const networkManager::ExceptionPtr& error) {
        responseTimer_.Stop();
        const std::string message = ErrorMessage(error);
        auto callback = std::move(failureHandler_);
        failureHandler_ = {};
        ticketsHandler_ = {};
        if (callback) callback(message);
    }

    static std::string ErrorMessage(const networkManager::ExceptionPtr& error) {
        std::string message = "Unknown network error";
        try {
            if (error) std::rethrow_exception(error);
        } catch (const std::exception& exception) {
            message = exception.what();
        } catch (...) {
        }
        return message;
    }

    void OnPollTimer(wxTimerEvent&) {
        try {
            manager_.poll();
        } catch (...) {
            Fail(std::current_exception());
        }
    }

    void OnResponseTimeout(wxTimerEvent&) {
        Fail(std::make_exception_ptr(std::runtime_error("Timed out waiting for the ticket list")));
    }
};

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

// Loads the user's ticket list as the server's only available login check.
class LoginDialog : public wxDialog {
public:
    LoginDialog(wxWindow* parent, TicketSession& session)
        : wxDialog(parent, wxID_ANY, _( "Ticket-Support" ), wxDefaultPosition,
                   wxSize(500, 520), wxDEFAULT_DIALOG_STYLE), session_(session) {
        auto* panel = new aero::Panel(this, aero::palette::skyTop,
                                      aero::palette::skyBottom, false, true);

        auto* layout = new wxBoxSizer(wxVERTICAL);
        layout->AddStretchSpacer(1);

        auto* title = new wxStaticText(panel, wxID_ANY, _( "Ticket-Support" ));
        title->SetFont(wxFont(24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                              wxFONTWEIGHT_BOLD));
        title->SetForegroundColour(aero::palette::ink);
        layout->Add(title, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 24);

        auto* serverLabel = new wxStaticText(panel, wxID_ANY, _( "Server IP-Adresse" ));
        serverLabel->SetForegroundColour(aero::palette::mutedInk);
        layout->Add(serverLabel, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 6);

        serverInput_ = new wxTextCtrl(panel, wxID_ANY, wxEmptyString,
                                      wxDefaultPosition, wxSize(320, -1));
        serverInput_->SetMinSize(wxSize(320, 38));
        serverInput_->SetHint(_( "e.g. 192.168.1.10" ));
        layout->Add(serverInput_, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 14);

        auto* nameLabel = new wxStaticText(panel, wxID_ANY, _( "Name" ));
        nameLabel->SetForegroundColour(aero::palette::mutedInk);
        layout->Add(nameLabel, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxBOTTOM, 6);

        usernameInput_ = new wxTextCtrl(panel, wxID_ANY, wxEmptyString,
                                        wxDefaultPosition, wxSize(320, -1));
        usernameInput_->SetMinSize(wxSize(320, 38));
        usernameInput_->SetBackgroundColour(wxColour(255, 255, 255));
        usernameInput_->SetForegroundColour(aero::palette::ink);
        layout->Add(usernameInput_, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxBOTTOM, 16);

        auto* passwordLabel = new wxStaticText(panel, wxID_ANY, _( "Passwort" ));
        passwordLabel->SetForegroundColour(aero::palette::mutedInk);
        layout->Add(passwordLabel, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxBOTTOM, 6);

        passwordInput_ = new wxTextCtrl(panel, wxID_ANY, wxEmptyString,
                                        wxDefaultPosition, wxSize(320, -1), wxTE_PASSWORD);
        passwordInput_->SetMinSize(wxSize(320, 38));
        passwordInput_->SetBackgroundColour(wxColour(255, 255, 255));
        passwordInput_->SetForegroundColour(aero::palette::ink);
        layout->Add(passwordInput_, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxBOTTOM, 18);

        status_ = new wxStaticText(panel, wxID_ANY, wxEmptyString);
        status_->SetForegroundColour(aero::palette::mutedInk);
        layout->Add(status_, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* loginButton = new aero::Button(panel, wxID_ANY, _( "Einloggen" ),
                                             wxSize(240, 56));
        loginButton->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                    wxFONTWEIGHT_BOLD));
        loginButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnLogin(); });
        layout->Add(loginButton, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 24);

        layout->AddStretchSpacer(1);
        panel->SetSizer(layout);

        auto* frameLayout = new wxBoxSizer(wxVERTICAL);
        frameLayout->Add(panel, 1, wxEXPAND);
        SetSizer(frameLayout);
        wxTopLevelWindowGTK::SetMinSize(wxSize(440, 400));
        CentreOnParent();
    }

    const LoginData& Credentials() const { return credentials_; }
    const std::vector<Ticket>& Tickets() const { return tickets_; }

private:
    TicketSession& session_;
    wxTextCtrl* serverInput_ = nullptr;
    wxTextCtrl* usernameInput_ = nullptr;
    wxTextCtrl* passwordInput_ = nullptr;
    wxStaticText* status_ = nullptr;
    LoginData credentials_;
    std::vector<Ticket> tickets_;
    bool pending_ = false;

    void OnLogin() {
        if (pending_) return;
        credentials_.Username = usernameInput_->GetValue().ToStdString();
        credentials_.Password = passwordInput_->GetValue().ToStdString();
        const std::string serverAddress = serverInput_->GetValue().ToStdString();
        pending_ = true;
        status_->SetLabel(_( "Connecting and loading tickets..." ));

        const wxWeakRef<LoginDialog> weakThis(this);
        session_.Login(serverAddress, credentials_,
            [weakThis](std::vector<Ticket> tickets) mutable {
                if (!weakThis) return;
                weakThis->tickets_ = std::move(tickets);
                weakThis->pending_ = false;
                weakThis->EndModal(wxID_OK);
            },
            [weakThis](const std::string& error) {
                if (!weakThis) return;
                weakThis->pending_ = false;
                weakThis->status_->SetLabel(wxString::FromUTF8(error.c_str()));
            });
    }
};

class LiveTicketDetailFrame : public wxFrame {
public:
    LiveTicketDetailFrame(wxWindow* parent, TicketSession& session, Ticket ticket)
        : wxFrame(parent, wxID_ANY, _( "Ticket details" ), wxDefaultPosition,
                  wxSize(720, 600)), session_(session), ticket_(std::move(ticket)) {
        auto* panel = new aero::Panel(this, aero::palette::skyTop,
                                      aero::palette::skyBottom, false, true);
        auto* layout = new wxBoxSizer(wxVERTICAL);

        auto* title = new wxStaticText(panel, wxID_ANY,
            wxString::Format("#%d  ", ticket_.id) + wxString::FromUTF8(ticket_.title.c_str()));
        title->SetFont(wxFont(20, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                              wxFONTWEIGHT_BOLD));
        title->SetForegroundColour(aero::palette::ink);
        layout->Add(title, 0, wxALL, 18);

        auto* metadata = new wxStaticText(panel, wxID_ANY,
            _( "Customer: " ) + wxString::FromUTF8(ticket_.customer.c_str()) +
            wxString::Format("    Status: %d    Date: ", ticket_.status) +
            wxString::FromUTF8(ticket_.dateText.c_str()));
        metadata->SetForegroundColour(aero::palette::mutedInk);
        layout->Add(metadata, 0, wxLEFT | wxRIGHT | wxBOTTOM, 18);

        auto* conversation = new wxTextCtrl(panel, wxID_ANY, FormatConversation(),
            wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
        layout->Add(conversation, 1, wxEXPAND | wxLEFT | wxRIGHT, 18);

        replyInput_ = new wxTextCtrl(panel, wxID_ANY, wxEmptyString,
            wxDefaultPosition, wxSize(-1, 90), wxTE_MULTILINE);
        replyInput_->SetHint(_( "Write a reply" ));
        layout->Add(replyInput_, 0, wxEXPAND | wxALL, 18);

        status_ = new wxStaticText(panel, wxID_ANY, wxEmptyString);
        status_->SetForegroundColour(aero::palette::mutedInk);
        layout->Add(status_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* actions = new wxBoxSizer(wxHORIZONTAL);
        auto* deleteButton = new aero::Button(panel, wxID_ANY, _( "Delete ticket" ), wxSize(170, 44));
        auto* replyButton = new aero::Button(panel, wxID_ANY, _( "Send reply" ), wxSize(170, 44));
        deleteButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { DeleteTicket(); });
        replyButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { SendReply(); });
        actions->Add(deleteButton, 0, wxRIGHT, 12);
        actions->AddStretchSpacer(1);
        actions->Add(replyButton, 0);
        layout->Add(actions, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 18);

        panel->SetSizer(layout);
        auto* frameLayout = new wxBoxSizer(wxVERTICAL);
        frameLayout->Add(panel, 1, wxEXPAND);
        SetSizer(frameLayout);
        CentreOnParent();
    }

private:
    TicketSession& session_;
    Ticket ticket_;
    wxTextCtrl* replyInput_ = nullptr;
    wxStaticText* status_ = nullptr;

    wxString FormatConversation() const {
        wxString text;
        if (ticket_.content.empty()) text += _( "No messages" );
        for (const auto& message : ticket_.content) {
            text += wxString::FromUTF8(message.name.c_str()) + ":\n";
            text += wxString::FromUTF8(message.matter.c_str()) + "\n\n";
        }
        return text;
    }

    void SendReply() {
        const std::string answer = replyInput_->GetValue().ToStdString();
        if (answer.empty()) {
            status_->SetLabel(_( "Enter a reply first." ));
            return;
        }
        const wxWeakRef<LiveTicketDetailFrame> weakThis(this);
        const bool sent = session_.AnswerTicket(ticket_.id, answer, [weakThis](const std::string& error) {
            if (weakThis) weakThis->status_->SetLabel(wxString::FromUTF8(error.c_str()));
        });
        if (!sent) return;
        status_->SetLabel(_( "Reply sent. Refresh the ticket list to confirm." ));
        replyInput_->Clear();
    }

    void DeleteTicket() {
        if (wxMessageBox(_( "Delete this ticket?" ), _( "Confirm deletion" ),
                         wxYES_NO | wxICON_QUESTION, this) != wxYES) return;
        const wxWeakRef<LiveTicketDetailFrame> weakThis(this);
        const bool sent = session_.DeleteTicket(ticket_.id, [weakThis](const std::string& error) {
            if (weakThis) weakThis->status_->SetLabel(wxString::FromUTF8(error.c_str()));
        });
        if (sent) status_->SetLabel(_( "Delete request sent. Refresh the ticket list to confirm." ));
    }
};

class LiveTicketsFrame : public wxFrame {
public:
    LiveTicketsFrame(wxWindow* parent, TicketSession& session,
                     const LoginData& credentials, std::vector<Ticket> tickets)
        : wxFrame(parent, wxID_ANY, _( "Ticket-Support" ), wxDefaultPosition,
                  wxSize(900, 650)), session_(session), credentials_(credentials),
          tickets_(std::move(tickets)) {
        auto* panel = new aero::Panel(this, aero::palette::skyTop,
                                      aero::palette::skyBottom, false, true);
        auto* layout = new wxBoxSizer(wxVERTICAL);

        auto* header = new wxBoxSizer(wxHORIZONTAL);
        auto* title = new wxStaticText(panel, wxID_ANY, _( "Tickets" ));
        title->SetFont(wxFont(25, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                              wxFONTWEIGHT_BOLD));
        title->SetForegroundColour(aero::palette::ink);
        header->Add(title, 1, wxALIGN_CENTER_VERTICAL);
        auto* userLabel = new wxStaticText(panel, wxID_ANY,
            wxString::FromUTF8(credentials_.Username.c_str()));
        userLabel->SetForegroundColour(aero::palette::mutedInk);
        header->Add(userLabel, 0, wxALIGN_CENTER_VERTICAL);
        layout->Add(header, 0, wxEXPAND | wxALL, 20);

        list_ = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                               wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES);
        list_->InsertColumn(0, _( "ID" ), wxLIST_FORMAT_LEFT, 70);
        list_->InsertColumn(1, _( "Title" ), wxLIST_FORMAT_LEFT, 300);
        list_->InsertColumn(2, _( "Customer" ), wxLIST_FORMAT_LEFT, 180);
        list_->InsertColumn(3, _( "Status" ), wxLIST_FORMAT_LEFT, 130);
        layout->Add(list_, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
        list_->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent& event) {
            OpenTicket(event.GetIndex());
        });

        status_ = new wxStaticText(panel, wxID_ANY, wxEmptyString);
        status_->SetForegroundColour(aero::palette::mutedInk);
        layout->Add(status_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 20);

        auto* actions = new wxBoxSizer(wxHORIZONTAL);
        auto* createButton = new aero::Button(panel, wxID_ANY, _( "Create ticket" ), wxSize(180, 44));
        auto* openButton = new aero::Button(panel, wxID_ANY, _( "Open selected" ), wxSize(180, 44));
        auto* refreshButton = new aero::Button(panel, wxID_ANY, _( "Refresh" ), wxSize(140, 44));
        createButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { CreateTicket(); });
        openButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            const long selected = list_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
            if (selected != -1) OpenTicket(selected);
        });
        refreshButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { RefreshTickets(); });
        actions->Add(createButton, 0, wxRIGHT, 10);
        actions->Add(openButton, 0, wxRIGHT, 10);
        actions->AddStretchSpacer(1);
        actions->Add(refreshButton, 0);
        layout->Add(actions, 0, wxEXPAND | wxALL, 20);

        panel->SetSizer(layout);
        auto* frameLayout = new wxBoxSizer(wxVERTICAL);
        frameLayout->Add(panel, 1, wxEXPAND);
        SetSizer(frameLayout);
        PopulateList();
        CentreOnParent();
    }

private:
    TicketSession& session_;
    LoginData credentials_;
    std::vector<Ticket> tickets_;
    wxListCtrl* list_ = nullptr;
    wxStaticText* status_ = nullptr;

    static wxString StatusName(const int status) {
        switch (status) {
            case 0: return _( "unverified" );
            case 1: return _( "verified" );
            case 2: return _( "in progress" );
            case 3: return _( "resolved" );
            case 4: return _( "dropped" );
            default: return wxString::Format("%d", status);
        }
    }

    void PopulateList() {
        list_->DeleteAllItems();
        for (std::size_t i = 0; i < tickets_.size(); ++i) {
            const Ticket& ticket = tickets_[i];
            const long row = list_->InsertItem(static_cast<long>(i), wxString::Format("%d", ticket.id));
            list_->SetItem(row, 1, wxString::FromUTF8(ticket.title.c_str()));
            list_->SetItem(row, 2, wxString::FromUTF8(ticket.customer.c_str()));
            list_->SetItem(row, 3, StatusName(ticket.status));
        }
        status_->SetLabel(wxString::FromUTF8(std::to_string(tickets_.size()).c_str()) +
                          _( " ticket(s)" ));
    }

    void OpenTicket(const long row) {
        if (row < 0 || static_cast<std::size_t>(row) >= tickets_.size()) return;
        auto* detail = new LiveTicketDetailFrame(this, session_, tickets_[static_cast<std::size_t>(row)]);
        detail->Show(true);
    }

    void RefreshTickets() {
        status_->SetLabel(_( "Loading tickets..." ));
        const wxWeakRef<LiveTicketsFrame> weakThis(this);
        session_.FetchTickets([weakThis](std::vector<Ticket> tickets) mutable {
            if (!weakThis) return;
            weakThis->tickets_ = std::move(tickets);
            weakThis->PopulateList();
        }, [weakThis](const std::string& error) {
            if (weakThis) weakThis->status_->SetLabel(wxString::FromUTF8(error.c_str()));
        });
    }

    void CreateTicket() {
        wxDialog dialog(this, wxID_ANY, _( "Create ticket" ), wxDefaultPosition,
                        wxSize(500, 400), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        auto* layout = new wxBoxSizer(wxVERTICAL);
        layout->Add(new wxStaticText(&dialog, wxID_ANY, _( "Title" )), 0, wxALL, 10);
        auto* titleInput = new wxTextCtrl(&dialog, wxID_ANY);
        layout->Add(titleInput, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
        layout->Add(new wxStaticText(&dialog, wxID_ANY, _( "Question" )), 0, wxALL, 10);
        auto* questionInput = new wxTextCtrl(&dialog, wxID_ANY, wxEmptyString,
                                             wxDefaultPosition, wxSize(-1, 180), wxTE_MULTILINE);
        layout->Add(questionInput, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
        layout->Add(dialog.CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0,
                    wxEXPAND | wxALL, 10);
        dialog.SetSizerAndFit(layout);
        if (dialog.ShowModal() != wxID_OK) return;

        const std::string title = titleInput->GetValue().ToStdString();
        const std::string question = questionInput->GetValue().ToStdString();
        if (title.empty() || question.empty()) {
            status_->SetLabel(_( "Both title and question are required." ));
            return;
        }
        const wxWeakRef<LiveTicketsFrame> weakThis(this);
        const bool sent = session_.CreateTicket(title, question, [weakThis](const std::string& error) {
            if (weakThis) weakThis->status_->SetLabel(wxString::FromUTF8(error.c_str()));
        });
        if (sent) status_->SetLabel(_( "Ticket request sent. Refresh to see the result." ));
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
    TicketSession session_;

    // Load the first ticket list, then open the live ticket view.
    void OnOpenLogin(wxCommandEvent&) {
        LoginDialog dialog(this, session_);
        if (dialog.ShowModal() != wxID_OK) return;
        auto* tickets = new LiveTicketsFrame(this, session_, dialog.Credentials(), dialog.Tickets());
        tickets->Show(true);
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
