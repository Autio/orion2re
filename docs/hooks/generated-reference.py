import os
from pathlib import Path
import re
import subprocess

import mkdocs_gen_files


build_directory = os.environ.get("ORION2RE_CONFIG_DOCS_BUILD_DIR")
if build_directory:
    subprocess.run(
        ["cmake", "--build", build_directory, "--target", "config-docs"],
        check=True,
    )

source = os.environ.get("ORION2RE_GENERATED_CONFIG_DOCS")
if not source:
    raise RuntimeError(
        "ORION2RE_GENERATED_CONFIG_DOCS is not set; build the config-docs target first"
    )

source_directory = Path(source)
if not source_directory.is_dir():
    raise RuntimeError(f"generated configuration reference not found: {source_directory}")


def player_reference(markdown: str, filename: str) -> str:
    if filename == "index.md":
        markdown = markdown.replace(
            "The exact paths and built-in defaults are generated from the runtime schema.",
            "Each page lists the setting names used in the configuration file, what they affect, their accepted values, and their built-in defaults.",
        )
    elif filename == "allowed-values.md":
        markdown = markdown.replace(
            "Names accepted by settings with enumerated, mask, or bitmap values.",
            "Named choices accepted by settings. Use the spelling shown here when a setting links to one of these lists; the Used by links lead back to the affected settings.",
        )
    elif filename == "record-layouts.md":
        markdown = markdown.replace(
            "Named field layouts used by structured configuration settings.",
            "Some settings contain several named fields inside one block. Each layout below explains those field names and links back to the settings that use them.",
        )

    markdown = re.sub(
        r"^- \*\*(?:Persistence|Multiplayer):\*\*.*\n",
        "",
        markdown,
        flags=re.MULTILINE,
    )
    return markdown


for source_file in sorted(source_directory.glob("*.md")):
    destination = Path("generated/configuration") / source_file.name
    with mkdocs_gen_files.open(destination, "w") as output:
        markdown = source_file.read_text(encoding="utf-8")
        output.write(player_reference(markdown, source_file.name))