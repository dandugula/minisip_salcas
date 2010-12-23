#include<config.h>

#include"LoginDialog.h"


#ifdef OLDLIBGLADEMM
#define SLOT(a,b) SigC::slot(a,b)
#define BIND SigC::bind
#else
#define SLOT(a,b) sigc::mem_fun(a,b)
#define BIND sigc::bind
#endif

using namespace std;


LoginDialog::LoginDialog( Glib::RefPtr<Gnome::Glade::Xml> theRefXml):
		refXml( theRefXml )
{
	registerDialog = new RegisterDialog( refXml );
	//mainTabWidget = manage( new Gtk::Notebook() );
	refXml->get_widget( "loginDialog", loginDialogWidget );
	//refXml->get_widget( "contactVBox", wContact );
	
	/* Connect the signin and createaccount buttons */
	
	refXml->get_widget( "nusernameEntry", nusernameEntry );
	refXml->get_widget( "nusernameLabel", nusernameLabel );
	refXml->get_widget( "npasswordEntry", npasswordEntry );
	refXml->get_widget( "npasswordLabel", npasswordLabel );
	refXml->get_widget( "signinButton", signinButton );
	refXml->get_widget( "createaccountButton", createaccountButton );
        refXml->get_widget( "treeView1", groupContactTreeView);

	createaccountButton->signal_clicked().connect( SLOT( *registerDialog, &RegisterDialog::show) );
	signinButton->signal_clicked().connect( SLOT( *this, &LoginDialog::signIn) );
	npasswordEntry->set_visibility(FALSE);
 	loginDialogWidget->hide();
}

LoginDialog::~LoginDialog(){
	delete loginDialogWidget;
}
void LoginDialog::show(){
	loginDialogWidget->show();
}
void LoginDialog::hide(){
	loginDialogWidget->hide();
}

void LoginDialog::signIn(){

	group_details * grp;
        credentials *cred, cred1;
	cred=&cred1;
	
	cred->cred_username=nusernameEntry->get_text();
	cred->cred_password=npasswordEntry->get_text();
	
	nebula_login(cred,grp);
        loginDialogWidget->hide();
        cerr<<"calling tree view"<<endl; 
        setup_tree_view(groupContactTreeView->gobj());
        
		
}
