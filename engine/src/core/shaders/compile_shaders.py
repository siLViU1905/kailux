import subprocess
import os

def get_glslc_path():
    sdk_path = os.environ.get('VULKAN_SDK')

    if not sdk_path:
        print("Error: Environment VULKAN_SDK not found")
        return None

    glslc_executable = os.path.join(sdk_path, 'Bin', 'glslc.exe')

    if os.path.exists(glslc_executable):
        return glslc_executable
    else:
        print(f"Error: glslc.exe not found at : {glslc_executable}")
        return None
def compile_shader(shader_path):
    glslc = get_glslc_path()
    if not glslc:
        return

    stage = ""
    if "vertex" in shader_path.lower():
        stage = "vertex"
    elif "fragment" in shader_path.lower():
        stage = "fragment"

    base_name = os.path.splitext(shader_path)[0]
    output_path = f"{base_name}.spv"
    try:
        command = [glslc, "-fshader-stage=" + stage, shader_path, "-o", output_path]

        result = subprocess.run(command, capture_output=True, text=True)

        if result.returncode == 0:
            print(f"Successfully compiled: {shader_path} -> {output_path}")
        else:
            print(f"Error compiling {shader_path}:")
            print(result.stderr)

    except FileNotFoundError:
        print("Error: glslc.exe not found")

shaders = {
    "vertex_shader.glsl": "vertex_shader.spv",
    "fragment_shader.glsl": "fragment_shader.spv"
}

for src, out in shaders.items():
    if os.path.exists(src):
        compile_shader(src)
    else:
        print(f"Source file not found: {src}")