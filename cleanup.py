#!/usr/bin/env python3
"""Delete generated asset files from the project Assets directory."""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Iterable


FILE_GROUPS = {
    "meta": ("*.meta",),
    "scene": ("*.gnsscene",),
    "material": ("*.gnsmaterial",),
}


def collect_files(assets_dir: Path, groups: Iterable[str]) -> list[Path]:
    files: set[Path] = set()
    for group in groups:
        for pattern in FILE_GROUPS[group]:
            files.update(path for path in assets_dir.rglob(pattern) if path.is_file())
    return sorted(files, key=lambda path: path.as_posix().lower())


def collect_artifacts(assets_dir: Path) -> list[Path]:
    artifacts_dir = assets_dir.parent / "Library" / "Artifacts"
    if not artifacts_dir.is_dir():
        return []

    return sorted(
        (path for path in artifacts_dir.rglob("*") if path.is_file()),
        key=lambda path: path.as_posix().lower(),
    )


def delete_files(files: Iterable[Path], dry_run: bool) -> int:
    count = 0
    for file_path in files:
        print(file_path)
        if not dry_run:
            file_path.unlink()
        count += 1
    return count


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Clean generated .meta, .gnsscene, and .gnsmaterial files from Assets."
    )
    parser.add_argument(
        "-assets",
        default="Assets",
        help="Assets directory to clean. Defaults to ./Assets.",
    )
    parser.add_argument(
        "-meta",
        action="store_true",
        help="Delete .meta files.",
    )
    parser.add_argument(
        "-scene",
        action="store_true",
        help="Delete .gnsscene files.",
    )
    parser.add_argument(
        "-material",
        action="store_true",
        help="Delete .gnsmaterial files.",
    )
    parser.add_argument(
        "-all",
        action="store_true",
        help="Delete all supported generated asset files.",
    )
    parser.add_argument(
        "-dry",
        action="store_true",
        help="Only print what would be deleted.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    selected_groups: list[str] = []
    if args.all:
        selected_groups = list(FILE_GROUPS.keys())
    else:
        if args.meta:
            selected_groups.append("meta")
        if args.scene:
            selected_groups.append("scene")
        if args.material:
            selected_groups.append("material")

    if not selected_groups:
        print("Nothing selected. Use -meta, -scene, -material, or -all.")
        return 2

    assets_dir = Path(args.assets).resolve()
    if not assets_dir.exists():
        print(f"Assets directory does not exist: {assets_dir}")
        return 1
    if not assets_dir.is_dir():
        print(f"Assets path is not a directory: {assets_dir}")
        return 1

    files = collect_files(assets_dir, selected_groups)
    artifact_files = collect_artifacts(assets_dir) if "meta" in selected_groups else []
    action = "Would delete" if args.dry else "Deleting"
    print(f"{action} {len(files)} file(s) from {assets_dir}")

    deleted_files = delete_files(files, args.dry)

    if "meta" in selected_groups:
        artifacts_dir = assets_dir.parent / "Library" / "Artifacts"
        print(f"{action} {len(artifact_files)} artifact file(s) from {artifacts_dir}")
        deleted_artifacts = delete_files(artifact_files, args.dry)
        artifact_action = "Would remove" if args.dry else "Removed"
        print(f"{artifact_action} {deleted_artifacts} artifact file(s).")

    file_action = "Would remove" if args.dry else "Removed"
    print(f"{file_action} {deleted_files} generated asset file(s).")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
