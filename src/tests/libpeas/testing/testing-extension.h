/*
 * testing-extension.h
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

#ifndef __TESTING_EXTENSION_H__
#define __TESTING_EXTENSION_H__

#include <libpeas/peas-engine.h>

#include "testing.h"

G_BEGIN_DECLS

void testing_extension_basic      (const gchar *loader);
void testing_extension_callable   (const gchar *loader);
void testing_extension_properties (const gchar *loader);
void testing_extension_add        (const gchar *path,
                                   gpointer     func);

int testing_extension_run_tests   (void);

#define testing_extension_all(loader) \
  testing_extension_basic (loader); \
  testing_extension_callable (loader); \
  testing_extension_properties (loader)

/* This macro is there to add loader-specific tests. */
#define EXTENSION_TEST(loader, path, func) \
  testing_extension_add ("/extension/" #loader "/" path, \
                         (gpointer) test_extension_##loader##_##func)

G_END_DECLS

#endif /* __TESTING__EXTENSION_H__ */
