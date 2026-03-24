echo off
set VULKAN_SDK=C:\VulkanSDK\1.4.335.0\Bin
set SHADER_DIR=./
for %%f in (%SHADER_DIR%*.frag %SHADER_DIR%*.vert %SHADER_DIR%*.comp %SHADER_DIR%*.compute) do (
    echo "%%f"
    "%VULKAN_SDK%\glslc.exe" "%%f" -o "%%f.spv"
)

echo Shader compilation complete.