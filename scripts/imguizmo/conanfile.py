from conan import ConanFile
from conan.tools.files import get, copy
from conan.tools.layout import basic_layout
import os


class ImGuizmoConan(ConanFile):
    name = "imguizmo"
    version = "cci.20261010"
    description = "Immediate mode 3D gizmo for scene editing based on Dear ImGui"
    license = "MIT"
    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        self.requires("imgui/[>=1.90-docking]")

    def layout(self):
        basic_layout(self)

    def source(self):
        get(self,
            url="https://github.com/CedricGuillemet/ImGuizmo/archive/105e4fde45e4e74aae8973d98750820ef7aca7e2.tar.gz",
            strip_root=True)

    def package(self):
        src = os.path.join(self.source_folder, "src")
        copy(self, "*.h",   src=src, dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.cpp", src=src, dst=os.path.join(self.package_folder, "src"))
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))

    def package_info(self):
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.srcdirs = ["src"]