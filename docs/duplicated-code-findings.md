# Duplicated Code Findings

Scope: first-party tracked project files under `Engine`, `Editor`, and root Premake Lua files. External/vendor/generated files were excluded. I used manual review plus a simple exact-body scan, then filtered out normal polymorphic methods such as `OnDraw` and system lifecycle overrides.

Cleanup is now in progress. Completed findings are marked `Done`; deferred findings carry a decision note.

## Duplicates To Collapse

1. `Engine/Core/Handles.h:5` and `Engine/Core/Handles.h:16`

   Duplicate: `HashString_constexpr_` and `HashString` contain the same FNV-1a loop.

   Collapse target: keep one `std::string_view` implementation and call it from the `std::string` overload.

   Reason: this was an exact body match. One implementation avoids hash drift if the algorithm changes.

2. `Engine/Systems/SystemsManager.h:14` and `Engine/Systems/SystemsManager.h:24`

   Duplicate: two `RegisterSystem` templates perform the same allocation, state assignment, and return pattern.

   Collapse target: use one constrained variadic template that also handles zero constructor arguments.

   Reason: the no-argument overload is just a special case of the variadic overload.

3. `Engine/Core/Entity.h:41` and `Engine/Core/Entity.h:49`

   Duplicate: two `AddComponent` overloads both call `registry.emplace<T>(entity_handle, ...)`.

   Collapse target: keep only the variadic overload; `Args...` can be empty.

   Reason: removes a redundant wrapper without changing callers.

4. `Engine/Renderer/RenderSystem.cpp:168`, `Engine/Renderer/RenderSystem.cpp:180`, `Engine/Renderer/RenderSystem.cpp:192`

   Duplicate: `GetRenderMeshHandle`, `GetRenderShaderHandle`, and `GetRenderMaterialHandle` all perform the same cache lookup, warning, handle logging, and invalid-handle return.

   Collapse target: add a private helper such as `FindCachedRenderHandle(cache, handle, missingMessage)`.

   Reason: all three functions differ only by which map and message they use.

5. `Engine/Renderer/RenderSystem.cpp:97`, `Engine/Renderer/RenderSystem.cpp:113`, `Engine/Renderer/RenderSystem.cpp:129`

   Duplicate: `ApplyMesh`, `ApplyShader`, and `ApplyTexture` repeat the same "check cache, create renderer resource, cache valid handle" pattern.

   Collapse target: add a private cache/apply helper that accepts the cache map and creation callback.

   Reason: the resource type differences are real, but the cache control flow is duplicated. Texture can still run its `FreeCPUSide` post-step after the shared helper returns.

6. `Engine/Object/Mesh.cpp:7`, `Engine/Object/Texture.cpp:20`, `Engine/Renderer/Shader.cpp:23`

   Duplicate: resource `Apply()` methods all fetch `RenderSystem` from `SystemsManager` and call a matching `RenderSystem::Apply*`.

   Collapse target: preferably remove these methods and let `RenderSystem` or an asset/resource bridge upload resources. If they stay temporarily, share a helper for system lookup and null handling.

   Reason: the duplicate pattern is also an ownership issue: engine-side resources should not need to know how to find the render system.

7. `Engine/Renderer/Renderer.cpp:318` and `Engine/Renderer/Vulkan/Device.cpp:454`

   Duplicate: `Renderer::ApplyTexture` and `Device::CreateDefaultTexture` both create a `VulkanTexture`, create a sampler, upload image data as `VK_FORMAT_R8G8B8A8_UNORM`, create a descriptor, check `descriptorSet`, and return the resource handle.

   Collapse target: move the common upload path into `Device`, for example `CreateTextureResource(data, size, format, usage, samplerInfo)`.

   Reason: regular textures and default textures should go through the same backend resource creation path so lifetime and descriptor behavior stay consistent.

8. `Engine/Renderer/Vulkan/DescriptorLayoutBuilder.cpp:26`, `Engine/Renderer/Vulkan/DescriptorLayoutBuilder.cpp:207`, `Engine/Renderer/Vulkan/DescriptorLayoutBuilder.cpp:88`, `Engine/Renderer/Vulkan/DescriptorLayoutBuilder.cpp:236`

   Duplicate: `DescriptorAllocatorGrowable::CreatePool` and `DescriptorAllocator::InitPool` duplicate descriptor pool-size creation and `VkDescriptorPoolCreateInfo` setup. Their `Allocate` methods also duplicate `VkDescriptorSetAllocateInfo` setup.

   Collapse target: add shared helpers for building pool sizes, descriptor pool create info, and descriptor set allocate info.

   Reason: growable and fixed allocators have different policies, but Vulkan struct setup is the same.

9. `Engine/Input/InputBackend.cpp:9`, `Engine/Input/InputBackend.cpp:46`, `Engine/Window/Window.cpp:8`, `Engine/Window/Window.cpp:12`

   Duplicate: border size/title bar/button area constants and edge detection exist in both input and window hit-test code.

   Collapse target: move borderless-window metrics and edge classification into a shared window helper or the window system.

   Reason: mismatched constants would make resize cursor behavior disagree with SDL hit-test behavior.

10. `Editor/EditorCameraSystem.cpp:25` and `Editor/EditorCameraSystem.cpp:68`

    Duplicate: camera forward/right/up vector calculation and view/projection update logic are repeated in `InitCamera` and `UpdateCamera`.

    Collapse target: extract `RebuildCameraBasis` and `RebuildCameraMatrices`, then call them from both paths.

    Reason: initialization and per-frame update should not maintain separate copies of the same camera math.

11. `Editor/EditorGUI/EditorWidgets.cpp:35`, `Editor/EditorGUI/EditorWidgets.cpp:41`, `Editor/EditorGUI/EditorWidgets.cpp:47`, `Editor/EditorGUI/EditorWidgets.cpp:53`, plus `Editor/EditorGUI/Windows/InspectorWindow.cpp:24`, `Editor/EditorGUI/Windows/InspectorWindow.cpp:41`, `Editor/EditorGUI/Windows/InspectorWindow.cpp:44`, `Editor/EditorGUI/Windows/InspectorWindow.cpp:47`

    Duplicate: float, float2, float3, and float4 widget wrappers all repeat the same setup and differ only by ImGui drag function. The inspector repeats similar `DragFloat*` dispatch.

    Collapse target: centralize numeric vector field widgets in `EditorWidgets`, either with a component-count helper or a small typed dispatch table.

    Reason: this keeps reflected inspector fields and custom debug widgets visually and behaviorally consistent.

12. `Engine/Utils/Random.h:15`, `Engine/Utils/Random.h:29`, `Engine/Utils/Random.h:35`, `Engine/Utils/Random.h:44`, `Engine/Utils/Random.h:53`, `Engine/Utils/Random.h:61`, `Engine/Utils/Random.h:68`, `Engine/Utils/Random.h:75`

    Duplicate: random generator construction is repeated in every public `Get` overload, and distribution setup is repeated across ranged/non-ranged integer and float helpers.

    Collapse target: share generator construction and use one distribution helper per numeric category.

    Reason: repeated random setup increases maintenance cost and makes it harder to improve generator lifetime later.

13. `Assimp.lua:14`, `Assimp.lua:21`, `Assimp.lua:35`, `Assimp.lua:42`, `YamlCpp.lua:10`, `YamlCpp.lua:14`, `YamlCpp.lua:23`, `YamlCpp.lua:27`, `YamlCpp.lua:38`

    Duplicate: the CMake utility project pattern is repeated for Assimp and yaml-cpp: make build dir, run `cmake -S/-B`, build config, copy outputs through `copy_files.bat`, and declare build outputs.

    Collapse target: add a Premake helper function such as `cmake_utility_project(name, sourceDir, buildDir, options, outputs, copyRules)`.

    Reason: dependency build flow is currently copy/paste with small variations. A helper would make future dependency changes safer.

14. `Engine/Utils/FileSystemUtils.h:9`, `Engine/Utils/FileSystemUtils.h:14`, `Engine/Utils/FileSystemUtils.h:19`, `Engine/Utils/FileSystemUtils.h:24`, `Engine/Utils/FileSystemUtils.h:29`, `Engine/Utils/FileSystemUtils.h:34`, `Engine/Utils/FileSystemUtils.h:39`, `Engine/Utils/FileSystemUtils.h:44`, `Engine/Utils/FileSystemUtils.h:49`

    Duplicate: `FileSystemUtils` is a thin wrapper over `gns::path`.

    Collapse target: gradually replace call sites with `gns::path` and keep or delete the wrapper depending on compatibility needs.

    Reason: AGENTS.md says `FileSystemUtils.h` should remain only a compatibility wrapper and should not regain independent path logic.

15. `Engine/Utils/Path.cpp:145`, `Engine/Utils/Path.cpp:150`, `Engine/Utils/Path.cpp:155`, `Engine/Utils/Path.cpp:160`

    Duplicate: `AssetsDirectory`, `LibraryDirectory`, `PackagesDirectory`, and `CacheDirectory` are exact same-shape functions that call `Resolve(Root::Project, "...")`.

    Collapse target: keep public convenience functions if desired, but back them with a single table/helper for project subdirectories.

    Reason: this was an exact body-pattern match. The current duplication is tiny, but a helper would make future root additions less repetitive.

16. **Done.** `Engine/Renderer/RenderSystem.cpp:82`, `Engine/Renderer/Renderer.cpp:299`, `Engine/Renderer/Vulkan/Device.cpp:69`, `Engine/Renderer/RenderSystem.cpp:264`, `Engine/Renderer/Renderer.cpp:258`, `Engine/Systems/GuiSystem.cpp:75`, `Engine/Renderer/RenderSystem.cpp:269`, `Engine/Renderer/Renderer.cpp:263`, `Engine/Renderer/RenderSystem.cpp:274`, `Engine/Renderer/Renderer.cpp:274`

    Duplicate: several functions are pure pass-through chains for wait, screen, and scene texture descriptor access.

    Collapse target: keep these only where they express a useful layer boundary. Otherwise expose a smaller renderer-facing service to GUI/editor code.

    Reason: pass-through APIs are sometimes valid, but too many of them make ownership harder to see and increase API surface area.

17. **Done.** `Engine/Renderer/Vulkan/PipelineBuilder.cpp:86` and `Engine/Renderer/Vulkan/PipelineBuilder.cpp:97`

    Duplicate: fragment and vertex shader path handling both check for `.spv`, append it, log the path, resolve against `EditorResources`, load the shader module, and log failure.

    Collapse target: extract `LoadShaderStageModule(path, stageName)` or a small shader path normalizer.

    Reason: shader-stage differences are only the destination module and error message.

18. **Done.** `Engine/Systems/GuiSystem.cpp:38`, `Engine/Systems/GuiSystem.cpp:65`, `Engine/Systems/GuiSystem.cpp:77`, `Engine/Systems/GuiSystem.cpp:89`, `Engine/Systems/GuiSystem.cpp:114`, `Engine/Systems/GuiSystem.cpp:156`

    Duplicate: `GuiSystem` repeatedly fetches `RenderSystem`, checks for null, logs a message, and then calls a single render-system function.

    Collapse target: add a private `GetRenderSystemOrWarn(contextMessage)` helper or inject/cache `RenderSystem` during `OnCreate`.

    Reason: repeated system lookup and warning text make the class noisy and inconsistent to maintain.

## Exact-Shape But Semantically Distinct

These were flagged by the exact-body scan but should not be collapsed without a design decision.

1. `Engine/Core/ComponentLibrary.h:27` and `Engine/Core/ComponentLibrary.h:32`

   Duplicate shape: `SceneRootComponent` and `SceneMemberComponent` both contain a hidden read-only `scene_handle`.

   Recommendation: keep separate unless the scene model changes.

   Reason: one marks the root entity and one marks scene membership. The storage shape is identical, but the semantics are not.
