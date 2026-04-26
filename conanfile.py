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
        self.requires("imgui/1.92.6")
        self.requires("glm/1.0.1")
        self.requires("entt/3.16.0")
        self.requires("magic_enum/0.9.7")
        self.requires("imguizmo/1.83")
        self.requires("stb/cci.20230920")
        self.requires("assimp/6.0.2")
        self.requires("portable-file-dialogs/0.1.0")
        self.requires("nlohmann_json/3.12.0")

    def configure(self):
        self.options["imgui"].with_glfw = True
        self.options["imgui"].with_vulkan = True

    def layout(self):
        cmake_layout(self)

    def generate(self):
        imgui = self.dependencies["imgui"]
        bindings_dir = os.path.join(imgui.package_folder, "res", "bindings")
        dest_dir = os.path.join(self.source_folder, "engine", "src", "core", "imgui_backend", "bindings")

        for pattern in ["*imgui_impl_glfw*", "*imgui_impl_vulkan*"]:
            copy(self, pattern, bindings_dir, dest_dir)