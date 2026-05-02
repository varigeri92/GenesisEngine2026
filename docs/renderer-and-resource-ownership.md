# Renderer and Resource Ownership

GenesisEngine is a Vulkan-only renderer. The most important architectural rule is the boundary between engine resources and Vulkan resources.

## Ownership Boundary

```mermaid
flowchart LR
    A["Engine object handle"] --> B["RenderSystem resource cache"]
    B --> C["Vulkan resource handle"]
    C --> D["Device resource registry"]
```

Engine objects own engine data:

- `Mesh`
- `Texture`
- `Material`
- `Shader`

Vulkan objects own backend data:

- `VulkanMesh`
- `VulkanTexture`
- `VulkanShader`
- `VulkanImage`
- `VulkanBuffer`

`RenderSystem` is the bridge. It is the only layer that should know both engine object handles and Vulkan resource handles.

## RenderSystem

`Engine/Renderer/RenderSystem.h` contains `RenderResourceCache`:

```cpp
struct RenderResourceCache
{
    std::unordered_map<Handle, Handle> meshes;
    std::unordered_map<Handle, Handle> shaders;
    std::unordered_map<Handle, Handle> textures;
    std::unordered_map<Handle, Handle> materials;
};
```

Each map uses an engine object handle as the key and a renderer/Vulkan handle as the value.

The public apply functions are:

- `ApplyMesh(Mesh& mesh)`
- `ApplyShader(Shader& shader)`
- `ApplyTexture(Texture& texture)`
- `ApplyMaterial(Material& material)`

These functions check the cache first and only create backend resources if the engine resource has not already been applied.

## Renderer

`Engine/Renderer/Renderer.h` is a facade over:

- `Device`
- `RenderGraph`
- Camera state
- Scene/screen texture access

It owns render pass setup and frame drawing. It creates Vulkan resources by delegating to `Device`.

Current render pass order:

1. Transition draw image to general.
2. Draw background.
3. Transition draw image and depth image for attachments.
4. Draw geometry.
5. Either copy scene to swapchain or prepare the draw image for shader read.
6. Clear swapchain.
7. Draw ImGui.
8. Present.

## RenderGraph

`RenderGraph` is a lightweight ordered list of named render steps. Each step receives:

- `VkCommandBuffer`
- mutable `RenderStepData`
- per-frame `FrameData`

It currently executes steps linearly in insertion order.

## Device

`Device` owns Vulkan initialization and lifetime:

- Vulkan instance, debug messenger, surface, physical device, logical device
- VMA allocator
- swapchain
- frame command pools and command buffers
- synchronization objects
- descriptor allocators and layouts
- draw/depth images
- default textures
- immediate-submit command objects
- `VulkanResourceRegistry`

It is the lowest engine-owned layer that should directly create and destroy Vulkan resources.

## Draw Data Flow

```mermaid
flowchart TD
    A["ECS view: EntityComponent + Transform + MeshComponent"] --> B["RenderSystem::BuildDrawData"]
    B --> C["Resolve shader and mesh through resource cache"]
    C --> D["Resolve material and albedo texture"]
    D --> E["Create DrawData"]
    E --> F["Renderer::DrawFrame"]
    F --> G["Device::DrawMesh"]
```

`DrawData` carries the Vulkan objects and descriptor handles required by the backend draw call.

## Default Textures

The renderer creates Vulkan default textures. `RenderSystem` registers engine-side `Texture` objects for them and stores mappings to the Vulkan texture handles.

Default texture handles:

- White
- Grey
- Black
- Error checkerboard

## Important Constraints

- Do not add Vulkan handles to engine-side `Mesh`, `Texture`, `Material`, or `Shader`.
- Keep engine resource handle to Vulkan resource handle mapping in `RenderSystem` or a renderer-facing cache.
- Keep `Device` responsible for Vulkan object lifetime.
- Keep renderer development Vulkan-only. Do not introduce a generic multi-backend abstraction unless the project direction changes.
