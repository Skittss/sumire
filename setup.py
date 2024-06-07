import os
import argparse
import subprocess
import sys
import platform
#import requests # Remove with GLSLANG downloads
import zipfile
import io

# ---- Global Consts --------------------------------------------------------------------------------------------
VERBOSE = True

OS_SYSTEM = platform.system()
if (OS_SYSTEM != 'Windows'):
    raise RuntimeError("Non-windows OSes are currently unsupported.") 

#GLSLANG_BUILD_PATH = f"external/glslang/build/{OS_SYSTEM}"
#GLSLANG_BUILD_PATH_DEBUG = f"{GLSLANG_BUILD_PATH}/Debug"
#GLSLANG_BUILD_PATH_RELEASE = f"{GLSLANG_BUILD_PATH}/Release"
#GLSLANG_GITHUB_API_DIST_URL = "https://api.github.com/repos/KhronosGroup/glslang/releases/tags/main-tot"

ENV_CMAKE_PATH = ".env.cmake"
ENV_CMAKE_META_STR = "# SUMIRE .env.cmake  -  Do not modify this comment and the next two lines."

# ---------------------------------------------------------------------------------------------------------------

def command_output(cmd, directory, fail_ok=False):
    """Runs a command in a directory and returns its standard output stream.

    Captures the standard error stream.

    Raises a RuntimeError if the command fails to launch or otherwise fails.
    """
    if VERBOSE:
        print('In {d}: {cmd}'.format(d=directory, cmd=cmd))
    p = subprocess.Popen(cmd,
                         cwd=directory,
                         stdout=subprocess.PIPE)
    (stdout, _) = p.communicate()
    if p.returncode != 0 and not fail_ok:
        raise RuntimeError('Failed to run {} in {}'.format(cmd, directory))
    if VERBOSE:
        print(stdout)
    return stdout

def command_retval(cmd, directory):
    """Runs a command in a directory and returns its return value.

    Captures the standard error stream.
    """
    p = subprocess.Popen(cmd,
                         cwd=directory,
                         stdout=subprocess.PIPE)
    p.communicate()
    return p.returncode

def getFiles(path):
    ls = os.listdir(path)
    files = [f for f in ls if os.path.isfile(os.path.join(path, f))]

    return files

#def validateDebugGlslangBinaries(path):
#    binaries = [
#        'GenericCodeGend.lib',
#        'glslangd.lib',
#        'glslang-default-resource-limitsd.lib',
#        'MachineIndependentd.lib',
#        'OSDependentd.lib',
#        'SPIRVd.lib',
#        'SPVRemapperd.lib',
#        'SPIRV-Toolsd.lib',
#        'SPIRV-Tools-optd.lib'
#    ]
#
#    files = getFiles(path)
#    for f in files:
#        if f in binaries:
#            binaries.remove(f)
#
#    return len(binaries) == 0
#
#def validateReleaseGlslangBinaries(path):
#    binaries = [
#        'GenericCodeGen.lib',
#        'glslang.lib',
#        'glslang-default-resource-limits.lib',
#        'MachineIndependent.lib',
#        'OSDependent.lib',
#        'SPIRV.lib',
#        'SPVRemapper.lib',
#        'SPIRV-Tools.lib',
#        'SPIRV-Tools-opt.lib'
#    ]
#
#    files = getFiles(path)
#    for f in files:
#        if f in binaries:
#            binaries.remove(f)
#
#    return len(binaries) == 0
#
#def validGlslangBinaries():
#    debugLibPath = os.path.join(GLSLANG_BUILD_PATH_DEBUG, "lib")
#    releaseLibPath = os.path.join(GLSLANG_BUILD_PATH_RELEASE, "lib")
#    validDebugFolder = os.path.exists(debugLibPath)
#    validReleaseFolder = os.path.exists(releaseLibPath)
#
#    validDebug = False
#    validRelease = False
#
#    if validDebugFolder:
#        validDebug = validateDebugGlslangBinaries(debugLibPath)
#    if validReleaseFolder:
#        validRelease = validateReleaseGlslangBinaries(releaseLibPath)
#
#    return (validDebug, validRelease)
#
#def downloadArchive(url, dest):
#    response = requests.get(url, stream=True)
#    if not response.ok:
#        raise RuntimeError(f"Failed to download archive to {dest} [Code: {response.status_code}].")
#
#    z = zipfile.ZipFile(io.BytesIO(response.content))
#    z.extractall(dest)
#
#def downloadGlslangBinaries(getDebug = True, getRelease = True):
#    if not (getDebug or getRelease):
#        return
#
#    # Get Asset URLs
#    response = requests.get(GLSLANG_GITHUB_API_DIST_URL)
#    if not response.ok:
#        raise RuntimeError(f"Asset URL fetch github request failed: {response.status_code}.")
#    response = response.json()
#
#    debugAssetURL = None
#    debugAssetName = None
#    releaseAssetURL = None
#    releaseAssetName = None
#
#    assets = response["assets"]
#    for asset in assets:
#        if "windows" in asset["name"]:
#            if getDebug and "Debug" in asset["name"]:
#                debugAssetURL = asset["browser_download_url"]
#                debugAssetName = asset["name"]
#            elif getRelease and "Release" in asset["name"]:
#                releaseAssetURL = asset["browser_download_url"]
#                releaseAssetName = asset["name"]
#
#    # Download asset archives (.zip)
#    if getDebug:
#        if not debugAssetURL:
#            raise RuntimeError("Could not fetch Debug asset URL from glslang repo.")
#        else:
#            print("    Downloading glslang Debug binaries...")
#            downloadArchive(debugAssetURL, GLSLANG_BUILD_PATH_DEBUG)
#            print("    Downloaded glslang Debug binaries.")
#
#    if getRelease:
#        if not releaseAssetURL:
#            raise RuntimeError("Could not fetch Release asset URL from glslang repo.")
#        else:
#            print("    Downloading glslang Release binaries...")
#            downloadArchive(releaseAssetURL, GLSLANG_BUILD_PATH_RELEASE)
#            print("    Downloaded glslang Release binaries.")

def initEnvCmake():
    with open(ENV_CMAKE_PATH, 'w') as f:
        f.seek(0, 0)
        f.write(f"{ENV_CMAKE_META_STR}\n") # meta
        f.write('#set(VULKAN_SDK_PATH <YOUR PATH HERE>)\n') # vkPth
        f.write('#set(GLFW_PATH <YOUR PATH HERE>)\n') # vkPth
        f.write("#---------------------------------------------------------------------")

def writeEnvCmake(vkPth, glfwPth):
    with open(ENV_CMAKE_PATH, 'r') as fr:
        lines = fr.readlines()

    nLines = len(lines)
    for i in range(0, 3 - nLines):
        line = nLines + i
        if line == 0:
            lines.append(f"{ENV_CMAKE_META_STR}\n")
        elif line == 1:
            lines.append('#set(VULKAN_SDK_PATH <YOUR PATH HERE>)\n')
        elif line == 2:
            lines.append('#set(GLFW_PATH <YOUR PATH HERE>)\n')
        else:
            lines.append("")

    lines[0] = f"{ENV_CMAKE_META_STR}\n"
    if vkPth:
        escapedVkPth = vkPth.replace("\\", "\\\\")
        lines[1] = f'set(VULKAN_SDK_PATH "{escapedVkPth}")\n'
    if glfwPth:
        escapedGlfwPth = glfwPth.replace("\\", "\\\\")
        lines[2] = f'set(GLFW_PATH "{escapedGlfwPth}")\n'

    with open(ENV_CMAKE_PATH, 'r+') as fw:
        fw.seek(0, 0)
        fw.writelines(lines)

def setupEnvCmake(vkPth, glfwPth):
    if not os.path.exists(ENV_CMAKE_PATH):
        initEnvCmake()
        print(f"Created cmake environment {ENV_CMAKE_PATH}")

    if vkPth or glfwPth:
        if vkPth:
            print(f"Using Vulkan library at {vkPth}")
        if glfwPth:
            print(f"using GLFW library at {glfwPth}")
        writeEnvCmake(vkPth, glfwPth)
    else:
        print(".env.cmake OK.")

# TODO: Switched to Shaderc, this is no longer required.
#       Keeping as reference for now.
#def setupGlslang():
#    # TODO: An option to update binaries would be nice here
#    # Create build dirs
#    if not os.path.exists(GLSLANG_BUILD_PATH_DEBUG):
#        print(f"Creating glslang Debug build folder at '{GLSLANG_BUILD_PATH_DEBUG}'")
#        os.makedirs(GLSLANG_BUILD_PATH_DEBUG)
#
#    if not os.path.exists(GLSLANG_BUILD_PATH_RELEASE):
#        print(f"Creating glslang Release build folder at '{GLSLANG_BUILD_PATH_RELEASE}'")
#        os.makedirs(GLSLANG_BUILD_PATH_RELEASE)
#
#    # Get binaries
#    validDebug, validRelease = validGlslangBinaries()
#    binDownloadStr = []
#    if not validDebug:
#        binDownloadStr.append("Debug")
#    if not validRelease:
#        binDownloadStr.append("Release")
#
#    if not validDebug or not validRelease:
#        print(f"Downloading glslang binaries ({', '.join(binDownloadStr)})...")
#        downloadGlslangBinaries(not validDebug, not validRelease)
#        print("Downloaded glslang binaries.")
#
#        print("Validating glslang binaries.")
#        if not validGlslangBinaries():
#            raise RuntimeError("Downloaded glslang binaries were not valid.")
#        print("Validated glslang binaries.")
#
#    else:
#        print("Valid glslang binaries already present. Skipping download.")


def setupSumire():
    # Create build dir
    if not os.path.exists("build/msvc"):
        print("Creating sumire build folder at 'build/msvc'.")
        os.makedirs("build/msvc")
    else:
        print("Build folder OK.")

def main(vkPth, glfwPth, updateGlslang):
    print("----- Setting Up .env.cmake -----------------------")
    setupEnvCmake(vkPth, glfwPth)
    print()
    #print("----- Setting Up glslang --------------------------")
    #setupGlslang()
    #print()
    print("----- Setting Up Sumire ---------------------------")
    setupSumire()
    pass

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog="Sumire Setup")
    parser.add_argument('--vulkan',         type=str, required=False, default=None)
    parser.add_argument('--glfw',           type=str, required=False, default=None)
    #parser.add_argument('--update-glslang', type=str, required=False, action='store_true')

    args = parser.parse_args()

    main(args.vulkan, args.glfw, False)