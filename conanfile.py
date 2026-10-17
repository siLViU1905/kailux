from conan import ConanFile
from conan.tools.cmake import cmake_layout
from conan.tools.files import copy
import os

class kailux(ConanFile):
    name = "kailux"
    version = "1.0"
    package_type = "application"
    settings = "os", "compiler", "build_type", "arch"

    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("glfw/3.4")
        self.requires("imgui/1.92.9b-docking", force=True)
        self.requires("imgui_backends/1.92.9b")
        self.requires("glm/1.0.1")
        self.requires("entt/3.16.0")
        self.requires("magic_enum/0.9.7")
        self.requires("imguizmo/cci.20261010")
        self.requires("stb/cci.20230920")
        self.requires("assimp/6.0.2")
        self.requires("portable-file-dialogs/0.1.0")
        self.requires("nlohmann_json/3.12.0")
        self.requires("joltphysics/5.2.0")
        self.requires("shaderc/2025.3")
        self.requires("onetbb/2023.1.0")
        self.requires("meshoptimizer/1.0")

    def configure(self):
        self.options["imgui"].with_glfw = True
        self.options["imgui"].with_vulkan = True

    def layout(self):
        cmake_layout(self)

    def generate(self):
        backends = self.dependencies["imgui_backends"].package_folder

        bindings_dir = os.path.join(backends, "res", "bindings")
        bindings_dest_dir = os.path.join(self.source_folder, "kailux", "src", "core", "imgui_backend", "bindings")
        copy(self, "*", bindings_dir, bindings_dest_dir)

        misc_dir = os.path.join(backends, "res", "misc", "cpp")
        misc_dest_dir = os.path.join(self.source_folder, "kailux", "src", "core", "imgui_backend", "misc")

        copy(self, "*", misc_dir, misc_dest_dir)