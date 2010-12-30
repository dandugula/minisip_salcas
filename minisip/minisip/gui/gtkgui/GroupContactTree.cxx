#include"GroupContactTree.h"

void setup_tree_view (GtkTreeView *treeview)
{
  group_details *groups=extractProfileInfo();
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("Groups", renderer,"text",Groups,NULL);
  gtk_tree_view_append_column(treeview,column);
  GtkTreeStore *store;
  GtkTreeIter iter, child;
  guint i,j;
  store=gtk_tree_store_new(COLUMNS,G_TYPE_STRING);
  for(i=0;groups[i].groupName !="";i++)
  {
      gtk_tree_store_append (store, &iter, NULL);
      gtk_tree_store_set(store,&iter,Groups,groups[i].groupName.c_str(),-1);      
      j=0;
      while(j<groups[i].users)
       {
            gtk_tree_store_append (store, &child, &iter);
            gtk_tree_store_set(store,&child,Groups,groups[i].user_profile[j].username.c_str(),-1);      
	j++;	 
        }
                  
  }
    
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));
  gtk_tree_view_expand_all (treeview);
  gtk_widget_show(GTK_WIDGET(treeview));
  g_object_unref (store);   

}

