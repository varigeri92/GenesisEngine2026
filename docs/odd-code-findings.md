# Odd Code Findings

Scope: first-party tracked project files under `Engine`, `Editor`, root Premake Lua files, and docs used for architecture context. External/vendor/generated areas such as `vendor`, `submodules`, `bin`, `bin-int`, `obj`, `stb_image`, and generated icon tables were treated as out of scope unless first-party integration code referenced them.

No source files were changed for this report.

## Findings

1. `Editor/EditorGUI/Windows/SceneViewWindow.cpp:8`, `Editor/EditorGUI/Windows/SceneViewWindow.cpp:21`, `Editor/EditorGUI/Windows/SceneViewWindow.cpp:102`, `Editor/EditorGUI/Windows/SceneViewWindow.cpp:111`, `Editor/EditorGUI/Windows/SceneViewWindow.cpp:169`, `Editor/EditorGUI/Windows/SceneViewWindow.cpp:206`, `Editor/EditorGUI/Windows/SceneViewWindow.h:16`

   Odd code: `SceneViewWindow` owns the full model import popup state, import option struct, YAML `.meta` file generation, asset type string conversion, and final import execution.

   Move target: split into an editor asset import controller/window such as `Editor/Assets/ModelImportController` plus an asset metadata writer under `Engine/Assets` or `Editor/Assets`.

   Reason: the scene view should display the render target, accept scene interactions, and draw gizmos. Asset import policy and metadata serialization are asset/editor responsibilities, not viewport responsibilities.

2. `Engine/Renderer/RenderSystem.cpp:279`, `Engine/Renderer/RenderSystem.cpp:284`, `Engine/Renderer/RenderSystem.cpp:304`, `Engine/Renderer/RenderSystem.cpp:313`, `Engine/Renderer/RenderSystem.cpp:329`, `Engine/Renderer/RenderSystem.cpp:344`

   Odd code: `RenderSystem::LoadMeshAssetIntoScene` imports an asset, creates ECS entities, parents them, assigns transforms, applies materials, and names scene objects.

   Move target: move asset-to-scene instantiation into a scene/editor bridge such as `SceneAssetImporter`, `AssetInstantiationSystem`, or an editor command layer. Keep `RenderSystem` focused on engine handle to Vulkan handle mapping and draw data.

   Reason: the AGENTS.md ownership rule says `RenderSystem` is the bridge between engine resources and Vulkan resources. Importing files and mutating scene hierarchy is a higher-level scene/editor workflow.

3. `Engine/Object/Mesh.cpp:7`, `Engine/Object/Mesh.cpp:10`, `Engine/Object/Texture.cpp:20`, `Engine/Object/Texture.cpp:23`, `Engine/Renderer/Shader.cpp:23`, `Engine/Renderer/Shader.cpp:26`

   Odd code: engine-side resource objects call `SystemsManager::GetSystem<RenderSystem>()` and trigger renderer upload through `Apply()`.

   Move target: let `RenderSystem`, an asset import/apply service, or a resource bridge initiate uploads. Engine resource classes should stay data/resource oriented.

   Reason: engine objects do not store Vulkan handles, which is good, but they still know the render system exists. That couples core resource types to renderer/system lifetime and creates null-render-system crash paths.

4. `Engine/Renderer/RenderSystem.cpp:146`, `Engine/Renderer/RenderSystem.cpp:159`, `Engine/Renderer/RenderSystem.cpp:164`, `Engine/Renderer/RenderSystem.cpp:192`

   Odd code: material resource cache entries map the engine material handle back to itself.

   Move target: finish the planned backend material resource path with `VulkanMaterial` or a renderer-owned material binding cache.

   Reason: the documented cache shape is `engine resource handle -> Vulkan resource handle`. Identity mapping is a temporary ownership gap and will become riskier as materials gain texture sets, descriptor sets, and lifetime rules.

5. `Engine/Gui/GuiBackend.cpp:15`, `Engine/Gui/GuiBackend.cpp:109`, `Engine/Gui/GuiBackend.cpp:215`, `Engine/Gui/GuiBackend.cpp:231`, `Engine/Gui/GuiBackend.cpp:244`

   Odd code: the low-level GUI backend owns Genesis editor styling, editor font loading from `EditorResources`, Material Icons setup, and always shows the Dear ImGui demo window.

   Move target: move styling and fonts to an editor theme/bootstrap module. Move `ImGui::ShowDemoWindow()` to a debug window or dev-only flag.

   Reason: `GuiBackend` should initialize and drive ImGui's SDL/Vulkan backend. Product/editor UI theme and demo windows are application/editor policy.

6. `Engine/Engine.cpp:6`, `Engine/Engine.cpp:9`, `Engine/Engine.cpp:39`, `Engine/Engine.cpp:44`, `Editor/Editor.cpp:10`, `Editor/Editor.cpp:13`, `Editor/Editor.cpp:84`, `Editor/Editor.cpp:93`

   Odd code: runtime/editor startup registers `TestSystem`, `TestWindow`, `TestSystemExternal`, and `TestEditorWindow` as normal boot behavior.

   Move target: move these into examples, tests, or a dev-only startup path gated by explicit config.

   Reason: test/demo systems should not be part of production engine/editor boot. They obscure what the actual runtime depends on and can change behavior unexpectedly.

7. `Editor/Editor.cpp:25`, `Editor/Editor.cpp:27`, `Editor/Editor.cpp:39`, `Editor/Editor.cpp:52`, `Editor/Editor.cpp:95`

   Odd code: `RunYamlCppSmokeTest` writes `yaml-cpp-smoke-test.yaml` into the project root on every editor launch, reads it back, and logs its contents.

   Move target: move to a test executable, build validation task, or explicit diagnostic command.

   Reason: opening the editor should not create project files just to prove a dependency works.

8. `Editor/EditorCameraSystem.h:35`, `Editor/TestEditorWindow.cpp:11`, `Editor/TestEditorWindow.cpp:22`, `Editor/TestEditorWindow.cpp:29`, `Editor/TestEditorWindow.cpp:39`

   Odd code: `EditorCameraSystem` exposes a public `test` flag, and `TestEditorWindow` toggles it while also editing camera and light state.

   Move target: remove the unused flag, or move camera/light debug controls into a named debug inspector panel.

   Reason: this is debug UI and state leaking into the editor camera system. The flag is not used by the camera system, so it is dead state.

9. `Engine/Renderer/Renderer.cpp:55`, `Engine/Renderer/Renderer.cpp:60`, `Engine/Renderer/Vulkan/Device.cpp:66`, `Engine/Renderer/Vulkan/Device.cpp:630`, `Engine/Renderer/Vulkan/Device.cpp:673`, `Engine/Renderer/Vulkan/Device.cpp:678`, `Engine/Renderer/Vulkan/Device.cpp:698`, `Engine/Renderer/Vulkan/Device.h:134`, `Engine/Renderer/Vulkan/Device.h:214`

   Odd code: the renderer background pass calls `Device::DrawTest`, while `Device` owns `_gradientPipeline`, `init_pipelines`, `init_background_pipelines`, and hard-coded `Shaders/sky.comp.spv` loading.

   Move target: rename and isolate this as a real background/sky pass resource, or move it to a debug/sample render pass.

   Reason: low-level device setup should not contain a named test path or hard-coded sample pass. The current naming hides a real render dependency behind test terminology.

10. `Engine/Input/InputBackend.cpp:9`, `Engine/Input/InputBackend.cpp:46`, `Engine/Input/InputBackend.cpp:67`, `Engine/Input/InputBackend.cpp:99`, `Engine/Input/InputBackend.cpp:120`, `Engine/Input/InputBackend.cpp:169`, `Engine/Input/InputBackend.cpp:245`, `Engine/Input/InputBackend.cpp:263`, `Engine/Window/Window.cpp:8`, `Engine/Window/Window.cpp:12`, `Engine/Window/Window.cpp:66`

    Odd code: borderless window resize state, resize cursors, mouse capture, and SDL window resizing live in `InputBackend`, while hit testing lives in `Window.cpp`.

    Move target: consolidate into `Window`, `WindowSystem`, or a dedicated borderless window controller.

    Reason: input should collect input state. Window resizing, hit testing, and cursor policy belong to the window layer, and the split currently duplicates constants and edge logic.

11. `Engine/Utils/Path.h:10`, `Engine/Utils/Path.cpp:43`, `Engine/Utils/Path.cpp:45`

    Odd code: `DefaultProjectRoot` is a hard-coded machine-local path: `D:\ProjectGenesis\TestProject\`.

    Move target: resolve the default project root from command-line/config, project discovery, or an editor project selection flow.

    Reason: AGENTS.md says rooted machine-local paths are allowed at process boundaries and inside configured path state. A compiled-in test project root is not portable.

12. `Engine/Window/WindowSystem.cpp:11`, `Engine/Window/WindowSystem.cpp:13`

    Odd code: the window system creates a fixed `1920x1080` window named `GenesisTestWindow`.

    Move target: put title and initial size in `EngineConfig` or a window config object.

    Reason: the window system should consume window configuration, not embed test naming and dimensions.

13. `Editor/EditorGUI/Windows/ProjectFilesWindow.cpp:14`, `Editor/EditorGUI/Windows/ProjectFilesWindow.cpp:19`, `Editor/EditorGUI/Windows/ProjectFilesWindow.cpp:25`, `Editor/EditorGUI/Windows/ProjectFilesWindow.cpp:44`, `Editor/EditorGUI/Windows/ProjectFilesWindow.cpp:77`

    Odd code: `ProjectFilesWindow` owns generic path helpers such as meta detection, display-name selection, child/root path checks, visible child enumeration, and directory-first sorting.

    Move target: move generic path/root checks to `gns::path`; move reusable project browser enumeration/sorting to an editor project file model.

    Reason: AGENTS.md says path normalization, root checks, relative paths, and existence helpers should stay centralized in `gns::path`. The UI window should render a model, not own generic filesystem rules.

14. `Editor/EditorAssetDragDrop.h:23`, `Editor/EditorAssetDragDrop.h:33`, `Editor/EditorAssetDragDrop.h:58`

    Odd code: asset type detection from file extension is implemented inside a drag-drop payload header.

    Move target: move extension-to-asset-type mapping to `Engine/Assets` or an editor asset classification service, then let drag-drop payload creation call that API.

    Reason: asset classification is broader than drag/drop. Importers, project browser, metadata generation, and validation will need the same rules.

15. `Engine/Assets/AssetManager.h:13`, `Engine/Assets/AssetManager.h:50`, `Engine/Assets/AssetManager.cpp:20`

    Odd code: `Asset`, `AssetMap`, and an asset registry shape exist, but `AssetMap` is not used anywhere else.

    Move target: either implement this as a real asset registry/catalog or remove it until needed.

    Reason: unused registry state suggests ownership that does not actually exist and confuses the role of `AssetManager`.

16. `Engine/Object/IObject.h:44`, `Engine/Object/IObject.h:50`

    Odd code: `Object::LoadFromFile` and `Object::Find` are templated API stubs that always return `nullptr`.

    Move target: move file loading to `AssetManager`/asset registry, and either implement lookup in `Object` or remove the stub API.

    Reason: object storage and asset loading are separate responsibilities. Stub APIs make callers think a behavior exists when it does not.

17. `Engine.lua:67`, `Engine.lua:72`, `Engine.lua:74`, `ImGui.lua:10`, `ImGui.lua:16`, `ImGui.lua:31`, `ImGui.lua:36`, `Editor.lua:50`

    Odd code: the standalone `ImGui` project links `Engine.lib` and Assimp/Vulkan, while `Engine.lua` also compiles ImGui backend/core sources directly. `Editor.lua` still depends on `ImGui`.

    Move target: choose one owner for ImGui compilation. Either make `Engine` depend on an `ImGui` static library, or keep ImGui fully embedded in `Engine` and remove the separate project dependency.

    Reason: build ownership is split and likely compiles the same third-party sources in multiple targets. The ImGui target should not depend back on the engine it is meant to support.

18. `Engine/Renderer/Vulkan/PipelineBuilder.cpp:5`, `Engine/Renderer/Vulkan/PipelineBuilder.cpp:86`, `Engine/Renderer/Vulkan/PipelineBuilder.cpp:97`

    Odd code: `PipelineBuilder` includes `FileSystemUtils.h` and calls `fileUtils::HasFileExtension`.

    Move target: use `gns::path::HasExtension` directly.

    Reason: AGENTS.md says `FileSystemUtils.h` is only a compatibility wrapper over `gns::path` and should not regain independent usage in new renderer code.

19. `Engine/Scene/SceneManager.cpp:31`, `Engine/Scene/SceneManager.cpp:80`, `Engine/Scene/SceneManager.cpp:105`

    Odd code: `SceneManager::CreateScene` always injects ambient and directional light entities through `CreateDefaultSceneEntities`.

    Move target: move default scene content to a scene template, editor "new scene" command, or project bootstrap layer.

    Reason: scene manager should manage scene lifetime and active-scene state. Authoring default content is editor/project policy and may not be wanted for every runtime-created scene.

20. `Engine/Systems/SystemsManager.cpp:2`, `Engine/Systems/SystemsManager.cpp:3`

    Odd code: `SystemsManager.cpp` includes `SystemsManager.h` twice.

    Move target: remove the duplicate include in a cleanup pass.

    Reason: this is not a relocation issue, but it is obvious source noise found during the scan.

