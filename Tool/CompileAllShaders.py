import os
import subprocess

def IsGlslSource(file):
    ext = file[-5:]
    result = (ext == ".vert") or (ext == ".frag") or (ext == ".geom") or (ext == ".comp") or (ext == ".tesc") or (ext == ".tese")
    return result

cwd = os.getcwd()
cwd = cwd + "\\..\\Coust\\shaders\\"
allGlslSources = [f for f in os.listdir(cwd) if os.path.isfile(os.path.join(cwd, f)) and IsGlslSource(f)]

compilerPath = os.getenv("VK_SDK_PATH")
compilerPath = compilerPath + "\\Bin\\glslc.exe"
for source in allGlslSources:
    outPath = cwd + "build\\" + source
    subprocess.run([compilerPath, cwd + source, "-S", "-o", outPath + ".ass"])
    subprocess.run([compilerPath, cwd + source, "-O", "-o", outPath + ".spv"])

