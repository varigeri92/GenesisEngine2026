# Render Resource Refactor Plan

This note records the renderer/resource design direction referenced by `D:\ProjectGenesis\AGENTS.md`.

## Current Rule

GenesisEngine is Vulkan-only, but engine-side resources should not store Vulkan objects or Vulkan resource handles.

The intended ownership path is:

```text
Engine resource handle -> RenderSystem cache -> Vulkan resource handle
```

The left-hand handle belongs to an engine object such as `Mesh`, `Texture`, `Material`, or `Shader`.

The right-hand handle belongs to a backend resource owned by the Vulkan renderer/device layer, such as `VulkanMesh`, `VulkanTexture`, or `VulkanShader`.

## Current Implementation Marker

`RenderSystem` currently owns `RenderResourceCache`:

```cpp
struct RenderResourceCache
{
    std::unordered_map<Handle, Handle> meshes;
    std::unordered_map<Handle, Handle> shaders;
    std::unordered_map<Handle, Handle> textures;
    std::unordered_map<Handle, Handle> materials;
};
```

This cache is the bridge between engine resource identity and renderer resource identity.

## Refactor Priority

When continuing renderer, material, or texture work, prefer this order:

1. Fix resource ownership and lifetime semantics.
2. Move all engine-to-Vulkan resource mapping into a render resource cache or renderer-facing bridge.
3. Clean up `VulkanResource` create/destroy/free behavior.
4. Keep engine-side `Texture` and `Material` types free of Vulkan fields.
5. Keep backend-side `VulkanTexture` and `VulkanMaterial` types responsible for Vulkan fields.
6. Refactor render pass flow after the resource model is stable.

## Do Not Add

Avoid engine-side backend handles such as:

```cpp
struct Mesh {
    Handle vulkanMeshHandle;
};

struct Shader {
    Handle m_vulkanShaderHandle;
};
```

That creates ownership ambiguity and makes asset lifetime harder to reason about.

## Preferred Direction

Keep this shape:

```cpp
struct RenderResourceCache {
    std::unordered_map<Handle, Handle> meshes;
    std::unordered_map<Handle, Handle> shaders;
    std::unordered_map<Handle, Handle> textures;
    std::unordered_map<Handle, Handle> materials;
};
```

Then let `RenderSystem` decide when to create, reuse, and eventually release renderer resources.

## Known In-Progress Areas

- Material handling currently maps material handles to themselves in `RenderSystem::ApplyMaterial`; a dedicated backend `VulkanMaterial` resource is still a future direction.
- Texture upload exists and defaults are registered through engine-side texture objects.
- Render pass ordering works, but ownership and lifetime should be stabilized before deeper render-graph refactors.
- Scene rendering currently builds draw data from the global ECS registry and editor camera state.
