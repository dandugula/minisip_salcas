#ifndef GROUP_DIALOG
#define GROUP_DIALOG

#include<config.h>
#include<libmutil/MemObject.h>
#include <libmsip/SipDialogConfig.h>
#include <libglademm/xml.h>
#include <gtkmm.h>
#include "MainWindow.h"


class GroupDialog
{
	public:
		GroupDialog( Glib::RefPtr<Gnome::Glade::Xml>  refXml);
		
		~GroupDialog();
		 
		void show();
		void hide();
		void removeGroup();
		void addGroup();
		void reset();
			
	private:
		Glib::RefPtr<Gnome::Glade::Xml> refXml;
		
		
		Gtk::Window * groupDialogWidget;
		Gtk::Entry * groupnameEntry;
		Gtk::Label * groupnameLabel;
				
		Gtk::Button * addgroupButton;
		Gtk::Button * resetButton;
                Gtk::TreeView * GroupTreeView;
};
#endif
