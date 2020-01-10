/*
 * extension-c-plugin.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>

#include <libpeas/peas.h>

#include "introspection-base.h"
#include "introspection-callable.h"
#include "introspection-has-prerequisite.h"

#include "extension-c-plugin.h"

static void introspection_base_iface_init (IntrospectionBaseInterface *iface);
static void introspection_extension_c_iface_init (IntrospectionCallableInterface *iface);
static void introspection_has_prerequisite_iface_init (IntrospectionHasPrerequisiteInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (TestingExtensionCPlugin,
                                testing_extension_c_plugin,
                                PEAS_TYPE_EXTENSION_BASE,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (INTROSPECTION_TYPE_BASE,
                                                               introspection_base_iface_init)
                                G_IMPLEMENT_INTERFACE_DYNAMIC (INTROSPECTION_TYPE_CALLABLE,
                                                               introspection_extension_c_iface_init)
                                G_IMPLEMENT_INTERFACE_DYNAMIC (INTROSPECTION_TYPE_HAS_PREREQUISITE,
                                                               introspection_has_prerequisite_iface_init))

static void
testing_extension_c_plugin_init (TestingExtensionCPlugin *plugin)
{
}

static const PeasPluginInfo *
testing_extension_c_plugin_get_plugin_info (IntrospectionBase *base)
{
  return peas_extension_base_get_plugin_info (PEAS_EXTENSION_BASE (base));
}

static GSettings *
testing_extension_c_plugin_get_settings (IntrospectionBase *base)
{
  PeasPluginInfo *info;

  info = peas_extension_base_get_plugin_info (PEAS_EXTENSION_BASE (base));

  return peas_plugin_info_get_settings (info, NULL);
}

static void
testing_extension_c_plugin_call_no_args (IntrospectionCallable *callable)
{
}

static const gchar *
testing_extension_c_plugin_call_with_return (IntrospectionCallable *callable)
{
  return "Hello, World!";
}

static void
testing_extension_c_plugin_call_single_arg (IntrospectionCallable *callable,
                                            gboolean              *called)
{
  *called = TRUE;
}

static void
testing_extension_c_plugin_call_multi_args (IntrospectionCallable *callable,
                                            gint                   in,
                                            gint                  *out,
                                            gint                  *inout)
{
  *out = *inout;
  *inout = in;
}

static void
testing_extension_c_plugin_class_init (TestingExtensionCPluginClass *klass)
{
}

static void
introspection_base_iface_init (IntrospectionBaseInterface *iface)
{
  iface->get_plugin_info = testing_extension_c_plugin_get_plugin_info;
  iface->get_settings = testing_extension_c_plugin_get_settings;
}

static void
introspection_extension_c_iface_init (IntrospectionCallableInterface *iface)
{
  iface->call_no_args = testing_extension_c_plugin_call_no_args;
  iface->call_with_return = testing_extension_c_plugin_call_with_return;
  iface->call_single_arg = testing_extension_c_plugin_call_single_arg;
  iface->call_multi_args = testing_extension_c_plugin_call_multi_args;
}

static void
introspection_has_prerequisite_iface_init (IntrospectionHasPrerequisiteInterface *iface)
{
}

static void
testing_extension_c_plugin_class_finalize (TestingExtensionCPluginClass *klass)
{
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
  testing_extension_c_plugin_register_type (G_TYPE_MODULE (module));

  peas_object_module_register_extension_type (module,
                                              INTROSPECTION_TYPE_BASE,
                                              TESTING_TYPE_EXTENSION_C_PLUGIN);
  peas_object_module_register_extension_type (module,
                                              INTROSPECTION_TYPE_CALLABLE,
                                              TESTING_TYPE_EXTENSION_C_PLUGIN);
  peas_object_module_register_extension_type (module,
                                              INTROSPECTION_TYPE_HAS_PREREQUISITE,
                                              TESTING_TYPE_EXTENSION_C_PLUGIN);
}
