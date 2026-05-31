# Odd Code Findings

Scope: first-party project files under `Engine`, `Editor`, and root Premake Lua files. External/vendor/generated areas are out of scope. Architecture-rule violations found during the same audit are recorded separately in `violations.md`.

This page tracks current oddities that are not necessarily rule violations. They are candidates for later cleanup or design decisions.

## Current Findings

1. `Engine/Engine.cpp:49`, `Engine/Engine.cpp:55`, `Editor/Editor.cpp:32`, `Editor/Editor.cpp:40`, `Editor/Editor.cpp:50`

   Odd code: runtime/editor startup still enables and registers dev/test surfaces as part of normal boot: `TestSystem`, `TestWindow`, `TestSystemExternal`, and `TestEditorWindow`.

   Move target: move these into examples, tests, or an explicit dev diagnostics mode.

   Reason: test/demo systems and windows obscure the minimal runtime/editor dependency graph.

2. `Engine/Gui/GuiBackend.cpp:17`, `Engine/Gui/GuiBackend.cpp:111`, `Engine/Gui/GuiBackend.cpp:242`

   Odd code: the low-level GUI backend owns Genesis editor styling, editor font loading, Material Icons setup, and always shows the Dear ImGui demo window.

   Move target: move styling and fonts to an editor theme/bootstrap module. Move `ImGui::ShowDemoWindow()` to a debug window or dev-only flag.

   Reason: `GuiBackend` should initialize and drive ImGui's SDL/Vulkan backend. Product/editor UI theme and demo-window policy belong higher up.

3. `Engine.lua`, `ImGui.lua`, `Editor.lua`

   Odd code: ImGui ownership is split. `Engine.lua` compiles ImGui and its SDL/Vulkan backends directly, while a standalone `ImGui` project still exists and `Editor.lua` depends on it.

   Move target: choose one owner for ImGui compilation. Either make `Engine` depend on an `ImGui` static library, or keep ImGui fully embedded in `Engine` and remove the separate project dependency.

   Reason: split build ownership makes dependency direction and duplicate compilation risk harder to reason about.

4. `Engine/Window/Window.cpp`, `Engine/Input/InputBackend.cpp`

   Odd code: borderless-window hit testing lives in the window layer, while resize cursor/capture behavior lives in input.

   Move target: consolidate borderless-window interaction policy into `Window`, `WindowSystem`, or a dedicated borderless window controller.

   Reason: input should collect input state; window movement, hit testing, cursor policy, and resizing are window behavior.

5. `Engine/Scene/SceneSystem.cpp:15`

   Odd code: `SceneSystem::OnStart` creates an `"empty(scene)"` scene automatically when no active scene exists.

   Move target: decide whether default scene creation is engine policy, editor bootstrap policy, or a project-template operation.

   Reason: automatic scene creation is convenient, but it makes scene lifetime less explicit for headless/runtime use cases.

6. `Engine/Renderer/RenderSystem.cpp:94`, `Engine/Scene/SceneAssetImporter.cpp`, `Engine/Scene/SceneSerializer.cpp`, `Engine/Assets/AssetManager.cpp`

   Odd code: several non-system helper/static layers retrieve systems through `SystemsManager::GetSystem`.

   Move target: prefer passing required system dependencies through system-owned public APIs where possible.

   Reason: hidden global system lookup makes dependency direction harder to see. It is not always wrong in the current architecture, but it should be intentional.

## Resolved Or Moved

- The old `RenderSystem::LoadMeshAssetIntoScene` concern has moved to `SceneAssetImporter`.
- The old `SceneViewWindow` asset-import ownership concern has largely moved to `ModelImportController` and asset metadata helpers.
- Hard architecture-rule violations are now tracked in `violations.md`.
