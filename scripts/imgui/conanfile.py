from conan import ConanFile
from conan.tools.files import copy, replace_in_file
import os


class ImGuiBackendsConan(ConanFile):
    name = "imgui_backends"
    version = "1.92.6"
    description = "Dear ImGui GLFW/Vulkan backends + imgui_stdlib, with local patches"
    license = "MIT"
    package_type = "header-library"
    no_copy_source = True

    def requirements(self):
        self.requires(f"imgui/{self.version}-docking")

    def package(self):
        imgui = self.dependencies["imgui"].package_folder

        bindings_dst = os.path.join(self.package_folder, "res", "bindings")
        misc_dst = os.path.join(self.package_folder, "res", "misc", "cpp")

        for pattern in ["*imgui_impl_glfw*", "*imgui_impl_vulkan*"]:
            copy(self, pattern, os.path.join(imgui, "res", "bindings"), bindings_dst)

        copy(self, "*imgui_stdlib*", os.path.join(imgui, "res", "misc", "cpp"), misc_dst)

        self._patch_vulkan_semaphores(os.path.join(bindings_dst, "imgui_impl_vulkan.cpp"))

    def _patch_vulkan_semaphores(self, path):
        replace_in_file(self, path,
                        "info.pSignalSemaphores = &fsd->RenderCompleteSemaphore;",
                        "info.pSignalSemaphores = &wd->FrameSemaphores[wd->FrameIndex].RenderCompleteSemaphore;")
        replace_in_file(self, path,
                        "info.pWaitSemaphores = &fsd->RenderCompleteSemaphore;",
                        "info.pWaitSemaphores = &wd->FrameSemaphores[wd->FrameIndex].RenderCompleteSemaphore;")
        self.output.info("Patched imgui_impl_vulkan.cpp: per-image RenderCompleteSemaphore")

    def package_info(self):
        self.cpp_info.includedirs = []
        self.cpp_info.libdirs = []
        self.cpp_info.bindirs = []
        self.cpp_info.resdirs = ["res"]