from pathlib import Path
import json
from datetime import datetime

from SCons.Script import Import

Import("env")

project_dir = Path(env["PROJECT_DIR"])
state_path = project_dir / ".version_state.json"
header_path = project_dir / "src" / "version_auto.h"

# Load version state (single source of truth)
version_state = {"major": 1, "minor": 3, "build": 67}
if state_path.exists():
    try:
        # Read with UTF-8 encoding, stripping BOM if present
        content = state_path.read_text(encoding="utf-8-sig").strip()
        if content:
            version_state.update(json.loads(content))
            print(f"[Versioning] Loaded state: {version_state}")
        else:
            print(f"[Versioning] State file empty, using defaults")
    except Exception as e:
        print(f"[Versioning] Error loading state: {e}")
        print(f"[Versioning] Using defaults: {version_state}")

version_string = f"{version_state['major']}.{version_state['minor']}.{version_state['build']}"

print(f"[Versioning] Building version {version_string}")

# Generate version header
generated = (
    "#pragma once\n"
    f"// Auto-generated on {datetime.utcnow().isoformat()}Z\n"
    f"constexpr const char* APP_VERSION = \"{version_string}\";\n"
)
header_path.write_text(generated, encoding="utf-8")

env.Append(CPPDEFINES=[( "APP_VERSION_STR", f'"{version_string}"' )])
