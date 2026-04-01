from conan import ConanFile
from conan.tools.files import get, copy, replace_in_file
from conan.tools.layout import basic_layout
import os


class ImGuizmoConan(ConanFile):
    name = "imguizmo"
    version = "1.83"
    description = "Immediate mode 3D gizmo for scene editing based on Dear ImGui"
    license = "MIT"
    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        self.requires("imgui/[>=1.90]")

    def layout(self):
        basic_layout(self)

    def source(self):
        get(self,
            url="https://github.com/CedricGuillemet/ImGuizmo/archive/refs/tags/1.83.tar.gz",
            sha256="e6d05c5ebde802df7f6c342a06bc675bd2aa1c754d2d96755399a182187098a8",
            strip_root=True)

        replacements = {
            "BeginChildFrame": "BeginChild",
            "EndChildFrame": "EndChild",
            "ImGui::CaptureMouseFromApp(": "// ImGui::CaptureMouseFromApp(",
            "CaptureMouseFromApp(": "// CaptureMouseFromApp("
        }

        for root, dirs, files in os.walk(self.source_folder):
            for file in files:
                if file.endswith(".cpp") or file.endswith(".h"):
                    file_path = os.path.join(root, file)

                    with open(file_path, "r", encoding="utf-8") as f:
                        content = f.read()

                    new_content = content
                    for old, new in replacements.items():
                        new_content = new_content.replace(old, new)

                    if new_content != content:
                        self.output.info(f"Patch applied to: {file}")
                        with open(file_path, "w", encoding="utf-8") as f:
                            f.write(new_content)

    def package(self):
        copy(self, "*.h",   src=self.source_folder, dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.cpp", src=self.source_folder, dst=os.path.join(self.package_folder, "src"))
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))

    def package_info(self):
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.srcdirs = ["src"]