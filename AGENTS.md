# AGENTS.md

Guidance for Codex or other coding agents working on GenesisEngine.

## Project Context

GenesisEngine is a Windows-focused C++20 Vulkan engine/editor project.

The project uses:

- Premake for project generation.
- MSVC / Visual Studio locally.
- Vulkan SDK.
- SDL2, ImGui, Assimp, fmt, EnTT, vk-bootstrap.

Generated Visual Studio files may not be present in Codex worktrees. Build and runtime
validation usually happen in the user's main local checkout, not inside the Codex
worktree.

## Workspace Layout

Codex may work in a separate worktree like:

```text
C:\Users\varig\.codex\worktrees\f833\GenesisEngine
```

The user's main local project is expected to be:

```text
D:\ProjectGenesis\GenesisEngine
```

Do not assume edits in the Codex worktree are automatically present in the user's main
checkout.

## Preferred Collaboration Workflow

Use patch-based transfer instead of full-file copying when possible.

1. Codex edits the separate worktree.
2. Codex provides or exports a patch.
3. The user applies the patch in `D:\ProjectGenesis\GenesisEngine`.
4. The user builds and runs locally.
5. The user sends back compiler, runtime, or Vulkan validation output.
6. Codex iterates from that feedback.

Patch export example:

```powershell
cd C:\Users\varig\.codex\worktrees\f833\GenesisEngine
git diff -- <changed-files> > D:\ProjectGenesis\codex.patch
```

Patch apply example:

```powershell
cd D:\ProjectGenesis\GenesisEngine
git apply --check D:\ProjectGenesis\codex.patch
git apply D:\ProjectGenesis\codex.patch
```

If files have diverged between the Codex worktree and the main checkout, inspect and
resolve the patch manually.

## Build Notes

The build is path- and machine-specific.

Expect local requirements such as:

- `VULKAN_SDK` environment variable.
- Vendor libraries already built in expected locations.
- Visual Studio / MSVC installed and available locally.
- Premake-generated `.sln` and `.vcxproj` files.

Codex should not claim a build passed unless it actually ran the build in an environment
with the required toolchain.

## Architecture Rule: Resource Ownership

The project is Vulkan-only. Do not design for DirectX, Metal, OpenGL, or a generic
multi-backend renderer.

Still keep a strict boundary:

```text
Engine
  Owns engine resources, assets, entities, and components.
  Must not know about Vulkan objects or Vulkan resource handles.

RenderSystem
  Bridges engine state to Vulkan renderer state.
  Owns mappings between engine resource handles and Vulkan resource handles.

Vulkan Renderer / Device
  Owns Vulkan resources and command recording.
  Knows about VkBuffer, VkImage, VkDescriptorSet, VkPipeline, synchronization, etc.
```

Main rule:

```text
Engine resource handle -> RenderSystem cache -> Vulkan resource handle
```

The RenderSystem is the only layer that should know both sides.

Avoid storing backend handles in engine-side resource classes:

```cpp
struct Mesh {
    Handle vulkanMeshHandle; // avoid
};

struct Shader {
    Handle m_vulkanShaderHandle; // avoid
};
```

Prefer a render resource cache owned by `RenderSystem` or a renderer-facing bridge:

```cpp
struct RenderResourceCache {
    std::unordered_map<Handle, Handle> meshes;
    std::unordered_map<Handle, Handle> shaders;
    std::unordered_map<Handle, Handle> textures;
    std::unordered_map<Handle, Handle> materials;
};
```

The left-hand handle is an engine resource handle. The right-hand handle is a
`VulkanResource` handle.


## Architecture Rule: Path Ownership

All runtime/editor path state is owned by the engine path API in `Engine/Utils/Path.h` (`gns::path`). Do not add new ad hoc path managers, rooted-path globals, or local copies of project/resource roots in editor systems, renderer systems, asset code, or UI windows.

Use logical roots and central helpers instead of raw rooted paths:

```text
gns::path::Root::Project
gns::path::Root::ProjectAssets
gns::path::Root::ProjectLibrary
gns::path::Root::ProjectPackages
gns::path::Root::ProjectCache
gns::path::Root::EditorResources
```

Preferred pattern:

```cpp
gns::path::Resolve(gns::path::Root::ProjectAssets, "Models/example.gltf");
gns::path::Resolve(gns::path::Root::EditorResources, "Shaders/default.frag");
```

Avoid direct `std::filesystem` helper duplication for normalization, existence checks, extension checks, relative paths, or root checks. Add or use `gns::path` helpers instead. `Engine/Utils/FileSystemUtils.h` is only a compatibility wrapper over `gns::path` and should not regain independent logic.

Project and metadata files should persist project-relative paths, not rooted machine-local paths. Rooted paths are allowed only at process boundaries such as CLI arguments (`-p`, `-r`) and inside the configured `gns::path` state.

## Refactor Priority

When continuing the renderer/material/texture work, prefer this order:

1. Fix resource ownership and lifetime semantics.
2. Move engine-to-Vulkan resource mapping into a render resource cache.
3. Clean up `VulkanResource` create/destroy/free behavior.
4. Add engine-side `Texture` and `Material` types without Vulkan fields.
5. Add backend-side `VulkanTexture` and `VulkanMaterial` types.
6. Refactor the render pass flow after the resource model is stable.

The current render pass flow is rough but usable. Resource ownership is the more urgent
problem because textures and materials will multiply lifetime issues.

## Design Notes

The current renderer/resource design marker is:

```text
docs/render-resource-refactor-plan.md
```

Read that note before continuing renderer, texture, material, or Vulkan resource
refactor work.

