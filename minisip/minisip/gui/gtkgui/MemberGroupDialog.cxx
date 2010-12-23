#include<config.h>

#include"GroupDialog.h"
#include"MemberGroupDialog.h"
#include<libminisip/gui/Gui.h>
#include "profile_mgmt_client.h"


#ifdef OLDLIBGLADEMM
#define SLOT(a,b) SigC::slot(a,b)
#define BIND SigC::bind
#else
#define SLOT(a,b) sigc::mem_fun(a,b)
#define BIND sigc::bind
#endif

using namespace std;

MemberGroupDialog::MemberGroupDialog( Glib::RefPtr<Gnome::Glade::Xml> theRefXml):
		refXml( theRefXml )
{
	refXml->get_widget( "addmemberGroup", membergroupDialogWidget );

	/* Connect the Add and Back buttons */
	
	refXml->get_widget( "contactnameEntry", contactnameEntry );
	refXml->get_widget( "contactnameLabel", contactnameLabel );
	refXml->get_widget( "nebulanameEntry", nebulanameEntry );
	refXml->get_widget( "nebulanameLabel", nebulanameLabel );
	refXml->get_widget( "selectgroupList", selectgroupList );
	refXml->get_widget( "selectgroupLabel", selectgroupLabel );
	refXml->get_widget( "addmemberButton", addmemberButton );
	refXml->get_widget( "resetButton", resetButton );
       
	groupList=selectgroupList->gobj();
       

	addmemberButton->signal_clicked().connect( SLOT( *this, &MemberGroupDialog::addMember));
	resetButton->signal_clicked().connect( SLOT( *this, &MemberGroupDialog::reset));
        
	selectgroupList->signal_changed().connect(SLOT( *this, &MemberGroupDialog::groupListSelect));
	membergroupDialogWidget->hide();

}

MemberGroupDialog::~MemberGroupDialog(){
	delete membergroupDialogWidget;
}
void MemberGroupDialog::show(){
	membergroupDialogWidget->show();
     updateGroupList();
}
void MemberGroupDialog::hide(){
	membergroupDialogWidget->hide();
}
void MemberGroupDialog::addMember()
{
      nebula_addContact(nebulanameEntry->get_text(), contactnameEntry->get_text());
      group_details *grp,grp1;
      grp=&grp1;
      grp->groupId=a;
      cout<<grp->groupId<<endl;
      profile * user,user1;
      user=&user1;
      user->username=nebulanameEntry->get_text();
      cout<<user->username<<endl;
      nebula_insertUserIntoGroup(user,grp);
    
      
}
    
      
       

void MemberGroupDialog::editMemberGroup(){
	this->show();
	contactnameEntry->set_text("");
	//nebulanameEntry->set_text(nebulanameEntry->get_text());
}

void MemberGroupDialog::updateGroupList()
{ 
  int i;
  group_details *grp=extractProfileInfo();
/* for (i=0;grp[i].groupName!="";i++)
    gtk_combo_box_remove_text(groupList,i);   */
  for (i=0;grp[i].groupName !="";i++)
   {
    gtk_combo_box_append_text(groupList,grp[i].groupName.c_str());
}}

void MemberGroupDialog::groupListSelect()
{
int group_id,i;
 char * str=gtk_combo_box_get_active_text(groupList);
 group_details *grp=extractProfileInfo();
 for (i=0;grp[i].groupName!="";i++)
 {
  if(grp[i].groupName==str)
    group_id=grp[i].groupId;
  }
 a=group_id;
}

void MemberGroupDialog::reset()
{
contactnameEntry->set_text("");
nebulanameEntry->set_text("");
}


Glib::ustring MemberGroupDialog::getcontactNameString(){
	return contactnameEntry->get_text();
}
Glib::ustring MemberGroupDialog::getnebulaNameString(){
	return nebulanameEntry->get_text();
}
