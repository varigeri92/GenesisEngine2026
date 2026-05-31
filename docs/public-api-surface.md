# Public API Surface

Public headers live under `Engine/API/`. The editor includes these headers instead of reaching directly into every engine internal header when possible.

## Umbrella Header

`Engine/API/Genesis.h` currently includes:

- `Engine.h`
- `Logger.h`
- `InputBackend.h`
- `System.h`
- `SystemsManager.h`
- `Camera.h`
- `Screen.h`
- `ComponentLibrary.h`
- `Scene.h`

Use this header for broad editor/application access to the core runtime. More specialized API headers expose GUI, rendering, and window controls.

## API Export Macro

`Engine/API/API.h` defines the DLL import/export surface through `GNS_API`.

Expected usage:

- `BUILD_DLL` when building `Engine`.
- `BUILD_EXE` when building `Editor`.

Types and functions exported across the engine/editor boundary use `GNS_API`.

## GUI API

Relevant headers:

- `GenesisGUI.h`
- `GenesisGUI_Backend.h`
- `GenesisMaterialIcons.h`

These expose the GUI window base class, backend-facing ImGui helpers, and Material Icons constants used by editor UI code. `GuiWindow` classes are editor/UI windows, not runtime systems unless they separately derive from and register as `gns::core::System`.

## Rendering API

`GenesisRendering.h` exposes renderer-facing types used by editor systems. The editor camera uses it to pass camera backend data into `RenderSystem`, and scene/editor UI code uses render descriptors through render-system APIs.

## Window API

`GenesisWindow.h` exposes main-window commands used by editor UI:

- minimize
- maximize/restore
- query maximize state
- request close

## System API Rule

The public runtime system API is `gns::core::System` plus `SystemsManager::RegisterSystem`, `GetSystem`, `Run`, `Clear`, and ECS helpers. A class is only a runtime system when it derives from `System` and is registered. Static managers, utility classes, jobs, and GUI windows can be public interaction points, but they are not systems unless they participate in that lifecycle.

## Include Direction

Application and editor code can include the public API headers. Engine internals should avoid depending on editor headers.

Renderer ownership remains stricter than public include convenience: public API exposure does not mean engine-side resource objects should carry Vulkan handles.
