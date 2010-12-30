#include<config.h>

#include"GroupDialog.h"
#include<libminisip/gui/Gui.h>


#ifdef OLDLIBGLADEMM
#define SLOT(a,b) SigC::slot(a,b)
#define BIND SigC::bind
#else
#define SLOT(a,b) sigc::mem_fun(a,b)
#define BIND sigc::bind
#endif

using namespace std;

GroupDialog::GroupDialog( Glib::RefPtr<Gnome::Glade::Xml> theRefXml):
		refXml( theRefXml )
{
	refXml->get_widget( "addGroup", groupDialogWidget );

	/* Connect the Add and Back buttons */
	
	refXml->get_widget( "groupnameEntry", groupnameEntry );
	refXml->get_widget( "groupnameLabel", groupnameLabel );
	refXml->get_widget( "addgroupButton", addgroupButton );
	refXml->get_widget( "resetButton", resetButton );
        refXml->get_widget( "treeView1",GroupTreeView );

	addgroupButton->signal_clicked().connect( SLOT( *this, &GroupDialog::addGroup));
	resetButton->signal_clicked().connect( SLOT( *this, &GroupDialog::reset));

	groupDialogWidget->hide();
}

GroupDialog::~GroupDialog(){
	delete groupDialogWidget;
}
void GroupDialog::show(){
	groupDialogWidget->show();
}
void GroupDialog::hide(){
	groupDialogWidget->hide();
}
void GroupDialog::reset(){
	
}
void GroupDialog::removeGroup()
{
  int i,groupId;
  Glib::RefPtr<Gtk::TreeSelection>selectedVal=GroupTreeView->get_selection();
  Gtk::TreeModel::iterator it = selectedVal->get_selected();
  string str;
  it->get_value(0, str);
  group_details *grp=extractProfileInfo();
  for(i=0;grp[i].groupName!="";i++)
  {
    if(grp[i].groupName==str)
    { 
      groupId=grp[i].groupId;
    }
  }
  nebula_deleteGroup(groupId);
  GroupTreeView->remove_all_columns();
  setup_tree_view(GroupTreeView->gobj());
}
void GroupDialog::addGroup(){
		
	group_details *grp, grp1;
	grp=&grp1;
	
	grp->groupName=groupnameEntry->get_text();
	grp->status="Available";
        nebula_addGroup(grp);
        groupDialogWidget->hide();
        GroupTreeView->remove_all_columns();
        setup_tree_view(GroupTreeView->gobj());
      
}
