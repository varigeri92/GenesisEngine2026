# Assets and Resources

The current asset path is centered on Assimp for model import and STB Image for texture decoding.

## AssetManager

`Engine/Assets/AssetManager.cpp` implements `AssetManager::LoadAsset`.

Current behavior:

1. Assimp imports the file with tangent calculation, triangulation, joined vertices, and primitive sorting.
2. Materials are read first when present.
3. Material base color is read from `AI_MATKEY_BASE_COLOR` or diffuse color.
4. Albedo textures are loaded from base-color or diffuse texture slots.
5. Textures can be loaded from external files or embedded Assimp textures.
6. Meshes are converted to engine-side `Mesh` objects.
7. Each returned `LoadedObject` contains the mesh object, mesh handle, and optional material handle.

## Texture Loading

Texture files are loaded into RGBA8 memory:

- External textures are resolved relative to the imported asset directory.
- Embedded compressed textures are decoded with `stbi_load_from_memory`.
- Embedded raw texels are copied into RGBA memory.
- Loaded textures are cached by normalized path or embedded texture object name for the duration of a single `LoadAsset` call.

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

## Runtime Resources

The `Resources/` folder contains:

- `Resources/Shaders/`: GLSL shader sources, SPIR-V outputs, and `Compile.bat`.
- `Resources/Fonts/`: Geist text font and Material Icons font variants.

The path utility resolves resources through `gns::path::SetResourcesDirectory`, which is called during `Engine::Initialize`.

## Shader Resources

Render startup currently creates a default mesh shader from:

- `Resources/Shaders/default.frag`
- `Resources/Shaders/mesh.vert`

The shader object stores source paths. `Renderer::CreateVulkanShader` builds the Vulkan shader resource, descriptor set layout, pipeline layout, and graphics pipeline.

## CPU-Side Freeing

After successful upload:

- `Mesh::Apply` can clear CPU-side mesh arrays.
- `RenderSystem::ApplyTexture` frees CPU-side texture pixels.

Use `cpuReadWrite = true` when applying a mesh if CPU-side mesh data must remain available after upload.
