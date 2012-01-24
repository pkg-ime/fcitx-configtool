#include <gtk/gtk.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx-config/xdg.h>
#include <stdlib.h>
#include <libintl.h>
#include <errno.h>

#include "main_window.h"
#include "menu.h"
#include "config_widget.h"
#include "skin_stuff.h"
#include "addon_stuff.h"
#include "table_stuff.h"
#include "utarray.h"
#include "configdesc.h"

#define _(s) gettext(s)

static GtkWidget *mainWnd = NULL;
static GtkWidget *configTreeView = NULL;
static GtkWidget *configNotebook = NULL;
static GtkTreeStore *store = NULL;
static GtkWidget *hpaned = NULL;
static ConfigPage *configPage, *profilePage, *tablePage, *skinPage, *lastPage = NULL, *addonPage;

static int main_window_close(GtkWidget *theWindow, gpointer data);
static GtkTreeModel *fcitx_config_create_model();

int main_window_close(GtkWidget *theWindow, gpointer data)
{
    gtk_main_quit();
}

ConfigPage* main_window_add_page(char *cdesc, char* name, char *filename, ConfigFile *cfile, ConfigPage* parent, const char* domain, gboolean readonly)
{
    GtkTreeIter *p;
    if (parent)
        p = &parent->iter;
    else
        p = NULL;

    ConfigPage *page = (ConfigPage*) malloc(sizeof(ConfigPage));
    memset(page, 0, sizeof(ConfigPage));

    if (filename)
    {
        page->filename = strdup(filename);
        page->config.configFile = cfile;
        page->cfdesc = get_config_desc(cdesc);
        page->parent = parent;
        page->domain = domain;
        page->page = config_widget_new(page->cfdesc, cfile, page, readonly);
        ConfigBindSync(&page->config);
    }
    else
    {
        page->parent = parent;
        page->cfdesc = NULL;
        GtkWidget *label = gtk_label_new(_(cdesc));
        page->page = label;
    }

    g_object_ref(page->page);

    gtk_tree_store_append(store, &page->iter, p);
    gtk_tree_store_set(store, &page->iter, 0, name, 1, page, -1);

    return page;
}

gboolean selection_changed(GtkTreeSelection *selection, gpointer data) {
    GtkTreeView *treeView = gtk_tree_selection_get_tree_view(selection);
    GtkTreeModel *model = gtk_tree_view_get_model(treeView);
    GtkTreeIter iter;
    ConfigPage *page;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        gtk_tree_model_get(model, &iter,
                1, &page,
                -1);

        if (lastPage)
            gtk_container_remove(GTK_CONTAINER(hpaned), lastPage->page);
        gtk_paned_add2(GTK_PANED(hpaned), page->page);
        gtk_widget_show_all(mainWnd);

        lastPage = page;
    }
    else
    {
        gtk_tree_selection_select_iter(selection, &configPage->iter);
    }
}

GtkTreeModel *fcitx_config_create_model()
{
    store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
    return GTK_TREE_MODEL(store);
}

void add_config_file_page()
{
    FILE *fp;
    char *file;
    ConfigFileDesc* configDesc = get_config_desc("config.desc");
reload_config:
    fp = GetXDGFileUser( "config", "rt", &file);
    if (!fp) {
        if (errno == ENOENT)
        {
            fp = GetXDGFileUser("config", "wt", NULL);
            SaveConfigFileFp(fp, NULL, configDesc);
            fclose(fp);
            fp = NULL;
            goto reload_config;
        }
    }
    ConfigFile *cfile = ParseConfigFileFp(fp, configDesc);
    fclose(fp);

    configPage = main_window_add_page("config.desc", _("Config"), file, cfile, NULL, "fcitx", FALSE);
}

void add_profile_file_page()
{
    FILE *fp;
    char *file;
    ConfigFileDesc* configDesc = get_config_desc("profile.desc");
reload_profile:
    fp = GetXDGFileUser( "profile", "rt", &file);
    if (!fp) {
        if (errno == ENOENT)
        {
            fp = GetXDGFileUser("profile", "wt", NULL);
            SaveConfigFileFp(fp, NULL, configDesc);
            fclose(fp);
            fp = NULL;
            goto reload_profile;
        }
    }
    ConfigFile *cfile = ParseConfigFileFp(fp, configDesc);
    fclose(fp);

    profilePage = main_window_add_page("profile.desc", _("Profile"), file, cfile, NULL, "fcitx", FALSE);

}

void add_skin_page()
{
    skinPage = main_window_add_page(_("Skin Configuration"), _("Skin"), NULL, NULL, NULL, NULL, FALSE);
    UT_array *skinBuf = loadSkinDir();
    int j = 0;
    char buf[PATH_MAX];
    for(j=0;j<skinBuf->i;j++)
    {
        char **sskin = (char**)utarray_eltptr(skinBuf, j);
        snprintf(buf, PATH_MAX, "%s/fcitx_skin.conf", *sskin);
        buf[PATH_MAX-1] ='\0';
        size_t len;
        char ** path = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", "fcitx/skin" , DATADIR, "fcitx/skin" );
        char *file;
        ConfigFile *cfile;

        FILE* fp = GetXDGFile(buf, path, "r", len, NULL);

        FreeXDGPath(path);
        
        if (fp)
        {
            ConfigFileDesc* skinDesc = get_config_desc("skin.desc");
            cfile = ParseConfigFileFp(fp, skinDesc);
            if (!cfile)
            {
                fclose(fp);
                continue;
            }
            path = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", "fcitx/skin" , NULL, NULL );
            FILE * fp = GetXDGFile(buf, path, NULL, len, &file);
            if (fp) fclose(fp);
            FreeXDGPath(path);
        }
        else
            continue;
        
        main_window_add_page("skin.desc", *sskin, file, cfile, skinPage, "fcitx", FALSE);
        free(file);
    }

}

void add_addon_page()
{
    int i, j;
    addonPage = main_window_add_page(_("Addon Configuration"), _("Addon"), NULL, NULL, NULL, NULL, FALSE);
    UT_array *addonBuf = LoadAddonInfo();
    
    size_t len;
    char **addonPath = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", "fcitx/addon" , DATADIR, "fcitx/data/addon" );
    char **paths = malloc(sizeof(char*) *len);
    for (i = 0;i < len ;i ++)
        paths[i] = malloc(sizeof(char) *PATH_MAX);

    for(j=0;j<addonBuf->i;j++)
    {
        char *file;
        ConfigFile *cfile;
        char **saddon = (char**)utarray_eltptr(addonBuf, j);
        for (i = len -1; i >= 0; i--)
            snprintf(paths[i], PATH_MAX, "%s/%s", addonPath[len - i - 1], *saddon);

        cfile = ParseMultiConfigFile(paths, len, get_config_desc("addon.desc"));

        if (!cfile)
            continue;

        {
            size_t l;
            char **path = GetXDGPath(&l, "XDG_CONFIG_HOME", ".config", "fcitx/addon" , DATADIR, "fcitx/data/addon" );
            FILE *fp =GetXDGFile(*saddon, path, "r", l, &file);
            if (fp) fclose(fp);
            FreeXDGPath(path);
        }
        ConfigPage *page = main_window_add_page("addon.desc", *saddon, file, cfile, addonPage, "fcitx", TRUE);
        {
            /* add addon config page */
            size_t len = strlen(*saddon) - strlen(".conf");
            char *name = malloc((len + 1) * sizeof(char));
            char *descfilename = malloc((1 + len + strlen("addon/.desc")) * sizeof(char));
            char *filename = malloc((1 + len + strlen("addon/.config")) * sizeof(char));
            char *rfile = NULL;
            FILE *fp = NULL;
            gboolean reload = FALSE;
            strncpy(name ,*saddon ,len);
            name[len] = '\0';
            sprintf(descfilename, "addon/%s.desc", name);
            sprintf(filename, "addon/%s.config", name);
            ConfigFileDesc* addonConfigDesc = get_config_desc(descfilename);
reload_config:
            fp = GetXDGFileUser(filename, "rt", &rfile);
            if (!fp && !reload) {
                if (errno == ENOENT)
                {
                    fp = GetXDGFileUser(filename, "wt", NULL);
                    SaveConfigFileFp(fp, NULL, addonConfigDesc);
                    fclose(fp);
                    fp = NULL;
                    reload = TRUE;
                }
            }
            if (fp)
            {
                ConfigFile *addoncfile = ParseConfigFileFp(fp, addonConfigDesc);
                bindtextdomain(name, LOCALEDIR);
                main_window_add_page(descfilename, _("Configure"), rfile, addoncfile, page, strdup(name), FALSE);
            }
            free(name);
            free(descfilename);
            free(filename);
        }
        free(file);
    }

    for (i = 0;i < len ;i ++)
        free(paths[i]);
    free(paths);
    FreeXDGPath(addonPath);

}

void add_table_page()
{
    int i, j;
    tablePage = main_window_add_page(_("Table Configuration"), _("Table"), NULL, NULL, NULL, NULL, FALSE);
    UT_array *tableBuf = LoadTableInfo();
    
    size_t len;
    char **tablePath = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", "fcitx/table" , DATADIR, "fcitx/data/table" );
    char **paths = malloc(sizeof(char*) *len);
    for (i = 0;i < len ;i ++)
        paths[i] = malloc(sizeof(char) *PATH_MAX);

    for(j=0;j<tableBuf->i;j++)
    {
        char *file;
        ConfigFile *cfile;
        char **stable = (char**)utarray_eltptr(tableBuf, j);
        for (i = len -1; i >= 0; i--)
            snprintf(paths[i], PATH_MAX, "%s/%s", tablePath[len - i - 1], *stable);

        cfile = ParseMultiConfigFile(paths, len, get_config_desc("table.desc"));

        if (!cfile)
            continue;
        
        FILE *fp = GetXDGFileTable(*stable, "r", &file, True);
        if (fp) fclose(fp);
        main_window_add_page("table.desc", *stable, file, cfile, tablePage, "fcitx", FALSE);
        free(file);
    }

    for (i = 0;i < len ;i ++)
        free(paths[i]);
    free(paths);
    FreeXDGPath(tablePath);

}

GtkWidget* fcitx_config_main_window_new()
{
    if (mainWnd != NULL)
        return mainWnd;

    GtkWidget *menu = fcitx_config_menu_new();
    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    mainWnd = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    configTreeView = gtk_tree_view_new_with_model(fcitx_config_create_model());

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
            _("Config"), renderer,
            "text", 0,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW (configTreeView), column);

    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(configTreeView), FALSE);

    add_config_file_page();
    add_profile_file_page();
    add_table_page();
    add_skin_page();
    add_addon_page();

    gtk_widget_set_size_request(configTreeView, 170, -1);
    gtk_widget_set_size_request(mainWnd, -1, 660);

    hpaned = gtk_hpaned_new();
    GtkWidget *treescroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(treescroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_container_add(GTK_CONTAINER(treescroll), configTreeView);
    gtk_paned_add1(GTK_PANED(hpaned), treescroll);

    gtk_box_pack_start(GTK_BOX(vbox), menu, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(mainWnd), vbox);

    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(configTreeView));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

    gtk_signal_connect(GTK_OBJECT(mainWnd), "destroy", GTK_SIGNAL_FUNC(main_window_close), NULL);
    g_signal_connect(G_OBJECT(selection), "changed",
            G_CALLBACK(selection_changed), NULL);

    gtk_tree_view_expand_all(GTK_TREE_VIEW(configTreeView));
    gtk_tree_selection_select_iter(selection, &configPage->iter);

    gtk_window_set_icon_name(GTK_WINDOW(mainWnd), "fcitx-configtool");
    gtk_window_set_title(GTK_WINDOW(mainWnd), _("Fcitx Config"));
    return mainWnd;
}
