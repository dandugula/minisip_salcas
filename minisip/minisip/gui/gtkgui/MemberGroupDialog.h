#ifndef GROUPMEMBER_DIALOG
#define GROUPMEMBER_DIALOG

#include<config.h>
#include<libmutil/MemObject.h>
#include <libmsip/SipDialogConfig.h>
#include <libglademm/xml.h>
#include <gtkmm.h>

//#include"profile_mgmt_client.h"


class MemberGroupDialog
{
	public:
		MemberGroupDialog( Glib::RefPtr<Gnome::Glade::Xml>  refXml);
		
		~MemberGroupDialog();
		
						 
		void show();
		void hide();
		void addMember();
		void editMemberGroup();
		void removeMemberGroup();
		void reset();
		void updateGroupList();
		void groupListSelect();
		Glib::ustring getcontactNameString();
		Glib::ustring getnebulaNameString();
					
	private:
		Glib::RefPtr<Gnome::Glade::Xml> refXml;
		GroupDialog * groupDialog;
		
		Gtk::Window * membergroupDialogWidget;
		Gtk::Entry * contactnameEntry;
		Gtk::Label * contactnameLabel;
		Gtk::Entry * nebulanameEntry;
		Gtk::Label * nebulanameLabel;
		Gtk::ComboBox * selectgroupList;
                 GtkComboBox *groupList;
		Gtk::Label * selectgroupLabel;
				
		Gtk::Button * addmemberButton;
		Gtk::Button * resetButton;
                Gtk::TreeView *GroupTreeView;
                int groupId;
};
#endif
