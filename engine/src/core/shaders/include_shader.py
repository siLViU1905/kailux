import os

def assemble_shader():
    main_path = "base_fragment_shader.glsl"
    include_path = input("To include shader path: ").strip()
    output_path = "fragment_shader.glsl"

    if not os.path.exists(main_path):
        print(f"{main_path} not found")
        return
    if not os.path.exists(include_path):
        print(f"{include_path} not found")
        return

    try:
        with open(main_path, 'r', encoding='utf-8') as f:
            main_lines = f.readlines()

        with open(include_path, 'r', encoding='utf-8') as f:
            include_content = f.read()

        final_content = []
        injected = False

        for line in main_lines:
            final_content.append(line)
            if "#version" in line and not injected:
                final_content.append("\n// --- BEGIN INCLUDE ---\n")
                final_content.append(include_content)
                final_content.append("\n// --- END INCLUDE ---\n\n")
                injected = True

        with open(output_path, 'w', encoding='utf-8') as f:
            f.writelines(final_content)

        print(f"\nSuccess! Assembled shader saved in: {output_path}")

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    assemble_shader()