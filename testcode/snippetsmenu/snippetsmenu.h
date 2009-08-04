

/*****************************************************************/
/* stuff for the widget */
/*****************************************************************/

#define SNIPPETS_TYPE_MENU            (snippets_menu_get_type())
#define SNIPPETS_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), SNIPPETS_TYPE_MENU, SnippetsMenu))
#define SNIPPETS_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), SNIPPETS_TYPE_MENU, SnippetsMenuClass))
#define SNIPPETS_IS_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), SNIPPETS_TYPE_MENU))
#define SNIPPETS_IS_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), SNIPPETS_TYPE_MENU))
#define SNIPPETS_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SNIPPETS_TYPE_MENU, SnippetsMenuClass))

typedef struct _SnippetsMenu SnippetsMenu;
typedef struct _SnippetsMenuClass SnippetsMenuClass;

struct _SnippetsMenu {
	GtkMenuBar parent;
	GHashTable *hasht;
};

struct _SnippetsMenuClass {
	GtkMenuClass parent_class;
};

GType snippets_menu_get_type(void);

GtkWidget *snippets_menu_new(void);
void snippets_menu_set_model(SnippetsMenu *sm, GtkTreeModel *model);

