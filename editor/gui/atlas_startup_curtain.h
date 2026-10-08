/**************************************************************************/
/*  atlas_startup_curtain.h                                               */
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

#pragma once

#include "scene/gui/control.h"

class Texture2D;

// Atlas Studio (engine-fork/patches/0003): el telón que tapa el editor mientras arranca.
//
// Sigue al cartel de arranque del proyecto (la misma imagen, el mismo color, el mismo tamaño) y
// tapa el editor a medio armar y el progreso del primer escaneo, que sigue corriendo debajo sin
// verse: en lugar de la ventanita de progreso, una línea que se mueve. Lo saca el plugin de
// Studio cuando la pantalla ya quedó como tiene que verse (lo busca por su nombre y lo libera); si
// no, se va solo a los TIMEOUT_SEC.
//
// Sólo aparece en un proyecto de Studio (Inicio, o un juego con el plugin de Studio activo): con
// cualquier otro, el editor arranca como siempre.
class AtlasStartupCurtain : public Control {
	GDCLASS(AtlasStartupCurtain, Control);

	static constexpr double TIMEOUT_SEC = 45.0;

	Ref<Texture2D> splash;
	Color bg_color;
	int stretch_mode = 1;
	double elapsed = 0.0;

	static ObjectID current;

protected:
	void _notification(int p_what);

public:
	static constexpr const char *NODE_NAME = "AtlasStartupCurtain";

	// Si el proyecto abierto es de Studio.
	static bool is_studio_project();
	// Si hay un telón arriba: mientras tanto, el progreso no se ve.
	static bool is_up();

	AtlasStartupCurtain();
};
