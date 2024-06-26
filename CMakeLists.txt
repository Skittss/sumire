cmake_minimum_required(VERSION 3.15.0)
set(NAME sumire)
project(${NAME} VERSION 0.1.0)

option(ENABLE_BUILD_ERRORS "Enable build warnings and errors." ON)

include(./.env.cmake OPTIONAL RESULT_VARIABLE LOCAL_ENV)
message(STATUS "Using local .env.cmake at: ${LOCAL_ENV}")
 
# Compiler choice
message(STATUS "Using ${CMAKE_GENERATOR}")
 
# ---- Vulkan Package --------------------------------------------------------------------------------------------
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
 
# ---- GLFW Package ----------------------------------------------------------------------------------------------
if (DEFINED GLFW_PATH)
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
  
# ---- External Lib Paths ----------------------------------------------------------------------------------------
# TINY OBJ LOADER
set(TINYOBJ_PATH external/tinyobjloader)
# TINY GLTF LOADER
set(TINYGLTF_PATH external/tinygltf)
# IMGUI
set(IMGUI_PATH external/imgui)
set(IMGUI_BACKEND_PATH external/imgui/backends)
# MIKKTSPACE
set(MIKKTSPACE_PATH external/MikkTSpace)
# RAPIDJSON
set(RAPIDJSON_PATH external/rapidjson)
# GLM
set(GLM_PATH external/glm)

# ---- Shader libs -----------------------------------------------------------------------------------------------
# Vulkan provides shader libs, so no need to find_library here.
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(SHADERC_LIBS shaderc_combinedd)
else()
    set(SHADERC_LIBS shaderc_combined)
endif()

# ---- Sumire Binary ---------------------------------------------------------------------------------------------
# Binary Output Dirs
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

# External Sources
file(GLOB MIKKTSPACE_SOURCES ${MIKKTSPACE_PATH}/*.c)
file(GLOB IMGUI_SOURCES ${IMGUI_PATH}/*.cpp)
file(GLOB IMGUI_BACKEND_SOURCES # Only use required Imgui Backends
    ${IMGUI_BACKEND_PATH}/imgui_impl_vulkan.cpp ${IMGUI_BACKEND_PATH}/imgui_impl_glfw.cpp)

# Sumire Sources
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp) # find cpp files (nested) -> store in ${SOURCES}
 
add_executable(${PROJECT_NAME} 
    ${SOURCES} 
    ${IMGUI_SOURCES} 
    ${IMGUI_BACKEND_SOURCES} 
    ${MIKKTSPACE_SOURCES}
)
 
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17) # c++ 17 standard
 
# set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

# ---- Includes and Linking --------------------------------------------------------------------------------------
if (WIN32)
    message(STATUS "Creating target Sumire (Windows)")

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
            ${IMGUI_BACKEND_PATH}
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
    )

    target_link_libraries(${PROJECT_NAME} PUBLIC
        glfw3 
        vulkan-1
        ${SHADERC_LIBS}
    )

elseif (UNIX)
    message(FATAL_ERROR "Building for UNIX is currently unsupported.")
endif()
 
# ---- Custom Target SHADERS -------------------------------------------------------------------------------------
message(STATUS "Creating target Shaders")
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
