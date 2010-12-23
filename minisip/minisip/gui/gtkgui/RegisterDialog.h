#ifndef REGISTER_DIALOG
#define REGISTER_DIALOG

#include<config.h>
#include<libmutil/MemObject.h>
#include <libmsip/SipDialogConfig.h>
#include <libglademm/xml.h>
#include <gtkmm.h>

class LoginDialog;

class RegisterDialog
{
	public:
		RegisterDialog( Glib::RefPtr<Gnome::Glade::Xml>  refXml/*,
				LoginDialog * loginDialog*/);
		
		~RegisterDialog();
		 
		void show();
		void hide();
		void registerAccount();
		void reset();
			
	private:
		Glib::RefPtr<Gnome::Glade::Xml> refXml;
		LoginDialog * loginDialog;
		
		Gtk::Window * registerDialogWidget;
		Gtk::Entry * fullnameEntry;
		Gtk::Label * fullnameLabel;
		Gtk::Entry * nebulausernameEntry;
		Gtk::Label * nebulausernameLabel;
		Gtk::Entry * passwordnEntry;
		Gtk::Label * passwordnLabel;
		Gtk::Entry * emailaddressEntry;
		Gtk::Label * emailaddressLabel;
		Gtk::Entry * addressEntry;
		Gtk::Label * addressLabel;
		Gtk::Entry * phonenumberEntry;
		Gtk::Label * phonenumberLabel;
		
		
		
		Gtk::Button * registeraccountButton;
		Gtk::Button * resetButton;
};
#endif
