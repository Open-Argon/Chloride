#!/usr/bin/env python3
import os
import uuid
import sys

# Config
APP_NAME = "Argon"
MANUFACTURER = "William Bell"
VERSION = os.environ.get("ARGON_VERSION", "dev")
UPGRADE_CODE = "a77b066a-9b7b-4580-be8e-25320f96d86c"
DIST_DIR = "out/windows/build/dist"

def guid():
    return str(uuid.uuid4()).upper()

def collect_files(base):
    """Walk base and yield (relative_path, dir_id, file) tuples."""
    for root, dirs, files in os.walk(base):
        dirs.sort()
        for f in sorted(files):
            abs_path = os.path.join(root, f)
            rel_path = os.path.relpath(abs_path, DIST_DIR)
            yield rel_path, abs_path

def dir_id(path):
    """Turn a relative dir path into a safe WiX ID."""
    return "Dir_" + path.replace(os.sep, "_").replace("-", "_").replace(".", "_")

def build_dir_tree(paths):
    """Build a nested dict representing the directory tree."""
    tree = {}
    for p in paths:
        parts = p.split(os.sep)
        node = tree
        for part in parts[:-1]:
            node = node.setdefault(part, {})
    return tree

def render_dirs(tree, parent_id, parent_path=""):
    lines = []
    for name, subtree in sorted(tree.items()):
        path = os.path.join(parent_path, name) if parent_path else name
        did = dir_id(path)
        lines.append(f'        <Directory Id="{did}" Name="{name}">')
        lines.extend(render_dirs(subtree, did, path))
        lines.append(f'        </Directory>')
    return lines

def render_components(rel_paths):
    lines = []
    for rel_path in rel_paths:
        parts = rel_path.split(os.sep)
        dir_path = os.sep.join(parts[:-1]) if len(parts) > 1 else ""
        did = dir_id(dir_path) if dir_path else "INSTALLDIR"
        cid = "Comp_" + rel_path.replace(os.sep, "_").replace("-", "_").replace(".", "_")
        source = os.path.join(DIST_DIR, rel_path)
        lines.append(f'      <Component Id="{cid}" Directory="{did}">')
        lines.append(f'        <File Source="{source}" KeyPath="yes" />')
        lines.append(f'      </Component>')
    return lines

# Collect all files
all_files = list(collect_files(DIST_DIR))
rel_paths = [r for r, _ in all_files]

# Separate stdlib and bin/license files
stdlib_paths = [p for p in rel_paths if p.startswith("stdlib" + os.sep)]
other_paths  = [p for p in rel_paths if not p.startswith("stdlib" + os.sep)]

# Build stdlib dir tree
stdlib_dirs = build_dir_tree([p[len("stdlib/"):] for p in stdlib_paths])

wxs = f"""<?xml version="1.0" encoding="UTF-8"?>
<Wix xmlns="http://wixtoolset.org/schemas/v4/wxs"
     xmlns:util="http://wixtoolset.org/schemas/v4/wxs/util">
  <Package Name="{APP_NAME}"
           Version="{VERSION}"
           Manufacturer="{MANUFACTURER}"
           UpgradeCode="{UPGRADE_CODE}">

    <MajorUpgrade DowngradeErrorMessage="A newer version of Argon is already installed." />
    <MediaTemplate EmbedCab="yes" />

    <Feature Id="Main" Level="1">
      <ComponentGroupRef Id="BinComponents" />
      <ComponentGroupRef Id="StdlibComponents" />
      <ComponentGroupRef Id="LicenseComponents" />
      <ComponentRef Id="PathEnvComponent" />
    </Feature>

    <StandardDirectory Id="ProgramFilesFolder">
      <Directory Id="INSTALLDIR" Name="{APP_NAME}">
        <Directory Id="Dir_bin" Name="bin" />
        <Directory Id="Dir_LICENSES" Name="LICENSES" />
        <Directory Id="Dir_stdlib" Name="stdlib">
"""

for line in render_dirs(stdlib_dirs, "Dir_stdlib", "stdlib"):
    wxs += line + "\n"

wxs += f"""        </Directory>
      </Directory>
    </StandardDirectory>

    <!-- Add bin/ to PATH -->
    <ComponentGroup Id="PathEnvGroup">
      <Component Id="PathEnvComponent" Directory="Dir_bin">
        <Environment Id="PATH"
                     Name="PATH"
                     Value="[Dir_bin]"
                     Permanent="no"
                     Part="last"
                     Action="set"
                     System="yes" />
        <RegistryValue Root="HKLM"
                       Key="Software\\{APP_NAME}"
                       Name="PathSet"
                       Type="integer"
                       Value="1"
                       KeyPath="yes" />
      </Component>
    </ComponentGroup>

"""

# Bin and license components
bin_comps   = [p for p in other_paths if p.startswith("bin" + os.sep)]
lic_comps   = [p for p in other_paths if p.startswith("LICENSES" + os.sep) or p == "LICENSE.txt"]

wxs += "    <ComponentGroup Id=\"BinComponents\">\n"
for line in render_components(bin_comps):
    wxs += "  " + line + "\n"
wxs += "    </ComponentGroup>\n\n"

wxs += "    <ComponentGroup Id=\"LicenseComponents\">\n"
for line in render_components(lic_comps):
    wxs += "  " + line + "\n"
wxs += "    </ComponentGroup>\n\n"

wxs += "    <ComponentGroup Id=\"StdlibComponents\">\n"
for line in render_components(stdlib_paths):
    wxs += "  " + line + "\n"
wxs += "    </ComponentGroup>\n\n"

wxs += "  </Package>\n</Wix>\n"

out = sys.argv[1] if len(sys.argv) > 1 else "argon.wxs"
with open(out, "w") as f:
    f.write(wxs)
print(f"Written to {out}")