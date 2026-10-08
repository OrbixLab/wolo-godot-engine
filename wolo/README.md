# El motor de Atlas Studio

Este fork de Godot es el motor de **Atlas Studio**, el editor de juegos de Atlas que vive en
[WoloSimulator](https://github.com/OrbixLab/WoloSimulator). Es Godot 4.7.2 con cambios chicos
encima, todos del editor: el telón de arranque, el título y el ícono de Atlas, y la recarga de
archivos sin preguntar.

Hasta 2026-10-08 esos cambios eran parches en `WoloSimulator/engine-fork/patches/` que se aplicaban
sobre un clon de Godot en cada compilación. Ahora viven acá, versionados, y lo que usa Studio es
una **release** de este repositorio: nadie del equipo necesita Visual Studio para trabajar en el
simulador.

| | |
|---|---|
| `wolo/4.7` | La rama del motor: `4.7.2-stable` de Godot más los cambios de abajo. `master` sigue a Godot sin tocar. |
| `wolo/README.md` | Esto: el registro de cambios y cómo compilar, publicar y actualizar. |
| `wolo/build-windows.ps1` | Compila el editor para Windows (y opcionalmente lo instala en WoloSimulator o arma el zip). |
| `.github/workflows/wolo_release.yml` | El CI: compila Windows y Linux y, con un tag, publica la release. |

## Registro de cambios al motor

**La regla: tocar el motor lo menos posible, y nada que llegue al código que corre el juego.** El hub
corre los juegos con Godot oficial, y el backend los compila con Godot oficial: un cambio que el
juego pueda notar (un nombre, una versión, un comportamiento) rompería la simetría entre lo que el
creador prueba en Studio y lo que pasa en el teléfono. Antes de agregar un cambio, buscar la forma
de hacerlo desde un ajuste del proyecto o desde el plugin de Studio (lo que ya se resolvió así está
en `WoloSimulator/engine-fork/README.md`).

Cada cambio queda en esta tabla, y en el código con un comentario `Atlas Studio (wolo/README.md,
cambio N)`, así `git grep "wolo/README.md"` los encuentra todos.

| # | Qué | Dónde | ¿Llega al juego? | Al cambiar de versión de Godot |
|---|---|---|---|---|
| 1 | Los metadatos del ejecutable de Windows: empresa "Wolo Labs", descripción y producto "Atlas Studio", y el copyright con el aviso de Godot, que la licencia MIT pide conservar | `platform/windows/godot_res.rc` | No: son las propiedades del archivo `.exe` | Rehacer a mano los cinco `VALUE` del bloque `040904b0` si hay conflicto |
| 2 | El ícono del ejecutable (el de `WoloSimulator/wolo-labs-simulator/tools/atlas-studio.ico`) | `platform/windows/godot.ico` | No | Nada: es el mismo archivo en la misma ruta. Si cambia el ícono en WoloSimulator, copiarlo acá |
| 3 | El ícono de la ventana desde el primer frame (el de `addons/atlas_studio/brand/app_icon.png`). El motor pone el suyo al arrancar y el plugin el de Atlas recién al cargar: la barra de tareas mostraba uno y después el otro | `main/app_icon.png` | Sólo al juego que corre en el Simulador, si su proyecto no tiene ícono (el motor le pone el de Atlas en vez del de Godot). El que se publica lo compila el backend con Godot oficial | Nada, como el 2 |
| 4 | El título de la ventana termina en "Atlas Studio", no en "Godot Engine", desde el primer frame (el plugin lo reescribía recién a los 2 s) | `EditorNode::_update_title` en `editor/editor_node.cpp` | No: es el editor | Una línea: el `window_set_title` del final de `_update_title` |
| 5 | **El telón de arranque.** En un proyecto de Studio (Inicio, o un juego con el plugin de Studio), un panel con el mismo cartel de arranque tapa el editor desde el primer frame, con una línea que se mueve. El progreso del primer escaneo e importación sigue corriendo debajo, sin verse. Lo saca el plugin (por su nombre, `AtlasStartupCurtain`) cuando la pantalla ya quedó en Simulador o Atlas; si no, se va solo a los 45 s. Con cualquier otro proyecto, el editor arranca como siempre | `editor/gui/atlas_startup_curtain.{h,cpp}` (nuevos), el constructor de `EditorNode` y `ProgressDialog::_popup` | No: es el editor | La clase nueva no depende de nada interno salvo `get_splash_stretched_screen_rect` (el mismo tamaño que el cartel). Revisar dónde el constructor de `EditorNode` crea el `ProgressDialog` y que `_popup` siga fijando `center_panel` |
| 6 | **La recarga sin preguntar.** Dos ajustes de editor nuevos, apagados por defecto (como Godot), que `core/editor_brand.gd` de Studio prende: `interface/editor/behavior/auto_reload_external_changes` (escenas abiertas, `project.godot` y scripts con cambios sin guardar se recargan del disco sin el diálogo; el agente es quien los cambia) y `run/window_placement/game_embed_keep_main_screen` (el juego incrustado no se lleva la pantalla a "Juego": se ve en el Simulador; y su ventana se crea donde está el control que lo incrusta, el teléfono, y no en el centro de la pantalla principal, donde aparecía un momento suelta) | `editor/settings/editor_settings.cpp`, `EditorNode::_scan_external_changes`, `ScriptEditor::_test_script_times_on_disk`, `GameView::_play_pressed`, `_embedded_process_focused` y `_get_arguments` | No: es el editor | Buscar dónde se abre el diálogo `disk_changed` en `EditorNode` y en `ScriptEditor`, dónde `GameView` hace `select(EDITOR_GAME)` y dónde calcula el `--position` del juego |
| 7 | **La versión dice `wolo`**: `4.7.2.stable.wolo.<commit>` en lugar de `official` o `custom_build`. No es un cambio de código: los scripts de compilación definen `BUILD_NAME=wolo`. Así `tools/package-windows.ps1` sabe que el motor es éste | `wolo/build-windows.ps1`, `.github/workflows/wolo_release.yml` | Sólo `Engine.get_version_info().build` del juego que corre en el Simulador, que con cualquier compilación propia ya decía `custom_build` | Nada |
| 8 | **El crash al abrir un juego recién creado.** El editor se cerraba a veces (1 de cada 3 a 6 aperturas) al importar por primera vez las tres fuentes de un proyecto de Studio: Godot las importa en paralelo y el guardado lee `ClassDB` desde varios hilos sin lock ([godotengine/godot#111039](https://github.com/godotengine/godot/issues/111039)). Es el arreglo de upstream, [#123546](https://github.com/godotengine/godot/pull/123546), traído de Godot 4.8 | `ClassDB::class_get_default_property_value` en `core/object/class_db.cpp` | No cambia lo que hace: sólo pone un lock. El juego del Simulador lo tiene; el del teléfono no lo necesita (no importa nada) | Al pasar a 4.8 se saca: ya viene en Godot |

**Sacado el 2026-10-05:** cambiar `name` en `version.py` a "Atlas Studio". Ese nombre no queda en el
editor: lo usan la versión que reciben las extensiones (`gdextension_interface`), el nombre del
motor que recibe Vulkan, el encabezado del log y los reportes de crash, todos del lado del juego.
Los metadatos del `.exe` llevan "Atlas Studio" escrito directamente (cambio 1).

## Publicar una versión

Las versiones se llaman `<versión de Godot>-wolo.<n>`: `4.7.2-wolo.1`, `4.7.2-wolo.2`… El `n`
sube con cada cambio nuestro y vuelve a 1 al cambiar de versión de Godot.

1. Commitear en `wolo/4.7` y subir la rama.
2. Crear y subir el tag desde ese commit:
   ```sh
   git tag 4.7.2-wolo.2
   git push origin 4.7.2-wolo.2
   ```
3. El workflow **🐺 Wolo - motor de Atlas Studio** compila Windows (MSVC) y Linux (GCC, con LTO) y
   publica la release con `wolo-godot-windows.zip` (trae `godot.exe`), `wolo-godot-linux.zip` (trae
   `godot`) y `SHA256SUMS`. Lleva entre 30 minutos y una hora y media; la caché de SCons acorta las
   siguientes.
4. En WoloSimulator, poner el tag nuevo en `engine-fork/ENGINE_VERSION` y commitearlo. Cada
   desarrollador lo baja con `bash wolo-labs-simulator/tools/fetch-engine.sh`.

Para probar una rama sin publicar: Actions → el mismo workflow → **Run workflow** sobre la rama. Los
zips quedan como artifacts del run (7 días).

**Una vez, en la configuración del repositorio:** crear la variable `DISABLE_GODOT_CI` = `true`
(Settings → Secrets and variables → Actions → Variables). Sin ella, cada push corre el CI completo
de Godot (`runner.yml`: todas las plataformas, horas de runner). Con ella sólo corre el nuestro.

## Compilar a mano (Windows)

Hace falta para cambiar el motor, no para trabajar en el simulador.

```powershell
powershell -ExecutionPolicy Bypass -File wolo/build-windows.ps1 -Install C:\ruta\a\WoloSimulator
```

Necesita Visual Studio (Build Tools 2019 o más nuevo, con C++), Python 3 y SCons
(`python -m pip install scons`). La primera vez baja AccessKit, ANGLE y el SDK de Direct3D 12 a
`%LOCALAPPDATA%\Godot\build_deps` y compila todo (unos 13 minutos en 12 núcleos); después, sólo lo
que cambió. `-Install` copia el ejecutable a `wolo-labs-simulator/engine/godot.exe` y marca
`engine/ENGINE_VERSION` como `local`, para que `fetch-engine.sh` lo reemplace por la release fijada
la próxima vez que se corra.

Si el checkout está en una ruta muy larga, el compilador choca con el límite de 260 caracteres de
Windows: clonar el fork en una ruta corta. Si falla con `C1356: no se puede encontrar
mspdbcore.dll` a mitad de camino, es falta de memoria (cada `cl.exe` de los archivos grandes del
editor pide más de 1 GB): volver a correrlo con menos compilaciones a la vez, `-Jobs 6`; retoma
donde quedó. Medido el 2026-10-08 en 12 núcleos y 16 GB: con 12 falla, con 6 anda.

Con MSVC, `production=yes` no usa LTO: Godot lo apaga porque con ese compilador no ayuda
(`platform/windows/detect.py`). Las compilaciones oficiales usan MinGW con LTO; esa es una
diferencia de compilador, no de código. WinRT (las voces de texto a habla de OneCore) pide el
Windows SDK 10.0.22621 o más nuevo: sin él, el script compila con `winrt=no`. Studio no usa esas
voces.

## Actualizar la versión de Godot

1. Traer el tag de Godot: `git fetch https://github.com/godotengine/godot.git tag 4.7.3-stable`.
2. En una rama nueva desde `wolo/4.7`, `git merge 4.7.3-stable` (en la misma línea 4.7 los
   conflictos deberían ser pocos; para 4.8, rama nueva `wolo/4.8` desde `4.8-stable` y
   `git cherry-pick` de los commits de `wolo/4.7`).
3. Resolver con la columna "Al cambiar de versión de Godot" de la tabla.
4. Compilar, probar Studio entero (`ATLAS_CREATOR_PLATFORM.md` §10 en WoloSimulator: lo del plugin
   depende de nombres internos del editor) y publicar `4.7.3-wolo.1`.

La plataforma entera usa la misma versión de Godot: el hub y el backend también. Cambiarla acá es
cambiarla en todos lados.
