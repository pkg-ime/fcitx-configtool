/***************************************************************************
 *   Copyright (C) 2010~2011 by CSSlayer                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/* fcitx-config-widget.h */

#ifndef _FCITX_CONFIG_WIDGET
#define _FCITX_CONFIG_WIDGET

#include <gtk/gtkwidget.h>
#include <glib/gstring.h>
#include <fcitx-config/fcitx-config.h>
#include <gtk/gtkvbox.h>
#include "sub_config_parser.h"

G_BEGIN_DECLS

#define FCITX_TYPE_CONFIG_WIDGET fcitx_config_widget_get_type()

#define FCITX_CONFIG_WIDGET(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), FCITX_TYPE_CONFIG_WIDGET, FcitxConfigWidget))

#define FCITX_CONFIG_WIDGET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), FCITX_TYPE_CONFIG_WIDGET, FcitxConfigWidgetClass))

#define FCITX_IS_CONFIG_WIDGET(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FCITX_TYPE_CONFIG_WIDGET))

#define FCITX_IS_CONFIG_WIDGET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), FCITX_TYPE_CONFIG_WIDGET))

#define FCITX_CONFIG_WIDGET_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), FCITX_TYPE_CONFIG_WIDGET, FcitxConfigWidgetClass))

typedef struct {
  GtkVBox parent;
  ConfigFileDesc* cfdesc;
  gchar* prefix;
  gchar* name;
  FcitxSubConfigParser* parser;
  GenericConfig gconfig;
} FcitxConfigWidget;

typedef struct {
  GtkVBoxClass parent_class;
} FcitxConfigWidgetClass;

typedef enum {
    CONFIG_WIDGET_SAVE,
    CONFIG_WIDGET_CANCEL,
    CONFIG_WIDGET_DEFAULT
} ConfigWidgetAction;

GType fcitx_config_widget_get_type (void);

FcitxConfigWidget* fcitx_config_widget_new (ConfigFileDesc* cfdesc, const gchar* prefix, const gchar* name, const char* subconfig);
void fcitx_config_widget_response(FcitxConfigWidget* config_widget, ConfigWidgetAction action);
G_END_DECLS

#endif /* _FCITX_CONFIG_WIDGET */
