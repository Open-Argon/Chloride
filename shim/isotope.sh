#!/bin/sh

# SPDX-FileCopyrightText: 2026 William Bell
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Launcher for Isotope: `isotope <args>` is the same as `argon isotope <args>`.
#
# This is installed as <prefix>/bin/isotope, next to <prefix>/bin/argon. It
# runs the argon that sits beside it (not whatever `argon` happens to be first
# on PATH), so the stdlib and argon_modules that belong to this install are
# the ones that get used.

self=$0

# Follow symlinks (e.g. /usr/local/bin/isotope -> .../chloride/bin/isotope) so
# we find the real bin directory. `readlink` without flags is available on
# Linux, macOS, Alpine/busybox and the BSDs.
while [ -h "$self" ]; do
    dir=$(cd "$(dirname "$self")" && pwd)
    link=$(readlink "$self")
    case $link in
        /*) self=$link ;;
        *) self=$dir/$link ;;
    esac
done

dir=$(cd "$(dirname "$self")" && pwd)

exec "$dir/argon" isotope "$@"