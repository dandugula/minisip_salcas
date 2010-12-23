#include<config.h>

#include"LoginDialog.h"
#include"RegisterDialog.h"
#include<libminisip/gui/Gui.h>


#ifdef OLDLIBGLADEMM
#define SLOT(a,b) SigC::slot(a,b)
#define BIND SigC::bind
#else
#define SLOT(a,b) sigc::mem_fun(a,b)
#define BIND sigc::bind
#endif

using namespace std;

RegisterDialog::RegisterDialog( Glib::RefPtr<Gnome::Glade::Xml> theRefXml/*,
				LoginDialog * loginDialog*/):
		refXml( theRefXml )
{
		
	refXml->get_widget( "registerDialog", registerDialogWidget );

	/* Connect the registeraccount and back buttons */
	
	refXml->get_widget( "fullnameEntry", fullnameEntry );
	refXml->get_widget( "fullnameLabel", fullnameLabel );
	refXml->get_widget( "nebulausernameEntry", nebulausernameEntry );
	refXml->get_widget( "nebulausernameLabel", nebulausernameLabel );
	refXml->get_widget( "passwordnEntry", passwordnEntry );
	refXml->get_widget( "passwordnLabel", passwordnLabel );	
	refXml->get_widget( "emailaddressEntry", emailaddressEntry );
	refXml->get_widget( "emailaddressLabel", emailaddressLabel );
	refXml->get_widget( "addressEntry", addressEntry );
	refXml->get_widget( "addressLabel", addressLabel );
	refXml->get_widget( "phonenumberEntry", phonenumberEntry );
	refXml->get_widget( "phonenumberLabel", phonenumberLabel );
	refXml->get_widget( "registeraccountButton", registeraccountButton );
	refXml->get_widget( "resetButton", resetButton );

	registeraccountButton->signal_clicked().connect( SLOT( *this, &RegisterDialog::registerAccount) );
	resetButton->signal_clicked().connect( SLOT( *this, &RegisterDialog::reset));

	passwordnEntry->set_visibility(FALSE);
 	registerDialogWidget->hide();
}
RegisterDialog::~RegisterDialog(){
	delete registerDialogWidget;
}
void RegisterDialog::show(){
	registerDialogWidget->show();
}
void RegisterDialog::hide(){
	registerDialogWidget->hide();
}
void RegisterDialog::registerAccount(){

	profile prof1;
	profile *prof;
	prof=&prof1;

	prof->username=nebulausernameEntry->get_text();
	prof->phoneNumber=phonenumberEntry->get_text();
	prof->fullName=fullnameEntry->get_text();
	prof->email_address=emailaddressEntry->get_text();
	prof->password=passwordnEntry->get_text();
	prof->address=addressEntry->get_text();
	
	nebula_register(prof);
	registerDialogWidget->hide();

}
void RegisterDialog::reset(){
	
	nebulausernameEntry->set_text("");
	fullnameEntry->set_text("");
	passwordnEntry->set_text("");
	addressEntry->set_text("");	
	emailaddressEntry->set_text("");
	phonenumberEntry->set_text("");
}
