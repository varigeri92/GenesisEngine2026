# Assets and Resources

The current asset path is centered on `AssetSystem`, `AssetManager`, `AssetLoader`, Assimp model import, STB image decoding, YAML metadata, and project-library artifact links.

`AssetSystem` is the registered runtime system. `AssetManager` and `AssetLoader` are helper/backend classes, not systems in the lifecycle sense.

## AssetManager

`Engine/Assets/AssetManager.cpp` implements synchronous load/commit helpers and metadata-aware asset resolution. `Engine/Assets/AssetSystem.cpp` wraps that behavior in a registered system with request tracking and background job scheduling.

Synchronous model load behavior:

1. Assimp imports the file with tangent calculation, triangulation, joined vertices, and primitive sorting.
2. Materials are read first when present.
3. Material base color is read from `AI_MATKEY_BASE_COLOR` or diffuse color.
4. Albedo textures are loaded from base-color or diffuse texture slots.
5. Textures can be loaded from external files or embedded Assimp textures.
6. Mesh, material, and texture descriptions are committed into engine-side `Object` instances.
7. Each returned `LoadedObject` contains an object pointer, object handle, optional material handle, and source transform data.

## AssetSystem

`AssetSystem` derives from `gns::core::System` and is registered during engine initialization. It provides:

- `RequestAsset(path, options)` for async source-file loads.
- `RequestAsset(handle)` and `QueueAsset(handle)` for handle-driven lazy loads through artifact metadata.
- `Flush()` during `OnUpdate` to collect completed jobs and finish requests.
- `EnsureMeshLoaded`, `EnsureMaterialLoaded`, and `EnsureTextureLoaded` forwarding to `AssetManager`.

In-flight requests are batched by normalized source path and load options so multiple callers can share one background load.

## Texture Loading

Texture files are loaded into RGBA8 memory:

- External textures are resolved relative to the imported asset directory.
- Embedded compressed textures are decoded with `stbi_load_from_memory`.
- Embedded raw texels are copied into RGBA memory.
- Loaded textures are cached by normalized path or embedded texture object name for the duration of a single source load.

If a material texture slot exists but loading fails, the material uses the engine default error checkerboard texture handle.

## Mesh Loading

Imported meshes currently populate:

- positions
- normals
- colors
- uvs
- tangents
- bitangents
- indices
- buffer range

If normals are missing, the loader supplies a default up normal and white color. If UVs are missing, it supplies `(0, 0)`.

## Project Metadata and Artifacts

Imported model metadata uses project-relative paths where possible. `AssetMetadataWriter` writes model `.meta` data, material files, and artifact links under the project library. `AssetManager` can resolve handles back to source metadata by reading library artifact links and scanning project assets.

Project roots and project subdirectories must come from `gns::path`; they should not be duplicated in editor windows or asset code.

## Runtime Resources

The `Resources/` folder contains:

- `Resources/Shaders/`: GLSL shader sources, SPIR-V outputs, and `Compile.bat`.
- `Resources/Fonts/`: Geist text font and Material Icons font variants.
- `Resources/Defaults/`: default model assets.

The path utility resolves editor resources through `gns::path::Configure` and `gns::path::Root::EditorResources`. The editor accepts `-r` / `--resources`; otherwise the engine discovers or falls back to a `Resources` directory.

## Shader Resources

`RenderSystem::EnsureDefaultMeshResources` creates a default mesh shader from:

- `Resources/Shaders/default.frag`
- `Resources/Shaders/mesh.vert`

The shader object stores source paths. `Renderer::CreateVulkanShader` reflects compiled SPIR-V, validates descriptor rules, builds material layout metadata, descriptor set layouts, push constant ranges, pipeline layout, and graphics pipeline.

## CPU-Side Freeing

After successful render-thread upload:

- `RenderSystem` clears CPU-side mesh arrays through `Mesh::FreeCPUSide`.
- `RenderSystem` frees CPU-side texture pixels through `Texture::FreeCPUSide`.

There is no current public `cpuReadWrite` apply option. If CPU-side mesh or texture data must stay resident, the upload/free policy needs to be changed before relying on that behavior.
