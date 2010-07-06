/*
 *
 *  bluez-tools - a set of tools to manage bluetooth devices for linux
 *
 *  Copyright (C) 2010  Alexander Orlenko <zxteam@gmail.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <glib.h>

#include "lib/dbus-common.h"
#include "lib/helpers.h"
#include "lib/adapter.h"
#include "lib/device.h"
#include "lib/audio.h"

static void audio_property_changed(Audio *audio, const gchar *name, const GValue *value, gpointer data)
{
	if (g_strcmp0(name, "state") == 0) {
		if (g_strcmp0(g_value_get_string(value), "connecting") == 0) {
			g_print("Connecting to an audio service\n");
		} else if (g_strcmp0(g_value_get_string(value), "connected") == 0) {
			g_print("Audio service is connected\n");
		} else {
			g_print("Audio service is disconnected\n");
		}
	}
}

static gchar *adapter_arg = NULL;
static gchar *connect_arg = NULL;
static gchar *disconnect_arg = NULL;

static GOptionEntry entries[] = {
	{"adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter name or MAC", "adapter#id"},
	{"connect", 'c', 0, G_OPTION_ARG_STRING, &connect_arg, "Connect to an audio device", "device#id"},
	{"disconnect", 'd', 0, G_OPTION_ARG_STRING, &disconnect_arg, "Disconnect from an audio device", "device#id"},
	{NULL}
};

int main(int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

	g_type_init();

	context = g_option_context_new("- a bluetooth generic audio manager");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "audio summary");
	g_option_context_set_description(context, "audio desc");

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("%s: %s\n", g_get_prgname(), error->message);
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	} else if (!connect_arg && !disconnect_arg) {
		g_print("%s", g_option_context_get_help(context, FALSE, NULL));
		exit(EXIT_FAILURE);
	}

	g_option_context_free(context);

	if (!dbus_connect(&error)) {
		g_printerr("Couldn't connect to dbus: %s", error->message);
		exit(EXIT_FAILURE);
	}

	Adapter *adapter = find_adapter(adapter_arg, &error);
	exit_if_error(error);

	Device *device = find_device(adapter, connect_arg != NULL ? connect_arg : disconnect_arg, &error);
	exit_if_error(error);

	// TODO: Test to AUDIO service

	Audio *audio = g_object_new(AUDIO_TYPE, "DBusObjectPath", device_get_dbus_object_path(device), NULL);
	g_signal_connect(audio, "PropertyChanged", G_CALLBACK(audio_property_changed), NULL);

	if (connect_arg) {
		if (g_strcmp0(audio_get_state(audio), "connected") == 0) {
			g_print("Audio service is already connected\n");
		} else if (g_strcmp0(audio_get_state(audio), "connecting") == 0) {
			//g_print("Audio service is already connecting\n");
		} else {
			audio_connect(audio, &error);
			exit_if_error(error);
		}
	} else if (disconnect_arg) {
		if (g_strcmp0(audio_get_state(audio), "disconnected") == 0) {
			g_print("Audio service is already disconnected\n");
		} else {
			audio_disconnect(audio, &error);
			exit_if_error(error);
		}
	}

	g_object_unref(audio);
	g_object_unref(device);
	g_object_unref(adapter);

	exit(EXIT_SUCCESS);
}
