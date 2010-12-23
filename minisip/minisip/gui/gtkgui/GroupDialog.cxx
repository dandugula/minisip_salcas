#include<config.h>

#include"GroupDialog.h"
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

GroupDialog::GroupDialog( Glib::RefPtr<Gnome::Glade::Xml> theRefXml):
		refXml( theRefXml )
{
	refXml->get_widget( "addGroup", groupDialogWidget );

	/* Connect the Add and Back buttons */
	
	refXml->get_widget( "groupnameEntry", groupnameEntry );
	refXml->get_widget( "groupnameLabel", groupnameLabel );
	refXml->get_widget( "addgroupButton", addgroupButton );
	refXml->get_widget( "resetButton", resetButton );

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
/*void GroupDialog::removeGroup(){
	groupDialogWidget->show();
}*/
void GroupDialog::addGroup(){
		
	group_details *grp, grp1;
	grp=&grp1;
	
	grp->groupName=groupnameEntry->get_text();
	grp->status="Available";


	nebula_addGroup(grp);
}
