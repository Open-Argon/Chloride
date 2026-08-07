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

# ARCH selects which build output tree to package and which NSIS target
# arch/registry view to generate for. Defaults to x86_64 to preserve
# existing behaviour when the env var isn't set.
ARCH = os.environ.get("ARGON_ARCH", "x86_64")
if ARCH not in ("x86_64", "arm64"):
    sys.exit(f"Unsupported ARGON_ARCH: {ARCH!r} (expected 'x86_64' or 'arm64')")

DIST_DIR_BY_ARCH = {
    "x86_64": "out/windows/build/dist",
    "arm64": "out/windows-arm64/build/dist",
}
DIST_DIR = DIST_DIR_BY_ARCH[ARCH]
OUT_FILE = os.environ.get("OUTPUT_FILE", f"argon-setup-{ARCH}.exe")

# NSIS target architecture: controls whether the installer runs natively as
# a 64-bit process (needed so $PROGRAMFILES64 / HKLM 64-bit registry view
# resolve correctly on both x86_64 and ARM64 Windows — ARM64 Windows has no
# separate "native ARM64" NSIS target constant, but running as an amd64
# process under emulation still gets you the 64-bit Program Files/registry
# view, so amd64-unicode is used for both).
NSIS_TARGET = "amd64-unicode"

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
    "https://git.wbell.dev/Open-Argon/Chloride$\\r$\\n"
    "https://github.com/Open-Argon/Chloride (Mirror)$\\r$\\n$\\r$\\n"
    "The next page shows the full license text. You must review it before "
    "continuing the installation."
)

lines = []

# ── MUI2 includes & settings ─────────────────────────────────────────────────
lines.append(f'!define ARCH "{ARCH}"')
lines.append(f'; target arch: {ARCH} (NSIS built as {NSIS_TARGET})')
lines.append('!include "MUI2.nsh"')
lines.append('')
lines.append('Unicode True')
# Distinct display name per arch avoids two "Argon" entries silently
# clobbering each other's Program Files dir / uninstall registry key if a
# user ends up with both installed side by side (e.g. testing).
INSTALL_NAME = APP_NAME if ARCH == "x86_64" else f"{APP_NAME} (ARM64)"
lines.append(f'Name "{INSTALL_NAME}"')
lines.append(f'OutFile "{OUT_FILE}"')
lines.append(f'InstallDir "$PROGRAMFILES64\\{INSTALL_NAME}"')
lines.append(f'InstallDirRegKey HKLM "Software\\{INSTALL_NAME}" "Install_Dir"')
lines.append('RequestExecutionLevel admin')
lines.append('')

# ── Branding ──────────────────────────────────────────────────────────────────
lines.append('!define MUI_ICON "assets\\argon.ico"')
lines.append('!define MUI_UNICON "assets\\argon.ico"')
lines.append('!define MUI_WELCOMEFINISHPAGE_BITMAP "assets\\installer-sidebar.bmp"')
lines.append('!define MUI_WELCOMEFINISHPAGE_BITMAP_NOSTRETCH')
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

# ── Custom "What is free software?" page ─────────────────────────────────────
# We reuse MUI_PAGE_WELCOME a second time via a custom page function so we can
# have both a welcome splash and a separate free-software explanation page.
lines.append('; -- Free software explanation page (custom, before license) --')
lines.append('!define MUI_PAGE_CUSTOMFUNCTION_PRE FreeSwPagePre')
lines.append('!define MUI_WELCOMEPAGE_TITLE "What is free software?"')
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
lines.append(f'  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{INSTALL_NAME}" "DisplayName" "{INSTALL_NAME}"')
lines.append(f'  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{INSTALL_NAME}" "DisplayVersion" "{VERSION}"')
lines.append(f'  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{INSTALL_NAME}" "Publisher" "{MANUFACTURER}"')
lines.append(f'  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{INSTALL_NAME}" "UninstallString" "$INSTDIR\\uninstall.exe"')
lines.append('SectionEnd')
lines.append('')

# ── Uninstall section ─────────────────────────────────────────────────────────
lines.append('Section "Uninstall"')
lines.append('  ; Note: PATH entry at $INSTDIR\\bin should be removed manually or via a custom action')
lines.append(f'  DeleteRegKey HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{INSTALL_NAME}"')
lines.append(f'  DeleteRegKey HKLM "Software\\{INSTALL_NAME}"')
lines.append('  RMDir /r "$INSTDIR"')
lines.append('SectionEnd')

out = sys.argv[1] if len(sys.argv) > 1 else "installer.nsi"
with open(out, 'w') as f:
    f.write('\n'.join(lines))
print(f"Written to {out}")