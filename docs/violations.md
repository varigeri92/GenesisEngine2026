# Known Architecture Violations

This file records rule violations found during documentation audits. These are not fixed in docs-only passes.

## Active Violations

1. `Engine/Utils/Path.h:10`

   Violation: `DefaultProjectRoot` is a compiled-in machine-local rooted path: `D:\ProjectGenesis\TestProject\`.

   Rule: rooted machine-local project paths should enter through process boundaries such as command-line arguments or configured path state, not through hard-coded defaults.

   Suggested direction: replace the hard-coded default with project discovery, a project picker, or an explicit required `-p` / `--project` argument for editor startup.

2. `Editor/Assets/AssetMetadataWriter.cpp:23`, `Editor/Assets/AssetMetadataWriter.cpp:406`

   Violation: editor asset metadata generation includes `Engine/Renderer/Vulkan/ShaderUtils.h` and uses Vulkan/SPIR-V reflection helpers to build the default material layout.

   Rule: renderer/Vulkan backend details should stay behind `RenderSystem` or renderer-facing bridge APIs. Editor asset code should not directly depend on Vulkan backend helpers.

   Suggested direction: expose a renderer/system-level material-layout query or move shader reflection needed for asset metadata into a backend-neutral asset/material service.

3. `Editor/Assets/AssetMetadataWriter.cpp:422`, `Editor/Assets/AssetMetadataWriter.cpp:549`, `Engine/Profiling/Profiler.cpp:172`, `Engine/Profiling/Profiler.cpp:288`

   Violation: directory creation is performed directly with `std::filesystem::create_directories` outside `gns::path`.

   Rule: project/editor path operations should be centralized in `gns::path`; avoid reintroducing ad hoc filesystem helper logic in editor, asset, renderer, or UI modules.

   Suggested direction: add a small `gns::path::CreateDirectories` helper and route these call sites through it.

4. `Engine/Object/Mesh.h:40`

   Violation: engine-side `Mesh` grants `RenderSystem` friendship so the render system can call the private CPU-side free method.

   Rule: engine resources should stay backend-neutral and as self-contained as possible. They should not need special render-system access for lifetime transitions.

   Suggested direction: make CPU-side retention/freeing an explicit resource policy handled by an asset/resource bridge, or expose a narrow backend-neutral method if this ownership is intentional.

## Not Violations

- `SystemsManager`, `JobSystem`, and `SystemViewer` are not runtime systems despite their names. They do not derive from `gns::core::System` and are not registered through `SystemsManager::RegisterSystem`.
- Engine resource classes currently avoid storing Vulkan handles directly. The current handle bridge remains `Engine resource handle -> RenderSystem cache -> Vulkan resource handle`.
