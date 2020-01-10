/*
 * peas-plugin-loader-python.c
 * This file is part of libpeas
 *
 * Copyright (C) 2008 - Jesse van den Kieboom
 * Copyright (C) 2009 - Steve Frécinaux
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

#include "peas-plugin-loader-python.h"

/* _POSIX_C_SOURCE is defined in Python.h and in limits.h included by
 * glib-object.h, so we unset it here to avoid a warning. Yep, that's bad.
 */
#undef _POSIX_C_SOURCE
#include <pygobject.h>
#include <Python.h>
#include <signal.h>

#if PY_VERSION_HEX < 0x02050000
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif

struct _PeasPluginLoaderPythonPrivate {
  GHashTable *loaded_plugins;
  guint idle_gc;
  guint init_failed : 1;
  guint must_finalize_python : 1;
  PyThreadState *py_thread_state;
  PyObject *hooks;
};

typedef struct {
  PyObject *module;
} PythonInfo;

static gboolean   peas_plugin_loader_python_add_module_path (PeasPluginLoaderPython *pyloader,
                                                             const gchar            *module_path);

G_DEFINE_TYPE (PeasPluginLoaderPython, peas_plugin_loader_python, PEAS_TYPE_PLUGIN_LOADER)

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
  peas_object_module_register_extension_type (module,
                                              PEAS_TYPE_PLUGIN_LOADER,
                                              PEAS_TYPE_PLUGIN_LOADER_PYTHON);
}

/* NOTE: This must not be called with the GIL held */
static void
internal_python_hook (PeasPluginLoaderPython *pyloader,
                      const gchar            *name)
{
  PyGILState_STATE state = PyGILState_Ensure ();
  PyObject *result;

  result = PyObject_CallMethod (pyloader->priv->hooks, (gchar *) name, NULL);
  Py_XDECREF (result);

  if (PyErr_Occurred ())
    PyErr_Print ();

  PyGILState_Release (state);
}

/* NOTE: This must be called with the GIL held */
static PyTypeObject *
find_python_extension_type (PeasPluginInfo *info,
                            GType           exten_type,
                            PyObject       *pymodule)
{
  PyObject *pygtype, *pytype;
  PyObject *locals, *key, *value;
  Py_ssize_t pos = 0;

  locals = PyModule_GetDict (pymodule);

  pygtype = pyg_type_wrapper_new (exten_type);
  pytype = PyObject_GetAttrString (pygtype, "pytype");
  g_warn_if_fail (pytype != NULL);

  if (pytype != NULL && pytype != Py_None)
    {
      while (PyDict_Next (locals, &pos, &key, &value))
        {
          if (!PyType_Check (value))
            continue;

          switch (PyObject_IsSubclass (value, pytype))
            {
            case 1:
              Py_DECREF (pytype);
              Py_DECREF (pygtype);
              return (PyTypeObject *) value;
            case 0:
              continue;
            case -1:
            default:
              PyErr_Print ();
              continue;
            }
        }
    }

  Py_DECREF (pytype);
  Py_DECREF (pygtype);

  return NULL;
}

static gboolean
peas_plugin_loader_python_provides_extension (PeasPluginLoader *loader,
                                              PeasPluginInfo   *info,
                                              GType             exten_type)
{
  PeasPluginLoaderPython *pyloader = PEAS_PLUGIN_LOADER_PYTHON (loader);
  PythonInfo *pyinfo;
  PyTypeObject *extension_type;
  PyGILState_STATE state;

  pyinfo = (PythonInfo *) g_hash_table_lookup (pyloader->priv->loaded_plugins, info);

  state = PyGILState_Ensure ();
  extension_type = find_python_extension_type (info, exten_type, pyinfo->module);
  PyGILState_Release (state);

  return extension_type != NULL;
}

static PeasExtension *
peas_plugin_loader_python_create_extension (PeasPluginLoader *loader,
                                            PeasPluginInfo   *info,
                                            GType             exten_type,
                                            guint             n_parameters,
                                            GParameter       *parameters)
{
  PeasPluginLoaderPython *pyloader = PEAS_PLUGIN_LOADER_PYTHON (loader);
  PythonInfo *pyinfo;
  PyTypeObject *pytype;
  GType the_type;
  GObject *object = NULL;
  PyObject *pyobject;
  PyObject *pyplinfo;
  PyGILState_STATE state;

  pyinfo = (PythonInfo *) g_hash_table_lookup (pyloader->priv->loaded_plugins, info);

  state = PyGILState_Ensure ();

  pytype = find_python_extension_type (info, exten_type, pyinfo->module);

  if (pytype == NULL)
    goto out;

  the_type = pyg_type_from_object ((PyObject *) pytype);

  if (the_type == G_TYPE_INVALID)
    goto out;

  if (!g_type_is_a (the_type, exten_type))
    {
      g_warn_if_fail (g_type_is_a (the_type, exten_type));
      goto out;
    }

  object = g_object_newv (the_type, n_parameters, parameters);

  if (!object)
    goto out;

  /* As we do not instantiate a PeasExtensionWrapper, we have to remember
   * somehow which interface we are instantiating, to make it possible to use
   * the deprecated peas_extension_get_extension_type() method.
   */
  g_object_set_data (object, "peas-extension-type",
                     GUINT_TO_POINTER (exten_type));

  pyobject = pygobject_new (object);
  pyplinfo = pyg_boxed_new (PEAS_TYPE_PLUGIN_INFO, info, TRUE, TRUE);

  /* Set the plugin info as an attribute of the instance */
  if (PyObject_SetAttrString (pyobject, "plugin_info", pyplinfo) == -1)
    {
      g_warning ("Failed to set 'plugin_info' for '%s'",
                 g_type_name (the_type));

      if (PyErr_Occurred ())
        PyErr_Print ();

      g_clear_object (&object);
    }

  Py_DECREF (pyplinfo);
  Py_DECREF (pyobject);

out:

  PyGILState_Release (state);

  return object;
}

/* NOTE: This must be called with the GIL held */
static void
add_python_info (PeasPluginLoaderPython *loader,
                 PeasPluginInfo         *info,
                 PyObject               *module)
{
  PythonInfo *pyinfo;

  pyinfo = g_new (PythonInfo, 1);
  pyinfo->module = module;
  Py_INCREF (pyinfo->module);

  g_hash_table_insert (loader->priv->loaded_plugins, info, pyinfo);
}

static gboolean
peas_plugin_loader_python_load (PeasPluginLoader *loader,
                                PeasPluginInfo   *info)
{
  PeasPluginLoaderPython *pyloader = PEAS_PLUGIN_LOADER_PYTHON (loader);
  const gchar *module_dir, *module_name;
  PyObject *pymodule, *fromlist;
  PyGILState_STATE state;

  /* See if python definition for the plugin is already loaded */
  if (g_hash_table_lookup (pyloader->priv->loaded_plugins, info))
    return TRUE;

  module_dir = peas_plugin_info_get_module_dir (info);
  module_name = peas_plugin_info_get_module_name (info);

  state = PyGILState_Ensure ();

  /* If we have a special path, we register it */
  if (!peas_plugin_loader_python_add_module_path (pyloader, module_dir))
    {
      g_warning ("Error loading plugin '%s': failed to add module path '%s'",
                 module_name, module_dir);

      if (PyErr_Occurred ())
        PyErr_Print ();

      PyGILState_Release (state);

      return FALSE;
    }

  /* We need a fromlist to be able to import modules with a '.' in the name */
  fromlist = PyTuple_New (0);

  pymodule = PyImport_ImportModuleEx ((gchar *) module_name,
                                      NULL, NULL, fromlist);
  Py_DECREF (fromlist);

  if (!pymodule)
    {
      PyErr_Print ();
      PyGILState_Release (state);

      return FALSE;
    }

  add_python_info (pyloader, info, pymodule);
  Py_DECREF (pymodule);

  PyGILState_Release (state);

  return TRUE;
}

static void
peas_plugin_loader_python_unload (PeasPluginLoader *loader,
                                  PeasPluginInfo   *info)
{
  PeasPluginLoaderPython *pyloader = PEAS_PLUGIN_LOADER_PYTHON (loader);

  g_hash_table_remove (pyloader->priv->loaded_plugins, info);

  /* We have to use this as a hook as the
   * loader will not be finalized by applications
   */
  if (g_hash_table_size (pyloader->priv->loaded_plugins) == 0)
    internal_python_hook (pyloader, "all_plugins_unloaded");
}

static void
run_gc_protected (void)
{
  PyGILState_STATE state = PyGILState_Ensure ();

  while (PyGC_Collect ())
    ;

  PyGILState_Release (state);
}

static gboolean
run_gc (PeasPluginLoaderPython *loader)
{
  run_gc_protected ();

  loader->priv->idle_gc = 0;
  return FALSE;
}

static void
peas_plugin_loader_python_garbage_collect (PeasPluginLoader *loader)
{
  PeasPluginLoaderPython *pyloader = PEAS_PLUGIN_LOADER_PYTHON (loader);

  /* We both run the GC right now and we schedule
   * a further collection in the main loop.
   */
  run_gc_protected ();

  if (pyloader->priv->idle_gc == 0)
    {
      pyloader->priv->idle_gc = g_idle_add ((GSourceFunc) run_gc, pyloader);
      g_source_set_name_by_id (pyloader->priv->idle_gc, "[libpeas] run_gc");
    }
}

/* C equivalent of
 *    import sys
 *    sys.path.insert(0, module_path)
 */
/* NOTE: This must be called with the GIL held */
static gboolean
peas_plugin_loader_python_add_module_path (PeasPluginLoaderPython *pyloader,
                                           const gchar            *module_path)
{
  PyObject *pathlist, *pathstring;
  gboolean success = TRUE;

  g_return_val_if_fail (PEAS_IS_PLUGIN_LOADER_PYTHON (pyloader), FALSE);
  g_return_val_if_fail (module_path != NULL, FALSE);

  pathlist = PySys_GetObject ((char *) "path");
  if (pathlist == NULL)
    return FALSE;

#if PY_VERSION_HEX < 0x03000000
  pathstring = PyString_FromString (module_path);
#else
  pathstring = PyUnicode_FromString (module_path);
#endif

  if (pathstring == NULL)
    return FALSE;

  switch (PySequence_Contains (pathlist, pathstring))
    {
    case 0:
      success = PyList_Insert (pathlist, 0, pathstring) >= 0;
      break;
    case 1:
      break;
    case -1:
    default:
      success = FALSE;
      break;
    }

  Py_DECREF (pathstring);
  return success;
}

#if PY_VERSION_HEX >= 0x03000000
static wchar_t *
peas_wchar_from_str (const gchar *str)
{
  wchar_t *outbuf;
  gsize argsize, count;

  argsize = mbstowcs (NULL, str, 0);
  if (argsize == (gsize)-1)
    {
      g_warning ("Could not convert argument to wchar_t string.");
      return NULL;
    }

  outbuf = g_new (wchar_t, argsize + 1);
  count = mbstowcs (outbuf, str, argsize + 1);
  if (count == (gsize)-1)
    {
      g_warning ("Could not convert argument to wchar_t string.");
      return NULL;
    }

  return outbuf;
}
#endif

#ifdef HAVE_SIGACTION
static void
default_sigint (int sig)
{
  struct sigaction sigint;

  /* Invoke default sigint handler */
  sigint.sa_handler = SIG_DFL;
  sigint.sa_flags = 0;
  sigemptyset (&sigint.sa_mask);

  sigaction (SIGINT, &sigint, NULL);

  raise (SIGINT);
}
#endif

static gboolean
peas_plugin_loader_python_initialize (PeasPluginLoader *loader)
{
  PeasPluginLoaderPython *pyloader = PEAS_PLUGIN_LOADER_PYTHON (loader);
  PyGILState_STATE state = 0;
  long hexversion;
  GBytes *internal_python;
  PyObject *gettext, *result, *builtins_module, *code, *globals;
  const gchar *prgname;
#if PY_VERSION_HEX < 0x03000000
  const char *argv[] = { NULL, NULL };
#else
  wchar_t *argv[] = { NULL, NULL };
#endif

  /* We are trying to initialize Python for the first time,
     set init_failed to FALSE only if the entire initialization process
     ends with success */
  pyloader->priv->init_failed = TRUE;

  /* Python initialization */
  if (Py_IsInitialized ())
    {
      state = PyGILState_Ensure ();
    }
  else
    {
#ifdef HAVE_SIGACTION
      struct sigaction sigint;

      /* We are going to install a signal handler for SIGINT if the current
         signal handler for sigint is SIG_DFL. We do this because even if
         Py_InitializeEx will not set the signal handlers, the 'signal' module
         (which can be used by plugins for various reasons) will install a
         SIGINT handler when imported, if SIGINT is set to SIG_DFL. Our
         override will simply call the default SIGINT handler in the end. */
      sigaction (SIGINT, NULL, &sigint);

      if (sigint.sa_handler == SIG_DFL)
        {
          sigemptyset (&sigint.sa_mask);
          sigint.sa_flags = 0;
          sigint.sa_handler = default_sigint;

          sigaction (SIGINT, &sigint, NULL);
        }
#endif

      Py_InitializeEx (FALSE);
      pyloader->priv->must_finalize_python = TRUE;
    }

  hexversion = PyLong_AsLong (PySys_GetObject ((char *) "hexversion"));

#if PY_VERSION_HEX < 0x03000000
  if (hexversion >= 0x03000000)
#else
  if (hexversion < 0x03000000)
#endif
    {
      g_critical ("Attempting to mix incompatible Python versions");

      goto python_init_error;
    }

  prgname = g_get_prgname ();
  prgname = prgname == NULL ? "" : prgname;

#if PY_VERSION_HEX < 0x03000000
  argv[0] = prgname;
#else
  argv[0] = peas_wchar_from_str (prgname);
#endif

  /* See http://docs.python.org/c-api/init.html#PySys_SetArgvEx */
#if PY_VERSION_HEX < 0x02060600
  PySys_SetArgv (1, (char**) argv);
  PyRun_SimpleString ("import sys; sys.path.pop(0)\n");
#elif PY_VERSION_HEX < 0x03000000
  PySys_SetArgvEx (1, (char**) argv, 0);
#elif PY_VERSION_HEX < 0x03010300
  PySys_SetArgv (1, argv);
  PyRun_SimpleString ("import sys; sys.path.pop(0)\n");
  g_free (argv[0]);
#else
  PySys_SetArgvEx (1, argv, 0);
  g_free (argv[0]);
#endif

  if (!peas_plugin_loader_python_add_module_path (pyloader, PEAS_PYEXECDIR))
    {
      g_warning ("Error initializing Python Plugin Loader: "
                 "failed to add the module path");

      goto python_init_error;
    }

  /* Initialize PyGObject */
  pygobject_init (PYGOBJECT_MAJOR_VERSION,
                  PYGOBJECT_MINOR_VERSION,
                  PYGOBJECT_MICRO_VERSION);

  if (PyErr_Occurred ())
    {
      g_warning ("Error initializing Python Plugin Loader: "
                 "PyGObject initialization failed");

      goto python_init_error;
    }

  /* Initialize support for threads */
  pyg_enable_threads ();
  PyEval_InitThreads ();

  /* Only redirect warnings when python was not already initialized */
  if (!pyloader->priv->must_finalize_python)
    pyg_disable_warning_redirections ();

  /* i18n support */
  gettext = PyImport_ImportModule ("gettext");
  if (gettext == NULL)
    {
      g_warning ("Error initializing Python Plugin Loader: "
                 "failed to import gettext");

      goto python_init_error;
    }

  result = PyObject_CallMethod (gettext, "install", "ss",
                                GETTEXT_PACKAGE, PEAS_LOCALEDIR);
  Py_XDECREF (result);

  if (PyErr_Occurred ())
    {
      g_warning ("Error initializing Python Plugin Loader: "
                 "failed to install gettext");

      goto python_init_error;
    }

#if PY_VERSION_HEX < 0x03000000
  builtins_module = PyImport_ImportModule ("__builtin__");
#else
  builtins_module = PyImport_ImportModule ("builtins");
#endif

  if (builtins_module == NULL)
    goto python_init_error;

  internal_python = g_resources_lookup_data ("/org/gnome/libpeas/loaders/"
#if PY_VERSION_HEX < 0x03000000
                                             "python/"
#else
                                             "python3/"
#endif
                                             "internal.py",
                                             G_RESOURCE_LOOKUP_FLAGS_NONE,
                                             NULL);

  if (internal_python == NULL)
    {
      g_warning ("Error initializing Python Plugin Loader: "
                 "failed to locate internal python code");

      goto python_init_error;
    }

  /* Compile it manually so the filename is available */
  code = Py_CompileString (g_bytes_get_data (internal_python, NULL),
                           "peas-plugin-loader-python-internal.py",
                           Py_file_input);

  g_bytes_unref (internal_python);

  if (code == NULL)
    {
      g_warning ("Error initializing Python Plugin Loader: "
                 "failed to compile internal python code");

      goto python_init_error;
    }

  globals = PyDict_New ();
  if (globals == NULL)
    {
      Py_DECREF (code);
      goto python_init_error;
    }

  if (PyDict_SetItemString (globals, "__builtins__",
                            PyModule_GetDict (builtins_module)) != 0)
    {
      Py_DECREF (globals);
      Py_DECREF (code);
      goto python_init_error;
    }

  result = PyEval_EvalCode ((gpointer) code, globals, globals);
  Py_XDECREF (result);

  Py_DECREF (code);

  if (PyErr_Occurred ())
    {
      g_warning ("Error initializing Python Plugin Loader: "
                 "failed to run internal python code");

      Py_DECREF (globals);
      goto python_init_error;
    }

  pyloader->priv->hooks = PyDict_GetItemString (globals, "hooks");
  Py_XINCREF (pyloader->priv->hooks);

  Py_DECREF (globals);

  if (pyloader->priv->hooks == NULL)
    {
      g_warning ("Error initializing Python Plugin Loader: "
                 "failed to find internal python hooks");

      goto python_init_error;
    }

  /* Python has been successfully initialized */
  pyloader->priv->init_failed = FALSE;

  if (!pyloader->priv->must_finalize_python)
    PyGILState_Release (state);
  else
    pyloader->priv->py_thread_state = PyEval_SaveThread ();

  return TRUE;

python_init_error:

  if (PyErr_Occurred ())
    PyErr_Print ();

  g_warning ("Please check the installation of all the Python "
             "related packages required by libpeas and try again");

  if (!pyloader->priv->must_finalize_python)
    PyGILState_Release (state);

  return FALSE;
}

static void
destroy_python_info (PythonInfo *info)
{
  PyGILState_STATE state = PyGILState_Ensure ();

  Py_DECREF (info->module);

  PyGILState_Release (state);

  g_free (info);
}

static void
peas_plugin_loader_python_init (PeasPluginLoaderPython *pyloader)
{
  pyloader->priv = G_TYPE_INSTANCE_GET_PRIVATE (pyloader,
                                                PEAS_TYPE_PLUGIN_LOADER_PYTHON,
                                                PeasPluginLoaderPythonPrivate);

  /* loaded_plugins maps PeasPluginInfo to a PythonInfo */
  pyloader->priv->loaded_plugins = g_hash_table_new_full (g_direct_hash,
                                                          g_direct_equal,
                                                          NULL,
                                                          (GDestroyNotify) destroy_python_info);
}

static void
peas_plugin_loader_python_finalize (GObject *object)
{
  PeasPluginLoaderPython *pyloader = PEAS_PLUGIN_LOADER_PYTHON (object);

  g_hash_table_destroy (pyloader->priv->loaded_plugins);

  if (Py_IsInitialized ())
    {
      if (pyloader->priv->hooks != NULL)
        {
          internal_python_hook (pyloader, "exit");

          /* Borrowed Reference */
          pyloader->priv->hooks = NULL;
        }

      if (pyloader->priv->py_thread_state)
        {
          PyEval_RestoreThread (pyloader->priv->py_thread_state);
          pyloader->priv->py_thread_state = NULL;
        }

      if (pyloader->priv->idle_gc != 0)
        {
          g_source_remove (pyloader->priv->idle_gc);
          pyloader->priv->idle_gc = 0;
        }

      if (!pyloader->priv->init_failed)
        run_gc_protected ();

      if (pyloader->priv->must_finalize_python)
        {
          if (!pyloader->priv->init_failed)
            PyGILState_Ensure ();

          Py_Finalize ();
        }
    }

  G_OBJECT_CLASS (peas_plugin_loader_python_parent_class)->finalize (object);
}

static void
peas_plugin_loader_python_class_init (PeasPluginLoaderPythonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PeasPluginLoaderClass *loader_class = PEAS_PLUGIN_LOADER_CLASS (klass);

  object_class->finalize = peas_plugin_loader_python_finalize;

  loader_class->initialize = peas_plugin_loader_python_initialize;
  loader_class->load = peas_plugin_loader_python_load;
  loader_class->unload = peas_plugin_loader_python_unload;
  loader_class->create_extension = peas_plugin_loader_python_create_extension;
  loader_class->provides_extension = peas_plugin_loader_python_provides_extension;
  loader_class->garbage_collect = peas_plugin_loader_python_garbage_collect;

  g_type_class_add_private (object_class, sizeof (PeasPluginLoaderPythonPrivate));
}
