#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 William Bell
#
# SPDX-License-Identifier: GPL-3.0-or-later

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

# FSF-faithful explanation of GPL-3.0 and free software, shown on a custom
# page before the scrollable LICENSE.txt. Wording follows the FSF's own
# published guidance on describing the GPL and the four freedoms.
FREE_SOFTWARE_TEXT = (
    f"{APP_NAME} is free software: you can redistribute it and/or modify it "
    "under the terms of the GNU General Public License as published by the "
    "Free Software Foundation, either version 3 of the License, or (at your "
    "option) any later version.$\\r$\\n$\\r$\\n"
    "Free software means software that respects your freedom. Specifically, "
    "it gives you four essential freedoms:$\\r$\\n$\\r$\\n"
    "  (0)  The freedom to run the program as you wish, for any purpose.$\\r$\\n"
    "  (1)  The freedom to study how the program works and change it so it "
    "does your computing as you wish. Access to the source code is a "
    "precondition for this.$\\r$\\n"
    "  (2)  The freedom to redistribute copies so you can help others.$\\r$\\n"
    "  (3)  The freedom to distribute copies of your modified versions to "
    "others. By doing this you can give the whole community a chance to "
    "benefit from your changes. Access to the source code is a precondition "
    "for this.$\\r$\\n$\\r$\\n"
    f"The source code for {APP_NAME} is available at:$\\r$\\n"
    "https://github.com/wbellxyz/argon$\\r$\\n$\\r$\\n"  # update URL as needed
    "The next page shows the full license text. You must review it before "
    "continuing the installation."
)

lines = []

# ── MUI2 includes & settings ─────────────────────────────────────────────────
lines.append('!include "MUI2.nsh"')
lines.append('')
lines.append(f'Name "{APP_NAME}"')
lines.append(f'OutFile "{OUT_FILE}"')
lines.append(f'InstallDir "$PROGRAMFILES64\\{APP_NAME}"')
lines.append(f'InstallDirRegKey HKLM "Software\\{APP_NAME}" "Install_Dir"')
lines.append('RequestExecutionLevel admin')
lines.append('Unicode True')
lines.append('')

# ── Custom "What is free software?" page ─────────────────────────────────────
# We reuse MUI_PAGE_WELCOME a second time via a custom page function so we can
# have both a welcome splash and a separate free-software explanation page.
lines.append('; -- Free software explanation page (custom, before license) --')
lines.append('!define MUI_PAGE_CUSTOMFUNCTION_PRE FreeSwPagePre')
lines.append('!define MUI_WELCOMEPAGE_TITLE "Free Software — Your Rights"')
lines.append(f'!define MUI_WELCOMEPAGE_TEXT "{FREE_SOFTWARE_TEXT}"')
lines.append('!insertmacro MUI_PAGE_WELCOME')
lines.append('')
lines.append('Function FreeSwPagePre')
lines.append('  ; Nothing extra needed — the defines above set the content.')
lines.append('FunctionEnd')
lines.append('')

# ── Scrollable LICENSE.txt page ───────────────────────────────────────────────
lines.append('; -- Full GPL-3.0 license text (scrollable) --')
lines.append('!define MUI_LICENSEPAGE_CHECKBOX')          # user must tick a checkbox
lines.append('!define MUI_LICENSEPAGE_CHECKBOX_TEXT "I have read and understood the terms of the GNU General Public License."')
lines.append('!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"')
lines.append('')

# ── Welcome splash ────────────────────────────────────────────────────────────
lines.append('; -- Welcome splash (shown first, before the rights pages) --')
lines.append('!define MUI_PAGE_CUSTOMFUNCTION_PRE WelcomePagePre')
lines.append(f'!define MUI_WELCOMEPAGE_TITLE "Welcome to the {APP_NAME} {VERSION} Setup"')
lines.append(f'!define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the installation of {APP_NAME} {VERSION}.$\\r$\\n$\\r$\\nBefore the installation begins, you will be shown information about your rights as a user of this free software, followed by the full license text.$\\r$\\n$\\r$\\nClick Next to continue."')
lines.append('!insertmacro MUI_PAGE_WELCOME')
lines.append('')
lines.append('Function WelcomePagePre')
lines.append('FunctionEnd')
lines.append('')

# ── Remaining pages ───────────────────────────────────────────────────────────
lines.append('!insertmacro MUI_PAGE_DIRECTORY')
lines.append('!insertmacro MUI_PAGE_INSTFILES')
lines.append('')
lines.append(f'!define MUI_FINISHPAGE_TEXT "{APP_NAME} has been installed successfully."')
lines.append('!insertmacro MUI_PAGE_FINISH')
lines.append('')
lines.append('!insertmacro MUI_UNPAGE_CONFIRM')
lines.append('!insertmacro MUI_UNPAGE_INSTFILES')
lines.append('')
lines.append('!insertmacro MUI_LANGUAGE "English"')
lines.append('')

# ── Install section ───────────────────────────────────────────────────────────
lines.append('Section "Main"')
lines.append('  SetOutPath "$INSTDIR"')
lines.append('')

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

# Write uninstaller & ARP entry
lines.append('  WriteUninstaller "$INSTDIR\\uninstall.exe"')
lines.append('')
lines.append('  ; Add/Remove Programs entry')
lines.append(f'  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{APP_NAME}" "DisplayName" "{APP_NAME}"')
lines.append(f'  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{APP_NAME}" "DisplayVersion" "{VERSION}"')
lines.append(f'  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{APP_NAME}" "Publisher" "{MANUFACTURER}"')
lines.append(f'  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{APP_NAME}" "UninstallString" "$INSTDIR\\uninstall.exe"')
lines.append('SectionEnd')
lines.append('')

# ── Uninstall section ─────────────────────────────────────────────────────────
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