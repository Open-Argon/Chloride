# SPDX-FileCopyrightText: 2025 William Bell
#
# SPDX-License-Identifier: GPL-3.0-or-later

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from shutil import which

class ArgonConan(ConanFile):
    name = "argon"
    version = "1.0"

    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"
    # Remove tool_requires, no flex from Conan
    requires = [
        "gmp/6.3.0",
        "bdwgc/8.2.8"
    ]

    default_options = {
        "gmp/*:shared": False,
        "bdwgc/*:shared": False
    }

    def layout(self):
        self.folders.source = "."
        self.folders.build = "build"
        self.folders.generators = "build"

    def generate(self):
        tc = CMakeToolchain(self)

        if os.name == "nt":  # Windows
            flex_path = which("win_flex") or which("win_flex.exe")
            if not flex_path:
                raise Exception("win_flex not found in PATH. Install winflexbison via choco.")
        else:
            flex_path = which("flex")
            if not flex_path:
                raise Exception("Flex not found in system PATH. Please install flex on Linux/macOS.")

        tc.variables["FLEX_EXECUTABLE"] = flex_path
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()