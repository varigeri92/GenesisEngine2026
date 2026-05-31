# GenesisEngine Documentation

GenesisEngine is a Windows-focused C++20 Vulkan engine and editor workspace. The codebase is organized around an engine shared library, an editor executable, and a small Premake/CMake helper project for Assimp.

## Start Here

- [Project Overview](project-overview.md)
- [Build and Workspace](build-and-workspace.md)
- [Runtime Architecture](runtime-architecture.md)
- [Renderer and Resource Ownership](renderer-and-resource-ownership.md)
- [Assets and Resources](assets-and-resources.md)
- [Editor Application](editor-application.md)
- [Public API Surface](public-api-surface.md)
- [HTML Documentation Site Plan](html-documentation-site-plan.md)
- [Render Resource Refactor Plan](render-resource-refactor-plan.md)

## Main Projects

| Project | Type | Purpose |
| --- | --- | --- |
| `Engine` | Shared library | Core runtime, ECS, systems, Vulkan renderer, assets, windowing, GUI backend, and public API headers. |
| `Editor` | Console application | Launches the engine with editor systems and ImGui windows. |

## Current Shape

The engine currently boots a global systems manager, creates an SDL/Vulkan window, initializes a Vulkan renderer, starts ImGui, loads a hard-coded sample asset during render-system startup, and displays the rendered scene texture inside an ImGui scene view.

The renderer is Vulkan-only. Engine-side objects such as `Mesh`, `Texture`, `Material`, and `Shader` should stay free of Vulkan handles. `RenderSystem` is the bridge between engine handles and renderer-owned Vulkan resource handles.

## Documentation Conventions

- Markdown files are kept flat under `docs/` for simple static-site ingestion.
- Code paths are written relative to the repository root.
- "Current behavior" documents what the code does today.
- "Important constraints" calls out rules that future implementation should preserve.
