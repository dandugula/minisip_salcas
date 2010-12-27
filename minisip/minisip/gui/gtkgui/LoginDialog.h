#ifndef LOGIN_DIALOG
#define LOGIN_DIALOG

#include<config.h>
#include<libmutil/MemObject.h>
#include <libmsip/SipDialogConfig.h>
#include <libglademm/xml.h>
#include <gtkmm.h>
#include"RegisterDialog.h"
#include"MainWindow.h"
#include<libminisip/gui/Gui.h>




class RegisterDialog;
 
class LoginDialog 

{
	public:
		LoginDialog( Glib::RefPtr<Gnome::Glade::Xml>  refXml);
		
		~LoginDialog();
		 
		void show();
		void hide();
		void signIn();
					
	private:
		Glib::RefPtr<Gnome::Glade::Xml> refXml;
		RegisterDialog * registerDialog;
		
		//Gtk::Notebook * mainTabWidget;
		//Gtk::Widget * wContact;
		Gtk::Window * loginDialogWidget;
		Gtk::Entry * nusernameEntry;
		Gtk::Label * nusernameLabel;
		Gtk::Entry * npasswordEntry;
		Gtk::Label * npasswordLabel;
                Gtk::TreeView * groupContactTreeView;
		Gtk::Label * nebnameLabel;

		Gtk::Button * signinButton;
		Gtk::Button * createaccountButton;
};
#endif
