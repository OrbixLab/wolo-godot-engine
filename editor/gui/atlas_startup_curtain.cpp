/**************************************************************************/
/*  atlas_startup_curtain.cpp                                             */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "atlas_startup_curtain.h"

#include "core/config/project_settings.h"
#include "core/io/image_loader.h"
#include "scene/resources/image_texture.h"
#include "servers/rendering/rendering_server_types.h"

ObjectID AtlasStartupCurtain::current;

bool AtlasStartupCurtain::is_studio_project() {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	if (ps->has_setting("atlas/studio/home") && bool(ps->get_setting("atlas/studio/home"))) {
		return true;
	}
	if (!ps->has_setting("editor_plugins/enabled")) {
		return false;
	}
	const PackedStringArray plugins = ps->get_setting("editor_plugins/enabled");
	for (const String &plugin : plugins) {
		if (plugin.contains("/addons/atlas_studio/")) {
			return true;
		}
	}
	return false;
}

bool AtlasStartupCurtain::is_up() {
	return current.is_valid() && ObjectDB::get_instance(current) != nullptr;
}

void AtlasStartupCurtain::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			current = get_instance_id();
		} break;

		case NOTIFICATION_EXIT_TREE: {
			if (current == get_instance_id()) {
				current = ObjectID();
			}
		} break;

		case NOTIFICATION_INTERNAL_PROCESS: {
			elapsed += get_process_delta_time();
			if (elapsed > TIMEOUT_SEC) {
				queue_free();
				return;
			}
			queue_redraw();
		} break;

		case NOTIFICATION_DRAW: {
			const Size2 size = get_size();
			draw_rect(Rect2(Point2(), size), bg_color);
			if (splash.is_valid()) {
				const Rect2 rect = RenderingServerTypes::get_splash_stretched_screen_rect(splash->get_size(), size, RSE::SplashStretchMode(stretch_mode));
				draw_texture_rect(splash, rect);
			}
			// Una línea fina abajo que va y viene: está trabajando. Lo que hace (el progreso del
			// escaneo y la importación) no es del creador.
			const real_t width = MIN(size.width * 0.3, 360.0);
			const real_t y = size.height - MAX(size.height * 0.12, 48.0);
			const Rect2 track(Point2((size.width - width) / 2.0, y), Size2(width, 2.0));
			draw_rect(track, Color(1, 1, 1, 0.08));
			const real_t t = Math::fposmod(elapsed * 0.6, 1.0);
			const real_t phase = t < 0.5 ? t * 2.0 : (1.0 - t) * 2.0;
			const real_t bar = width * 0.3;
			draw_rect(Rect2(track.position + Vector2((width - bar) * phase, 0), Size2(bar, 2.0)), Color(0.945, 0.729, 0.255, 0.9));
		} break;
	}
}

AtlasStartupCurtain::AtlasStartupCurtain() {
	set_name(NODE_NAME);
	set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	set_mouse_filter(MOUSE_FILTER_STOP);
	// Arriba del editor, debajo del progreso (que igual no se ve mientras esté el telón).
	set_z_index(RSE::CANVAS_ITEM_Z_MAX - 1);
	set_process_internal(true);

	ProjectSettings *ps = ProjectSettings::get_singleton();
	bg_color = ps->get_setting("application/boot_splash/bg_color", Color(0.14, 0.14, 0.14));
	stretch_mode = ps->get_setting("application/boot_splash/stretch_mode", 1);
	const String path = ps->get_setting("application/boot_splash/image", String());
	if (bool(ps->get_setting("application/boot_splash/show_image", true)) && !path.is_empty() && !path.begins_with("uid://")) {
		Ref<Image> image;
		image.instantiate();
		if (ImageLoader::load_image(path, image) == OK) {
			splash = ImageTexture::create_from_image(image);
		}
	}
}
