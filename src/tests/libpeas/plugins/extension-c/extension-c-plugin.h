/*
 * extension-c-plugin.h
 * This file is part of libpeas
 *
 * Copyright (C) 2011 - Garrett Regier
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __EXTENSION_C_PLUGIN_H__
#define __EXTENSION_C_PLUGIN_H__

#include <libpeas/peas.h>

G_BEGIN_DECLS

#define TESTING_TYPE_EXTENSION_C_PLUGIN         (testing_extension_c_plugin_get_type ())
#define TESTING_EXTENSION_C_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TESTING_TYPE_EXTENSION_C_PLUGIN, TestingExtensionCPlugin))
#define TESTING_EXTENSION_C_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TESTING_TYPE_EXTENSION_C_PLUGIN, TestingExtensionCPlugin))
#define TESTING_IS_EXTENSION_C_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TESTING_TYPE_EXTENSION_C_PLUGIN))
#define TESTING_IS_EXTENSION_C_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TESTING_TYPE_EXTENSION_C_PLUGIN))
#define TESTING_EXTENSION_C_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TESTING_TYPE_EXTENSION_C_PLUGIN, TestingExtensionCPluginClass))

typedef struct _TestingExtensionCPlugin         TestingExtensionCPlugin;
typedef struct _TestingExtensionCPluginClass    TestingExtensionCPluginClass;
typedef struct _TestingExtensionCPluginPrivate  TestingExtensionCPluginPrivate;

struct _TestingExtensionCPlugin {
  PeasExtensionBase parent_instance;
};

struct _TestingExtensionCPluginClass {
  PeasExtensionBaseClass parent_class;
};

GType                 testing_extension_c_plugin_get_type (void) G_GNUC_CONST;
G_MODULE_EXPORT void  peas_register_types                 (PeasObjectModule *module);

G_END_DECLS

#endif /* __EXTENSION_C_PLUGIN_H__ */
