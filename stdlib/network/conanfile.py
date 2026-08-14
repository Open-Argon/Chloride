# SPDX-FileCopyrightText: 2026 William Bell
#
# SPDX-License-Identifier: LGPL-3.0-or-later

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
import os


class ArgonNetworkStdlibConan(ConanFile):
    name = "argon-stdlib-network"
    version = "1.0"

    settings = "os", "compiler", "build_type", "arch"

    requires = [
        "openssl/1.1.1w",
    ]

    default_options = {
        # Embed OpenSSL into network.so rather than depending on
        # libssl.so / libcrypto.so at runtime.
        "openssl/*:shared": False,

        # OpenSSL must be built as PIC because it is being linked
        # into our shared library.
        "openssl/*:fPIC": True,
    }

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
                "libnetwork.dll.a",
            )

            if os.path.exists(implib):
                os.remove(implib)