#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path


def _normalize_input(raw_name: str) -> str:
    name = raw_name.strip().replace("\\", "/")
    if not name:
        raise ValueError("Usage: create_cpp_files.py <file_name>")

    if name.startswith("./"):
        name = name[2:]

    for suffix in (".test.cpp", ".cpp", ".hpp", ".h"):
        if name.endswith(suffix):
            name = name[: -len(suffix)]
            break

    if not name.startswith("src/"):
        name = f"src/{name}"

    # If only a bare file name is given, default to src/leveldb.
    if name.count("/") < 2:
        tail = name.removeprefix("src/")
        name = f"src/{tail}"

    return name


def _sanitize_namespace(value: str) -> str:
    namespace_name = re.sub(r"[^a-zA-Z0-9_]", "_", value)
    if namespace_name and namespace_name[0].isdigit():
        namespace_name = f"ns_{namespace_name}"
    return namespace_name or "crispy"


def _build_include_guard(name: str) -> str:
    suffix = name.removeprefix("src/")
    normalized = re.sub(r"[^a-zA-Z0-9]", "_", suffix).upper()
    return f"CRISPY_{normalized}_H"


def _write_file(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8", newline="\n")


def main() -> int:
    if len(sys.argv) < 2:
        print("Usage: create_cpp_files.py <file_name>")
        print("Example: template/tmp")
        return 1

    try:
        name = _normalize_input(sys.argv[1])
    except ValueError as error:
        print(error)
        return 1

    dir_path = Path(name).parent
    base_name = Path(name).name

    header_file = dir_path / f"{base_name}.h"
    source_file = dir_path / f"{base_name}.cpp"
    test_file = dir_path / f"{base_name}.test.cpp"

    for file_path in (header_file, source_file, test_file):
        if file_path.exists():
            print(f"Error: file already exists: {file_path.as_posix()}")
            return 1

    dir_path.mkdir(parents=True, exist_ok=True)

    header_include = f"{name.removeprefix('src/')}.h"

    src_parts = Path(name).parts
    namespace_name = "crispy"
    if len(src_parts) > 1 and src_parts[0] == "src":
        namespace_name = _sanitize_namespace(src_parts[1])

    include_guard = _build_include_guard(name)

    header_content = (
        f"#ifndef {include_guard}\n"
        f"#define {include_guard}\n\n"
        f"namespace {namespace_name} {{\n\n"
        f"}} // namespace {namespace_name}\n\n"
        f"#endif // {include_guard}\n"
    )

    source_content = (
        f"#include \"{header_include}\"\n\n"
        f"namespace {namespace_name} {{\n\n"
        f"}} // namespace {namespace_name}\n"
    )

    test_content = (
        "#include <catch2/catch_test_macros.hpp>\n\n"
        f"#include \"{header_include}\"\n\n"
        f'TEST_CASE("{base_name} basic behavior", "[{base_name}]")\n'
        "{\n"
        "    CHECK(true);\n"
        "}\n"
    )

    _write_file(header_file, header_content)
    _write_file(source_file, source_content)
    _write_file(test_file, test_content)

    print("Created files:")
    print(f"- {header_file.as_posix()}")
    print(f"- {source_file.as_posix()}")
    print(f"- {test_file.as_posix()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
