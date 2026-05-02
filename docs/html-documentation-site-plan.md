# HTML Documentation Site Plan

These Markdown files are structured so they can be converted into a web documentation site later.

## Recommended Site Structure

```text
docs/
  README.md
  project-overview.md
  build-and-workspace.md
  runtime-architecture.md
  renderer-and-resource-ownership.md
  assets-and-resources.md
  editor-application.md
  public-api-surface.md
  render-resource-refactor-plan.md
  html-documentation-site-plan.md
```

For a generated site, map `README.md` to the landing page and expose the remaining files as sidebar pages.

## Static Site Options

Good fits for this repository:

- MkDocs Material
- Docusaurus
- VitePress
- mdBook

MkDocs Material is the simplest option if the goal is a clean documentation site from Markdown with a sidebar and search.

## Suggested MkDocs Layout

Add a future `mkdocs.yml` at the repository root:

```yaml
site_name: GenesisEngine Documentation
docs_dir: docs
nav:
  - Home: README.md
  - Project Overview: project-overview.md
  - Build and Workspace: build-and-workspace.md
  - Runtime Architecture: runtime-architecture.md
  - Renderer and Resource Ownership: renderer-and-resource-ownership.md
  - Assets and Resources: assets-and-resources.md
  - Editor Application: editor-application.md
  - Public API Surface: public-api-surface.md
  - Render Resource Refactor Plan: render-resource-refactor-plan.md
  - HTML Documentation Site Plan: html-documentation-site-plan.md
theme:
  name: material
  features:
    - navigation.sections
    - navigation.top
    - search.highlight
markdown_extensions:
  - admonition
  - pymdownx.superfences
```

This file is not added yet because the current request is for Markdown documentation, not a site generator setup.

## Documentation Maintenance Rules

- Keep architecture pages focused on stable boundaries.
- Keep implementation details near the modules they describe.
- When renderer ownership changes, update `renderer-and-resource-ownership.md` and `render-resource-refactor-plan.md` together.
- When adding new public headers under `Engine/API/`, update `public-api-surface.md`.
- When adding editor windows or systems, update `editor-application.md`.
