# SPDX-FileCopyrightText: 2025 William Bell
#
# SPDX-License-Identifier: GPL-3.0-or-later

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from shutil import which
import os

class ArgonConan(ConanFile):
    name = "argon"
    version = "1.0"

    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"
    # Remove tool_requires, no flex from Conan
    requires = [
        "gmp/6.3.0",
        "bdwgc/8.2.6"
    ]

    default_options = {
        "gmp/*:shared": False,
        "bdwgc/*:shared": False,
        "bdwgc/*:parallel_mark": False,
        "bdwgc/*:threads": True,
        "bdwgc/*:disable_debug": True,
    }

    def layout(self):
        self.folders.source = "."
        self.folders.build = "build"
        self.folders.generators = "build"

    def generate(self):
        os.environ["CONAN_NON_INTERACTIVE"] = "1"
        tc = CMakeToolchain(self)

        flex_path = which("flex")
        if not flex_path:
            raise Exception("Flex not found in system PATH. Please install flex on Linux/macOS.")

        tc.variables["FLEX_EXECUTABLE"] = flex_path.replace("\\", "\\\\")
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()