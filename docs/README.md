# GenesisEngine Documentation

GenesisEngine is a Windows-focused C++20 Vulkan engine and editor workspace. The codebase is organized around an engine shared library, an editor executable, Premake-generated Visual Studio projects, and utility dependency-build projects.

## Start Here

- [Project Overview](project-overview.md)
- [Build and Workspace](build-and-workspace.md)
- [Runtime Architecture](runtime-architecture.md)
- [Renderer and Resource Ownership](renderer-and-resource-ownership.md)
- [Assets and Resources](assets-and-resources.md)
- [Editor Application](editor-application.md)
- [Public API Surface](public-api-surface.md)
- [Known Architecture Violations](violations.md)

## Main Projects

| Project | Type | Purpose |
| --- | --- | --- |
| `Engine` | Shared library | Core runtime, ECS, systems, Vulkan renderer, assets, windowing, GUI backend, and public API headers. |
| `Editor` | Console application | Launches the engine, configures project/resource roots, registers editor systems, and creates the ImGui editor workspace. |

## Current Shape

The engine currently boots a global systems manager, configures the central path API, initializes the job system and component reflection, registers core systems, creates an SDL/Vulkan window when not headless, starts a render thread, initializes ImGui, and displays the rendered scene texture inside an ImGui scene view.

The renderer is Vulkan-only. Engine-side objects such as `Mesh`, `Texture`, `Material`, and `Shader` must stay free of Vulkan handles. `RenderSystem` is the public-facing render system and bridge between engine handles and renderer-owned Vulkan resource handles.

Only classes derived from `gns::core::System` and registered through `SystemsManager::RegisterSystem` are runtime systems. Helper classes with `System` in their name, such as `SystemsManager` or `JobSystem`, are not systems in this lifecycle sense.

## Documentation Conventions

- Markdown files are kept flat under `docs/` for simple static-site ingestion.
- Code paths are written relative to the repository root.
- "Current behavior" documents what the code does today.
- "Important constraints" calls out rules that future implementation should preserve.
- Known rule breaks found during documentation audits are recorded in `violations.md` instead of being fixed during docs-only work.
