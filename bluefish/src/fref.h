/* ---------------------------------------------------------- */
/* -------------- XML FILE FORMAT --------------------------- */
/* ---------------------------------------------------------- */

/* 
<ref name="MySQL Functions" description="MySQL functions for PHP " case="1">

<function name="mysql_db_query">

   <description>
     Function description 
   </description>

   <tip>Text shown in a tooltip or hints</tip>
      
   <param name="database" title="Database name" required="1" vallist="0" default="" type="string" >
     <vallist>Values if vallist==1</vallist>
     Parameter description
   </param>       
    
   <return type="resource">
     Return value description
   </return>
  
   <dialog title="Dialog title">
      Text inserted after executing dialog, use params as %0,%1,%2 etc.
      %_ means "insert only these attributes which are not empty and not 
                default values" 
   </dialog>
   
   <insert>
      Text inserted after activating this action
   </insert>
   
   <info title="Title of the info window">    
     Text shown in the info
   </info>    
   
 </function>
 
 <tag name="Table Element">
 
   <description>
       The TABLE element contains all other elements that specify caption, rows, content, and formatting.
   </description>

   <tip>Tag tooltip</tip>
     
   <attribute name="Border" title="Table border" required="0" vallist="0" default="0">
      <vallist></vallist>
          This attribute specifies the width (in pixels only) of the frame around a table (see the Note below for more information about this attribute).
   </attribute>

   <dialog title="Dialog title">
      Text inserted after executing dialog, use params as %0,%1,%2 etc.
   </dialog>
   
   <insert>
      Text inserted after activating this action
   </insert>
   
   <info title="Title of the info window">    
     Text shown in the info
   </info>    
  
 </tag>
   

</ref>
*/

#ifndef __FREF_H__
#define __FREF_H__

enum {
 STR_COLUMN,
 PTR_COLUMN,
 FILE_COLUMN,
 N_COLUMNS
};


#define FR_TYPE_TAG							1
#define FR_TYPE_FUNCTION			2
#define FR_TYPE_CLASS					3

#define MAX_NEST_LEVEL		20

typedef struct
{
  gchar *name;
  gchar *title;
  gchar *description;
  gchar *def_value;
  gboolean required;
  gboolean has_list;
  gchar *values; 
  GtkWidget *dlg_item;
} FRAttrInfo;

typedef struct
{
  gchar *name;
  gchar *title;
  gchar *description;
  gchar *def_value;
  gchar *type;
  gboolean required;
  gboolean has_list;
  gchar *values;
  GtkWidget *dlg_item;
} FRParamInfo;


typedef struct
{
  gchar type;  
  gchar *name;
  gchar *description;
  gchar *tip;
  gchar *return_type; /* if function */
  gchar *return_description; /* if function */
  GList *attributes; 
  GList *params;     
  GList *methods;    
  gchar *info_text;
  gchar *info_title;
  gchar *dialog_text;
  gchar *dialog_title;
  gchar *insert_text;
} FRInfo;


typedef struct
{
  FRInfo *act_info;
  FRAttrInfo *act_attr;
  FRParamInfo *act_param;
  GtkWidget *tree;
  GtkTreeStore *store;
  GtkTreeIter grp_parent[MAX_NEST_LEVEL];
  gint nest_level;
  GtkTreeIter parent;
  gint state;
  gint pstate;
  gint vstate;
  GList *autoitems;  
} FRParseAux;

#define FR_LOADER_STATE_NONE            1
#define FR_LOADER_STATE_TAG             2
#define FR_LOADER_STATE_FUNC            3
#define FR_LOADER_STATE_CLASS           4
#define FR_LOADER_STATE_ATTR            5
#define FR_LOADER_STATE_PARAM           6
#define FR_LOADER_STATE_TIP             7
#define FR_LOADER_STATE_DESCR           8
#define FR_LOADER_STATE_INFO            9
#define FR_LOADER_STATE_DIALOG          10
#define FR_LOADER_STATE_INSERT          11
#define FR_LOADER_STATE_VALLIST         12
#define FR_LOADER_STATE_RETURN          13


/* CONFIG PARSER FUNCTIONS */

void fref_loader_start_element(GMarkupParseContext *context,const gchar *element_name,
                               const gchar **attribute_names,const gchar **attribute_values,
                               gpointer user_data,GError **error);
                              
void fref_loader_end_element(GMarkupParseContext *context,const gchar *element_name,
                             gpointer user_data,GError **error);        
                             
void fref_loader_text(GMarkupParseContext *context,const gchar *_text,gsize _text_len,  
                      gpointer user_data,GError **error); 
                      
void fref_loader_error(GMarkupParseContext *context,GError *error,gpointer user_data);
                          
void fref_loader_load_ref_xml(gchar *filename,GtkWidget *tree,GtkTreeStore *store,GtkTreeIter *parent);

void fref_loader_unload_ref(GtkWidget *tree,GtkTreeStore *store,GtkTreeIter *position);
void fref_loader_unload_all(GtkWidget *tree,GtkTreeStore *store);

void fref_free_info(FRInfo *info);

/* FR GUI */

GtkWidget    *fref_init();
void         fref_cleanup();

gchar        *fref_prepare_info(FRInfo *entry);
void         fref_show_info(gchar *txt, gboolean modal,GtkWidget *parent);
GList        *fref_string_to_list(gchar *string,gchar *delimiter);
gchar        *fref_prepare_text(FRInfo *entry,GtkWidget *dialog);
GtkWidget    *fref_prepare_dialog(FRInfo *entry);
void         fref_ac_position(GtkMenu *menu,gint *x,gint *y,gboolean *push_in,gpointer user_data);

/* CALLBACKS */

void        frefcb_row_collapsed(GtkTreeView *treeview,GtkTreeIter *arg1,GtkTreePath *arg2,gpointer user_data);
void        frefcb_row_expanded(GtkTreeView *treeview,GtkTreeIter *arg1,GtkTreePath *arg2,gpointer user_data);
gboolean    frefcb_event_mouseclick(GtkWidget *widget,GdkEventButton *event,gpointer user_data);
gboolean    frefcb_event_keypress(GtkWidget *widget,GdkEventKey *event,gpointer user_data);
gboolean    frefcb_info_lost_focus(GtkWidget *widget,GdkEventFocus *event,gpointer user_data);
gboolean    frefcb_info_keypress(GtkWidget *widget,GdkEventKey *event,gpointer user_data);
void        frefcb_info_close(GtkButton *button,gpointer user_data);
void        frefcb_info_dialog(GtkButton *button,gpointer user_data);
void        frefcb_info_insert(GtkButton *button,gpointer user_data);
void		     frefcb_autocomplete(GtkWidget *widget,gpointer data);
void        frefcb_autocomplete_activate(GtkMenuItem *menuitem,gpointer user_data);
void 	      frefcb_info_show(GtkButton *button,gpointer user_data); 

#endif /* __FREF_H__ */


