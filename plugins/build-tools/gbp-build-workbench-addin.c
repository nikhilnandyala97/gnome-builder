/* gbp-build-workbench-addin.c
 *
 * Copyright (C) 2015 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <egg-binding-group.h>
#include <glib/gi18n.h>

#include "gbp-build-log-panel.h"
#include "gbp-build-panel.h"
#include "gbp-build-perspective.h"
#include "gbp-build-workbench-addin.h"

struct _GbpBuildWorkbenchAddin
{
  GObject              parent_instance;

  /* Unowned */
  GbpBuildPanel       *panel;
  IdeWorkbench        *workbench;
  GbpBuildLogPanel    *build_log_panel;
  GtkWidget           *run_button;
  GbpBuildPerspective *build_perspective;

  /* Owned */
  IdeBuildResult      *result;
  GSimpleActionGroup  *actions;
};

static void workbench_addin_iface_init (IdeWorkbenchAddinInterface *iface);

G_DEFINE_TYPE_EXTENDED (GbpBuildWorkbenchAddin, gbp_build_workbench_addin, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (IDE_TYPE_WORKBENCH_ADDIN, workbench_addin_iface_init))

enum {
  PROP_0,
  PROP_RESULT,
  LAST_PROP
};

static GParamSpec *properties [LAST_PROP];

static void
gbp_build_workbench_addin_set_result (GbpBuildWorkbenchAddin *self,
                                      IdeBuildResult         *result)
{
  g_return_if_fail (GBP_IS_BUILD_WORKBENCH_ADDIN (self));
  g_return_if_fail (!result || IDE_IS_BUILD_RESULT (result));
  g_return_if_fail (self->workbench != NULL);

  if (g_set_object (&self->result, result))
    {
      gbp_build_log_panel_set_result (self->build_log_panel, result);
      gtk_widget_show (GTK_WIDGET (self->build_log_panel));
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_RESULT]);
    }
}

static void
gbp_build_workbench_addin_view_output (GSimpleAction *action,
                                       GVariant      *param,
                                       gpointer       user_data)
{
  GbpBuildWorkbenchAddin *self = user_data;

  g_assert (G_IS_SIMPLE_ACTION (action));
  g_assert (GBP_IS_BUILD_WORKBENCH_ADDIN (self));

  ide_workbench_focus (self->workbench, GTK_WIDGET (self->build_log_panel));
}

static void
gbp_build_workbench_addin_configure (GSimpleAction *action,
                                     GVariant      *param,
                                     gpointer       user_data)
{
  GbpBuildWorkbenchAddin *self = user_data;
  IdeConfigurationManager *config_manager;
  IdeConfiguration *config;
  IdeContext *context;
  const gchar *id;

  g_assert (GBP_IS_BUILD_WORKBENCH_ADDIN (self));
  g_assert (g_variant_is_of_type (param, G_VARIANT_TYPE_STRING));

  ide_workbench_set_visible_perspective (self->workbench,
                                         IDE_PERSPECTIVE (self->build_perspective));

  context = ide_workbench_get_context (self->workbench);
  config_manager = ide_context_get_configuration_manager (context);
  id = g_variant_get_string (param, NULL);
  config = ide_configuration_manager_get_configuration (config_manager, id);

  if (config != NULL)
    gbp_build_perspective_set_configuration (self->build_perspective, config);
}

static const GActionEntry actions_entries[] = {
  { "configure", gbp_build_workbench_addin_configure, "s" },
  { "view-output", gbp_build_workbench_addin_view_output },
};

static void
gbp_build_workbench_addin_load (IdeWorkbenchAddin *addin,
                                IdeWorkbench      *workbench)
{
  IdeConfigurationManager *configuration_manager;
  GbpBuildWorkbenchAddin *self = (GbpBuildWorkbenchAddin *)addin;
  IdeConfiguration *configuration;
  IdeBuildManager *build_manager;
  IdePerspective *editor;
  IdeContext *context;
  GtkWidget *pane;

  g_assert (IDE_IS_WORKBENCH_ADDIN (addin));
  g_assert (GBP_IS_BUILD_WORKBENCH_ADDIN (self));
  g_assert (IDE_IS_WORKBENCH (workbench));

  self->workbench = workbench;

  context = ide_workbench_get_context (workbench);

  build_manager = ide_context_get_build_manager (context);

  g_signal_connect_object (build_manager,
                           "build-started",
                           G_CALLBACK (gbp_build_workbench_addin_set_result),
                           self,
                           G_CONNECT_SWAPPED);

  configuration_manager = ide_context_get_configuration_manager (context);
  configuration = ide_configuration_manager_get_current (configuration_manager);

  editor = ide_workbench_get_perspective_by_name (workbench, "editor");
  pane = ide_editor_perspective_get_right_edge (IDE_EDITOR_PERSPECTIVE (editor));
  self->panel = g_object_new (GBP_TYPE_BUILD_PANEL,
                              "visible", TRUE,
                              NULL);
  gtk_container_add (GTK_CONTAINER (pane), GTK_WIDGET (self->panel));

  pane = ide_editor_perspective_get_bottom_edge (IDE_EDITOR_PERSPECTIVE (editor));
  self->build_log_panel = g_object_new (GBP_TYPE_BUILD_LOG_PANEL, NULL);
  gtk_container_add (GTK_CONTAINER (pane), GTK_WIDGET (self->build_log_panel));

  gtk_widget_insert_action_group (GTK_WIDGET (workbench), "build-tools",
                                  G_ACTION_GROUP (self->actions));

  g_object_bind_property (self, "result", self->panel, "result", 0);

  self->build_perspective = g_object_new (GBP_TYPE_BUILD_PERSPECTIVE,
                                          "configuration-manager", configuration_manager,
                                          "configuration", configuration,
                                          "visible", TRUE,
                                          NULL);
  ide_workbench_add_perspective (workbench, IDE_PERSPECTIVE (self->build_perspective));
}

static void
gbp_build_workbench_addin_unload (IdeWorkbenchAddin *addin,
                                  IdeWorkbench      *workbench)
{
  GbpBuildWorkbenchAddin *self = (GbpBuildWorkbenchAddin *)addin;

  g_assert (IDE_IS_WORKBENCH_ADDIN (addin));
  g_assert (GBP_IS_BUILD_WORKBENCH_ADDIN (self));
  g_assert (IDE_IS_WORKBENCH (workbench));

  gtk_widget_insert_action_group (GTK_WIDGET (workbench), "build-tools", NULL);

  gtk_widget_destroy (GTK_WIDGET (self->panel));
  self->panel = NULL;
}

static void
gbp_build_workbench_addin_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GbpBuildWorkbenchAddin *self = GBP_BUILD_WORKBENCH_ADDIN(object);

  switch (prop_id)
    {
    case PROP_RESULT:
      g_value_set_object (value, self->result);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
gbp_build_workbench_addin_finalize (GObject *object)
{
  GbpBuildWorkbenchAddin *self = (GbpBuildWorkbenchAddin *)object;

  g_clear_object (&self->actions);
  g_clear_object (&self->result);

  G_OBJECT_CLASS (gbp_build_workbench_addin_parent_class)->finalize (object);
}

static void
gbp_build_workbench_addin_class_init (GbpBuildWorkbenchAddinClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gbp_build_workbench_addin_finalize;
  object_class->get_property = gbp_build_workbench_addin_get_property;

  properties [PROP_RESULT] =
    g_param_spec_object ("result",
                         "Result",
                         "The current build result",
                         IDE_TYPE_BUILD_RESULT,
                         (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, LAST_PROP, properties);
}

static void
gbp_build_workbench_addin_init (GbpBuildWorkbenchAddin *self)
{
  self->actions = g_simple_action_group_new ();

  g_action_map_add_action_entries (G_ACTION_MAP (self->actions),
                                   actions_entries,
                                   G_N_ELEMENTS (actions_entries),
                                   self);
}

static void
workbench_addin_iface_init (IdeWorkbenchAddinInterface *iface)
{
  iface->load = gbp_build_workbench_addin_load;
  iface->unload = gbp_build_workbench_addin_unload;
}
