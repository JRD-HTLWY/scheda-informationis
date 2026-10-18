///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-115-g11c2dec8)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "MyProjectBase.h"

///////////////////////////////////////////////////////////////////////////

MyFrame1::MyFrame1( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 480, 360 ), wxDefaultSize );

	wxPanel* panel = new wxPanel( this, wxID_ANY );
	panel->SetBackgroundColour( wxColour( 248, 250, 253 ) );

	wxBoxSizer* bSizer1 = new wxBoxSizer( wxVERTICAL );

	bSizer1->AddStretchSpacer( 1 );

	m_staticText1 = new wxStaticText( panel, wxID_ANY, _( "Ticket-Support" ), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_staticText1->Wrap( -1 );
	m_staticText1->SetFont( wxFont( 28, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	m_staticText1->SetForegroundColour( wxColour( 29, 44, 66 ) );
	bSizer1->Add( m_staticText1, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 32 );

	m_loginButton = new wxButton( panel, wxID_ANY, _( "Einloggen" ), wxDefaultPosition, wxSize( 280, 72 ), 0 );
	m_loginButton->SetFont( wxFont( 17, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	bSizer1->Add( m_loginButton, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 24 );

	bSizer1->AddStretchSpacer( 1 );

	panel->SetSizer( bSizer1 );
	wxBoxSizer* frameSizer = new wxBoxSizer( wxVERTICAL );
	frameSizer->Add( panel, 1, wxEXPAND );
	this->SetSizer( frameSizer );
	this->SetMinSize( wxSize( 480, 360 ) );
	this->SetSize( wxSize( 640, 440 ) );
	this->SetTitle( _( "Ticket-Support" ) );
	this->Centre( wxBOTH );

	this->Layout();
}

MyFrame1::~MyFrame1()
{
}
