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

        # Try to find flex in system PATH first
        flex_path = which("flex")

        # If not found, fallback to flex from Conan build requirements
        if not flex_path:
            flex_dep = self.dependencies.build.get("flex", None)
            if not flex_dep:
                raise Exception("Flex not found in system PATH and not declared as build requirement")
            flex_path = join(flex_dep.package_folder, "bin", "flex")

        tc.variables["FLEX_EXECUTABLE"] = flex_path

        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()