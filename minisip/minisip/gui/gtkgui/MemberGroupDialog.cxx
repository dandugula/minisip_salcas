#include<config.h>

#include"GroupDialog.h"
#include"MemberGroupDialog.h"
#include<libminisip/gui/Gui.h>

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
        refXml->get_widget("treeView1",GroupTreeView);
	
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
      group_details grp;
      nebula_insertUserIntoGroup(nebulanameEntry->get_text(),groupId);
      reset();
      hide();
      GroupTreeView->remove_all_columns();
      setup_tree_view(GroupTreeView->gobj());
      
}
          
void MemberGroupDialog::editMemberGroup()
{
	this->show();
	contactnameEntry->set_text("");
}
void MemberGroupDialog::removeMemberGroup()
{
  int i,j,userid=0,group_selected;
  group_details *grp=extractProfileInfo();
  Glib::RefPtr<Gtk::TreeSelection>selectedVal=GroupTreeView->get_selection();
  Gtk::TreeNodeChildren::iterator itc =selectedVal->get_selected();
  string str2;
  itc->get_value(0, str2);
  j=0;
  for(i=0;grp[i].groupName!="";i++)
  {
  while(j<grp[i].users)
  {
   if(grp[i].user_profile[j].username==str2)
   { 
      userid=grp[group_selected].user_profile[j].userId;
    }
  ++j;  
  }
  }
  nebula_deleteContact(userid);
  GroupTreeView->remove_all_columns();
  setup_tree_view(GroupTreeView->gobj());

}

void MemberGroupDialog::updateGroupList()
{ 
   GtkComboBox * groupList;
   group_details *groups=extractProfileInfo();
   GtkCellRenderer *renderer;
   GtkTreeViewColumn *column;
   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes("Groups", renderer,"text",Groups,NULL);
   GtkTreeStore *store;
   GtkTreeIter iter;
   guint i;
   store=gtk_tree_store_new(COLUMNS,G_TYPE_STRING);
   for(i=0;groups[i].groupName !="";i++)
   {
      gtk_tree_store_append (store, &iter, NULL);
      gtk_tree_store_set(store,&iter,Groups,groups[i].groupName.c_str(),-1); 
   }     
   gtk_combo_box_set_model(selectgroupList->gobj(),GTK_TREE_MODEL(store));
  
  
}


void MemberGroupDialog::groupListSelect()
{
 int group_id,i;
 string str;
 Gtk::TreeModel::iterator it = selectgroupList->get_active();
 it->get_value(0,str);
 group_details *grp=extractProfileInfo();
 for (i=0;grp[i].groupName!="";i++)
 {
  if(grp[i].groupName==str)
    group_id=grp[i].groupId;
 }
  groupId=group_id;
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
