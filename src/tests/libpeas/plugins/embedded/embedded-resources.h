#ifndef __RESOURCE_embedded_H__
#define __RESOURCE_embedded_H__

#include <gio/gio.h>

G_GNUC_INTERNAL GResource *embedded_get_resource (void);

G_GNUC_INTERNAL void embedded_register_resource (void);
G_GNUC_INTERNAL void embedded_unregister_resource (void);

#endif
