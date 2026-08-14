# SPDX-FileCopyrightText: 2026 William Bell
#
# SPDX-License-Identifier: LGPL-3.0-or-later

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
import os


class ArgonArchiveStdlibConan(ConanFile):
    name = "argon-stdlib-archive"
    version = "1.0"

    settings = "os", "compiler", "build_type", "arch"

    requires = [
        "libarchive/3.8.7",
    ]

    default_options = {
        # We are embedding the native dependency graph into archive.so.
        "*:shared": False,
        "*:fPIC": True,

        # libarchive features
        "libarchive/*:with_zlib": True,
        "libarchive/*:with_zstd": True,
        "libarchive/*:with_lzma": True,
        "libarchive/*:with_bzip2": True,
        "libarchive/*:with_lz4": True,
        "libarchive/*:with_libxml2": True,
        "libarchive/*:with_openssl": False,

        # We don't need libarchive utilities.
        "libarchive/*:with_acl": False,
        "libarchive/*:with_xattr": False,
        "libarchive/*:with_iconv": False,
    }

    def configure(self):
        if self.settings.os == "Windows":
            self.options["libarchive"].with_libxml2 = False

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        tc = CMakeToolchain(self)
        tc.variables["CMAKE_POSITION_INDEPENDENT_CODE"] = True
        tc.variables["BUILD_SHARED_LIBS"] = False
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

        if self.settings.os == "Windows":
            implib = os.path.join(
                self.source_folder,
                "native",
                "bin",
                "libarchive.dll.a",
            )

            if os.path.exists(implib):
                os.remove(implib)