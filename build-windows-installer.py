#!/usr/bin/env python3
import os
import sys
import re

APP_NAME = "Argon"
MANUFACTURER = "William Bell"
raw_version = os.environ.get("ARGON_VERSION", "0.0.0")
VERSION = re.sub(r'^v', '', raw_version)
VERSION = re.sub(r'-\d+-g[0-9a-f]+$', '', VERSION)
DIST_DIR = "out/windows/build/dist"
OUT_FILE = os.environ.get("OUTPUT_FILE", "argon-setup.exe")

lines = []
lines.append(f'Name "{APP_NAME}"')
lines.append(f'OutFile "{OUT_FILE}"')
lines.append(f'InstallDir "$PROGRAMFILES64\\{APP_NAME}"')
lines.append(f'InstallDirRegKey HKLM "Software\\{APP_NAME}" "Install_Dir"')
lines.append('RequestExecutionLevel admin')
lines.append('Unicode True')
lines.append('')
lines.append('Section "Main"')
lines.append('  SetOutPath "$INSTDIR"')
lines.append('')

# Walk all files
for root, dirs, files in os.walk(DIST_DIR):
    dirs.sort()
    rel_dir = os.path.relpath(root, DIST_DIR)
    if rel_dir == '.':
        win_dir = '$INSTDIR'
    else:
        win_dir = '$INSTDIR\\' + rel_dir.replace('/', '\\')
    
    if files:
        lines.append(f'  SetOutPath "{win_dir}"')
        for f in sorted(files):
            src = os.path.join(root, f)
            lines.append(f'  File "{src}"')
        lines.append('')

# Add to PATH
lines.append('  ; Add to PATH')
lines.append('  ReadRegStr $0 HKLM "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" "PATH"')
lines.append('  WriteRegExpandStr HKLM "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" "PATH" "$0;$INSTDIR\\bin"')
lines.append('  SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000')
lines.append('')

# Write uninstaller
lines.append('  WriteUninstaller "$INSTDIR\\uninstall.exe"')
lines.append('')
lines.append('  ; Add/Remove Programs entry')
lines.append(f'  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{APP_NAME}" "DisplayName" "{APP_NAME}"')
lines.append(f'  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{APP_NAME}" "DisplayVersion" "{VERSION}"')
lines.append(f'  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{APP_NAME}" "Publisher" "{MANUFACTURER}"')
lines.append(f'  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{APP_NAME}" "UninstallString" "$INSTDIR\\uninstall.exe"')
lines.append('SectionEnd')
lines.append('')
lines.append('Section "Uninstall"')
lines.append('  ; Note: PATH entry at $INSTDIR\\bin should be removed manually or via a custom action')
lines.append(f'  DeleteRegKey HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{APP_NAME}"')
lines.append(f'  DeleteRegKey HKLM "Software\\{APP_NAME}"')
lines.append('  RMDir /r "$INSTDIR"')
lines.append('SectionEnd')

out = sys.argv[1] if len(sys.argv) > 1 else "installer.nsi"
with open(out, 'w') as f:
    f.write('\n'.join(lines))
print(f"Written to {out}")