# Duplicated Code Findings

Scope: first-party project files under `Engine`, `Editor`, and root Premake Lua files. External/vendor/generated files are excluded. This page tracks current duplication worth considering later; it is not a mandate to refactor during documentation-only work.

## Duplicates To Consider

1. `Engine/Systems/SystemsManager.h`

   Duplicate: the no-argument and variadic `RegisterSystem` templates both allocate a system, set its state, assign metadata, and return the typed pointer.

   Collapse target: keep one constrained variadic template that also handles zero constructor arguments.

2. `Engine/Core/Entity.h`

   Duplicate: component add/get/ensure helpers have overlapping registry access patterns.

   Collapse target: keep the public API ergonomic, but centralize the repeated registry interaction if these helpers grow further.

3. `Engine/Renderer/RenderSystem.cpp`

   Duplicate: mesh, shader, texture, and material upload paths share the same "check cache, avoid duplicate pending upload, enqueue, flush, cache result" shape.

   Collapse target: introduce small private cache/upload helpers only if they reduce noise without hiding resource-specific lifetime behavior.

4. `Engine/Renderer/RenderSystem.cpp`

   Duplicate: `GetRenderMeshHandle`, `GetRenderShaderHandle`, and `GetRenderMaterialHandle` perform similar cache lookup, warning, and invalid-handle return behavior.

   Collapse target: use a private helper such as `FindCachedRenderHandle(cache, handle, missingMessage)`.

5. `Engine/Renderer/Renderer.cpp` and `Engine/Renderer/Vulkan/Device.cpp`

   Duplicate: regular texture creation and default texture creation follow similar upload, sampler, image-view, descriptor, and resource-registration paths.

   Collapse target: keep texture creation centralized in `Device` so default textures and imported textures share lifetime and descriptor behavior.

6. `Engine/Renderer/Vulkan/DescriptorLayoutBuilder.cpp`

   Duplicate: growable and fixed descriptor allocators repeat Vulkan descriptor pool size setup and descriptor-set allocation setup.

   Collapse target: share helpers for pool-size construction, pool create info, and descriptor set allocate info while preserving allocator policy differences.

7. `Engine/Input/InputBackend.cpp` and `Engine/Window/Window.cpp`

   Duplicate: borderless-window constants and edge classification are split across input and window code.

   Collapse target: centralize borderless-window metrics and edge detection in the window layer.

8. `Editor/EditorGUI/EditorWidgets.cpp` and `Editor/EditorGUI/Windows/InspectorWindow.cpp`

   Duplicate: float/vector widget wrappers and reflected inspector field drawing repeat similar `DragFloat*`, scalar, and table-layout code.

   Collapse target: centralize numeric/vector field widgets so reflected fields and hand-authored widgets stay visually consistent.

9. `Engine/Utils/Random.h`

   Duplicate: random generator construction and distribution setup are repeated across public `Get` overloads.

   Collapse target: share generator construction and use one distribution helper per numeric category.

10. `Assimp.lua` and `YamlCpp.lua`

    Duplicate: both utility projects follow the same CMake build/copy pattern.

    Collapse target: add a Premake helper such as `cmake_utility_project(name, sourceDir, buildDir, options, outputs, copyRules)`.

11. `Engine/Utils/FileSystemUtils.h`

    Duplicate: `FileSystemUtils` is a compatibility wrapper over `gns::path`.

    Collapse target: gradually replace call sites with `gns::path` and keep the wrapper thin if compatibility still needs it.

12. `Engine/Utils/Path.cpp`

    Duplicate: project subdirectory helpers such as `AssetsDirectory`, `LibraryDirectory`, `PackagesDirectory`, and `CacheDirectory` share the same `Resolve(Root::Project, "...")` shape.

    Collapse target: a small table/helper could reduce repetition if more roots are added.

## Semantically Distinct

- `SceneRootComponent` and `SceneMemberComponent` have similar storage, but different meaning. Keep them separate unless the scene model changes.
- Runtime systems and GUI windows both have registration APIs, but they are different lifecycles. Do not collapse `SystemsManager::RegisterSystem` and `GuiSystem::RegisterWindow`.
