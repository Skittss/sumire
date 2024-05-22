cmake_minimum_required(VERSION 3.15.0)
set(NAME sumire)
project(${NAME} VERSION 0.1.0)

option(ENABLE_BUILD_ERRORS "Enable build warnings and errors." ON)

include(./.env.cmake OPTIONAL RESULT_VARIABLE LOCAL_ENV)
message(STATUS "Local .env.cmake: ${LOCAL_ENV}")
 
# Compiler choice
message(STATUS "using ${CMAKE_GENERATOR}")
if (CMAKE_GENERATOR STREQUAL "MinGW Makefiles")
    if (NOT MINGW_PATH) 
        message(FATAL_ERROR "MINGW_PATH not set in .env.cmake")
    endif()
    set(USE_MINGW "True")
    set(CMAKE_C_COMPILER ${MINGW_PATH}/bin/gcc.exe)
    set(CMAKE_CXX_COMPILER  ${MINGW_PATH}/bin/g++.exe)
endif()
 
# Target vulkan
if (DEFINED VULKAN_SDK_PATH)
    set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include")
    set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/Lib")
    set(Vulkan_FOUND "True")
else()
    find_package(Vulkan REQUIRED)
    message(STATUS "Found Vulkan: $ENV{VULKAN_SDK}")
endif()
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Could not find Vulkan library!")
else()
    message(STATUS "Using vulkan lib at: ${Vulkan_LIBRARIES}")
endif()
 
# Target GLFW
if (DEFINED GLFW_PATH)
    message(STATUS "Using GLFW path specified in .env")
    set(GLFW_INCLUDE_DIRS "${GLFW_PATH}/include")
    if (MSVC)
        set(GLFW_LIB "${GLFW_PATH}/lib-vc2022") # Make sure this matches MSVC version
    elseif (CMAKE_GENERATOR STREQUAL "MinGW Makefiles")
        message(STATUS "USING MINGW")
        set(GLFW_LIB "${GLFW_PATH}/lib-mingw-w64")
    endif()
else()
    find_package(glfw3 3.3 REQUIRED)
    set(GLFW_LIB glfw)
    message(STATUS "Found GLFW")
endif()
if (NOT GLFW_LIB)
    message(FATAL_ERROR "Could not find glfw library!")
else()
    message(STATUS "Using glfw lib at: ${GLFW_LIB}")
endif()
  
# TINY OBJ LOADER
if (NOT TINYOBJ_PATH)
    message(STATUS "TINYOBJ_PATH not specified in .env.cmake, using external/tinyobjloader")
    set(TINYOBJ_PATH external/tinyobjloader)
endif()

# TINY GLTF LOADER
if (NOT TINYGLTF_PATH)
    message(STATUS "TINYGLTF_PATH not specified in .env.cmake, using external/tinygltf")
    set(TINYGLTF_PATH external/tinygltf)
endif()

# IMGUI
if (NOT IMGUI_PATH)
    message(STATUS "IMGUI_PATH not specified in .env.cmake, using external/imgui")
    set(IMGUI_PATH external/imgui)
endif()

if (NOT MIKKTSPACE_PATH)
    message(STATUS "MIKKTSPACE_PATH not specified in .env.cmake, using external/mikktspace")
    set(MIKKTSPACE_PATH external/mikktspace)
endif()

if (NOT RAPIDJSON_PATH)
    message(STATUS "RAPIDJSON_PATH not specified in .env.cmake, using external/rapidjson")
    set(RAPIDJSON_PATH external/rapidjson)
endif()

# ---- Glslang libs ---------------------------------------------------------------------------------------------
# NOTE: Vulkan installation provides includes.
# TODO: glslang should probably *not* be linked in a release binary 
#       as shaders should be precompiled only - this link is pretty heavy.

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(GLSLANG_LIB_DIR "external/glslang/build/Windows/Debug/lib")
else()
    set(GLSLANG_LIB_DIR "external/glslang/build/Windows/Release/lib")
endif()

#if (NOT EXISTS ${GLSLANG_LIB_DIR})
#    message(FATAL_ERROR 
#        "Glslang libs not found at ${GLSLANG_LIB_DIR}. "
#        "Ensure setup.py was run, or manually download the libs manually otherwise."
#    )
#endif()

find_library(GENERIC_CODE_GEN_LIB NAMES GenericCodeGen GenericCodeGend
            PATHS ${GLSLANG_LIB_DIR} NO_DEFAULT_PATH)
find_library(GLSLANG_LIB NAMES glslang glslangd
            PATHS ${GLSLANG_LIB_DIR} NO_DEFAULT_PATH)
find_library(GLSLANG_DEFAULT_RESOURCE_LIMITS_LIB NAMES glslang-default-resource-limits glslang-default-resource-limitsd
            PATHS ${GLSLANG_LIB_DIR} NO_DEFAULT_PATH)
find_library(MACHINE_INDEPENDENT_LIB NAMES MachineIndependent MachineIndependentd
            PATHS ${GLSLANG_LIB_DIR} NO_DEFAULT_PATH)
find_library(OS_DEPENDENT_LIB NAMES OSDependent OSDependentd
            PATHS ${GLSLANG_LIB_DIR} NO_DEFAULT_PATH)
find_library(SPIRV_LIB NAMES SPIRV SPIRVd
            PATHS ${GLSLANG_LIB_DIR} NO_DEFAULT_PATH)
find_library(SPVREMAPPER_LIB NAMES SPVRemapper SPVRemapperd
            PATHS ${GLSLANG_LIB_DIR} NO_DEFAULT_PATH)
find_library(SPIRV_TOOLS_LIB NAMES SPIRV-Tools SPIRV-Toolsd
            PATHS ${GLSLANG_LIB_DIR} NO_DEFAULT_PATH)
find_library(SPIRV_TOOLS_OPT_LIB NAMES SPIRV-Tools-opt SPIRV-Tools-optd
            PATHS ${GLSLANG_LIB_DIR} NO_DEFAULT_PATH)

set(GLSLANG_LIBS
    ${GENERIC_CODE_GEN_LIB}
    ${GLSLANG_LIB}
    ${GLSLANG_DEFAULT_RESOURCE_LIMITS_LIB}
    ${MACHINE_INDEPENDENT_LIB}
    ${OS_DEPENDENT_LIB}
    ${SPIRV_LIB}
    ${SPVREMAPPER_LIB}
    ${SPIRV_TOOLS_LIB}
    ${SPIRV_TOOLS_OPT_LIB}
)

# ---- Sumire Binary ----------------------------------------------------------------------------------------------
SET(OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}/Debug")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}/Release") # VS uses no configuration suffix for releases
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIRECTORY}/Release")         #  But I've included it anyhow just in case.
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}/Debug")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}/Release")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIRECTORY}/Release")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}/Debug")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}/Release")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIRECTORY}/Release")

file(GLOB_RECURSE MIKKTSPACE_SOURCES ${MIKKTSPACE_PATH}/*.c)
file(GLOB_RECURSE IMGUI_SOURCES ${IMGUI_PATH}/*.cpp)
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp) # find cpp files (nested) -> store in ${SOURCES}
 
add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SOURCES} ${MIKKTSPACE_SOURCES})
 
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17) # c++ 17 standard
 
# set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

# ---- Includes and Linking ---------------------------------------------------------------------------------------
if (WIN32)
    message(STATUS "CREATING BUILD FOR WINDOWS")

    if (USE_MINGW)
    target_include_directories(${PROJECT_NAME} PUBLIC
        ${MINGW_PATH}/include
    )
    target_link_directories(${PROJECT_NAME} PUBLIC
        ${MINGW_PATH}/lib
    )
    endif()
 
    # Includes
    target_include_directories(${PROJECT_NAME} 
        PUBLIC
            ${Vulkan_INCLUDE_DIRS}
            ${TINYOBJ_PATH}
            ${TINYGLTF_PATH}
            ${IMGUI_PATH}
            ${MIKKTSPACE_PATH}
            ${RAPIDJSON_PATH}/include
            ${GLFW_INCLUDE_DIRS}
            ${GLM_PATH}
        PRIVATE 
            ${PROJECT_SOURCE_DIR}/src
    )
 
    # Link libraries to project
    target_link_directories(${PROJECT_NAME} PUBLIC
        ${Vulkan_LIBRARIES}
        ${GLFW_LIB}
        ${GLSLANG_LIB_DIR}
    )

    target_link_libraries(${PROJECT_NAME} PUBLIC
        glfw3 
        vulkan-1
        ${GLSLANG_LIBS}
    )

elseif (UNIX)
    message(STATUS "CREATING BUILD FOR UNIX")
    target_include_directories(${PROJECT_NAME} 
    PUBLIC
        ${TINYOBJ_PATH}
        ${TINYGLTF_PATH}
        ${IMGUI_PATH}
        ${MIKKTSPACE_PATH}
        ${RAPIDJSON_PATH}/include
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
    )
    target_link_libraries(${PROJECT_NAME} glfw ${Vulkan_LIBRARIES})

endif()
 
# ---- Custom Target SHADERS --------------------------------------------------------------------------------------
message(STATUS "CREATING SHADERS TARGET")
# Find all shader sources within shaders directory
# taken from VBlancos vulkan tutorial
# https://github.com/vblanco20-1/vulkan-guide/blob/all-chapters/CMakeLists.txt
find_program(GLSL_VALIDATOR glslangValidator HINTS 
    ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} 
    /usr/bin 
    /usr/local/bin 
    ${VULKAN_SDK_PATH}/Bin
    ${VULKAN_SDK_PATH}/Bin32
    $ENV{VULKAN_SDK}/Bin/ 
    $ENV{VULKAN_SDK}/Bin32/
)
 
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/shaders/*.comp"
)
 
foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    get_filename_component(FILE_DIR ${GLSL} DIRECTORY)
    set(SPIRV "${FILE_DIR}/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV} --target-env spirv1.3
        DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)
 
add_custom_target(
    Shaders
    DEPENDS ${SPIRV_BINARY_FILES}
)

# Deal with MSVC not building cmake custom targets automatically.
if (MSVC)
    add_dependencies(${PROJECT_NAME} Shaders)
endif()
