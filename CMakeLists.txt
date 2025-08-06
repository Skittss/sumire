cmake_minimum_required(VERSION 3.15.0)

# ---- Project Meta ----------------------------------------------------------------------------------------------
set(NAME sumire)
project(
    ${NAME} 
    LANGUAGES C CXX
    VERSION 0.1.0
)

# ---- Sumire Directories ----------------------------------------------------------------------------------------
set(DISTRIB_DIR ${PROJECT_SOURCE_DIR}/distrib/)
set(ASSETS_DIR ${PROJECT_SOURCE_DIR})
set(SUMIRE_SRC_DIR ${PROJECT_SOURCE_DIR}/src/sumire)

# ---- Compiler Options ------------------------------------------------------------------------------------------

option(ENABLE_BUILD_ERRORS "Enable build warnings and errors." ON)

add_compile_definitions(ASSETS_ROOT="/")
add_compile_definitions(SUMI_NO_VALIDATION_LAYERS=1) # Turn off validation layers manually, if needed

include(./.env.cmake OPTIONAL RESULT_VARIABLE LOCAL_ENV)
message(STATUS "Using local .env.cmake at: ${LOCAL_ENV}")

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
set(TINYOBJ_PATH       external/tinyobjloader)     # TINY OBJ LOADER
set(TINYGLTF_PATH      external/tinygltf)          # TINY GLTF LOADER
set(IMGUI_PATH         external/imgui)             # IMGUI
set(IMGUI_BACKEND_PATH external/imgui/backends)    #  |-
set(MIKKTSPACE_PATH    external/MikkTSpace)        # MIKKTSPACE
set(RAPIDJSON_PATH     external/rapidjson)         # RAPIDJSON
set(GLM_PATH           external/glm)               # GLM

# ---- Shader libs -----------------------------------------------------------------------------------------------
# Vulkan provides shader libs, so no need to find_library here.
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(SHADERC_LIBS shaderc_combinedd)
else()
    set(SHADERC_LIBS shaderc_combined)
endif()

# ---- External Sources ------------------------------------------------------------------------------------------
SET(MIKKTSPACE_SOURCES
    "${MIKKTSPACE_PATH}/mikktspace.c")
SET(IMGUI_SOURCES
    "${IMGUI_PATH}/imgui.cpp"
    "${IMGUI_PATH}/imgui_draw.cpp"
    "${IMGUI_PATH}/imgui_tables.cpp"
    "${IMGUI_PATH}/imgui_widgets.cpp"
    "${IMGUI_PATH}/imgui_demo.cpp")
SET(IMGUI_BACKEND_SOURCES
    "${IMGUI_BACKEND_PATH}/imgui_impl_vulkan.cpp"
    "${IMGUI_BACKEND_PATH}/imgui_impl_glfw.cpp")

# ---- Sumire Sources --------------------------------------------------------------------------------------------
SET(SUMIRE_SOURCES
    "${SUMIRE_SRC_DIR}/common/external_impl/stb_image_impl.cpp"
    "${SUMIRE_SRC_DIR}/common/external_impl/stb_image_write_impl.cpp"
    "${SUMIRE_SRC_DIR}/common/external_impl/tinygltf_impl.cpp"
    "${SUMIRE_SRC_DIR}/common/external_impl/tinyobj_impl.cpp"
    "${SUMIRE_SRC_DIR}/config/sumi_config.cpp"
    "${SUMIRE_SRC_DIR}/core/graphics_pipeline/sumi_attachment.cpp"
    "${SUMIRE_SRC_DIR}/core/graphics_pipeline/sumi_buffer.cpp"
    "${SUMIRE_SRC_DIR}/core/graphics_pipeline/sumi_compute_pipeline.cpp"
    "${SUMIRE_SRC_DIR}/core/graphics_pipeline/sumi_descriptors.cpp"
    "${SUMIRE_SRC_DIR}/core/graphics_pipeline/sumi_device.cpp"
    "${SUMIRE_SRC_DIR}/core/graphics_pipeline/sumi_pipeline.cpp"
    "${SUMIRE_SRC_DIR}/core/graphics_pipeline/sumi_swap_chain.cpp "
    "${SUMIRE_SRC_DIR}/core/graphics_pipeline/sumi_texture.cpp"
    "${SUMIRE_SRC_DIR}/core/materials/sumi_material.cpp"
    "${SUMIRE_SRC_DIR}/core/models/mesh.cpp "
    "${SUMIRE_SRC_DIR}/core/models/node.cpp "
    "${SUMIRE_SRC_DIR}/core/models/sumi_model.cpp "
    "${SUMIRE_SRC_DIR}/core/models/vertex.cpp"
    "${SUMIRE_SRC_DIR}/core/profiling/cpu_profiler.cpp "
    "${SUMIRE_SRC_DIR}/core/profiling/gpu_profiler.cpp "
    "${SUMIRE_SRC_DIR}/core/rendering/general/sumi_camera.cpp"
    "${SUMIRE_SRC_DIR}/core/rendering/geometry/sumi_gbuffer.cpp"
    "${SUMIRE_SRC_DIR}/core/rendering/geometry/sumi_hzb.cpp"
    "${SUMIRE_SRC_DIR}/core/rendering/lighting/sumi_light.cpp"
    "${SUMIRE_SRC_DIR}/core/rendering/shadows/sumi_cascaded_shadow_map.cpp"
    "${SUMIRE_SRC_DIR}/core/rendering/shadows/sumi_shadow_cubemap_array.cpp"
    "${SUMIRE_SRC_DIR}/core/rendering/shadows/sumi_shadow_map_array.cpp"
    "${SUMIRE_SRC_DIR}/core/rendering/shadows/sumi_virtual_shadow_map.cpp"
    "${SUMIRE_SRC_DIR}/core/rendering/sumi_renderer.cpp"
    "${SUMIRE_SRC_DIR}/core/render_systems/deferred/deferred_mesh_rendersys.cpp "
    "${SUMIRE_SRC_DIR}/core/render_systems/depth_buffers/hzb_generator.cpp "
    "${SUMIRE_SRC_DIR}/core/render_systems/forward/mesh_rendersys.cpp "
    "${SUMIRE_SRC_DIR}/core/render_systems/high_quality_shadow_mapping/high_quality_shadow_mapper.cpp"
    "${SUMIRE_SRC_DIR}/core/render_systems/high_quality_shadow_mapping/hqsm_debugger.cpp"
    "${SUMIRE_SRC_DIR}/core/render_systems/post/post_processor.cpp "
    "${SUMIRE_SRC_DIR}/core/render_systems/world_ui/grid_rendersys.cpp "
    "${SUMIRE_SRC_DIR}/core/render_systems/world_ui/point_light_rendersys.cpp"
    "${SUMIRE_SRC_DIR}/core/shaders/shader_compiler.cpp "
    "${SUMIRE_SRC_DIR}/core/shaders/shader_manager.cpp "
    "${SUMIRE_SRC_DIR}/core/shaders/shader_source.cpp "
    "${SUMIRE_SRC_DIR}/core/shaders/shader_update_listener.cpp "
    "${SUMIRE_SRC_DIR}/core/windowing/sumi_window.cpp "
    "${SUMIRE_SRC_DIR}/core/sumire.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/data/armour/armour_list.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/data/bones/bone_cache_manager.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/data/kbf_data_manager.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/panels/bone_panel.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/panels/create_preset_group_from_bundle_panel.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/panels/create_preset_group_panel.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/panels/create_preset_panel.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/panels/edit_player_override_panel.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/panels/edit_preset_group_panel.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/panels/edit_preset_panel.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/panels/info_popup_panel.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/panels/player_list_panel.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/panels/preset_group_panel.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/panels/preset_panel.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/tabs/debug/debug_tab.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/tabs/editor/editor_tab.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/tabs/npc/npc_tab.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/tabs/player/player_tab.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/tabs/presets/presets_tab.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/tabs/preset_groups/preset_groups_tab.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/tabs/settings/settings_tab.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/tabs/about/about_tab.cpp "
    "${SUMIRE_SRC_DIR}/gui/prototypes/gui/kbf_window.cpp"
    "${SUMIRE_SRC_DIR}/gui/sumi_imgui.cpp "
    "${SUMIRE_SRC_DIR}/input/sumi_kbm_controller.cpp"
    "${SUMIRE_SRC_DIR}/loaders/gltf_loader.cpp "
    "${SUMIRE_SRC_DIR}/loaders/obj_loader.cpp"
    "${SUMIRE_SRC_DIR}/math/coord_space_converters.cpp "
    "${SUMIRE_SRC_DIR}/math/frustum_culling.cpp "
    "${SUMIRE_SRC_DIR}/math/view_space_depth.cpp "
    "${SUMIRE_SRC_DIR}/util/generate_mikktspace_tangents.cpp "
    "${SUMIRE_SRC_DIR}/util/gltf_interpolators.cpp "
    "${SUMIRE_SRC_DIR}/util/gltf_vulkan_flag_converters.cpp "
    "${SUMIRE_SRC_DIR}/util/relative_engine_filepath.cpp "
    "${SUMIRE_SRC_DIR}/util/rw_file_binary.cpp"
    "${SUMIRE_SRC_DIR}/watchers/fs_watcher_win.cpp"
    "${SUMIRE_SRC_DIR}/main.cpp"
)

# ---- Sumire Binary ---------------------------------------------------------------------------------------------
# Binary Output Dirs
SET(OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${OUTPUT_DIRECTORY}/Debug")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}/Release") # VS uses no configuration suffix for releases
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY         "${OUTPUT_DIRECTORY}/Release") #  But I've included it anyhow just in case.
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG   "${OUTPUT_DIRECTORY}/Debug")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}/Release")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY         "${OUTPUT_DIRECTORY}/Release")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   "${OUTPUT_DIRECTORY}/Debug")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}/Release")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY         "${OUTPUT_DIRECTORY}/Release")
 
add_executable(${PROJECT_NAME} 
    ${SUMIRE_SOURCES} 
    ${IMGUI_SOURCES} 
    ${IMGUI_BACKEND_SOURCES} 
    ${MIKKTSPACE_SOURCES})
 
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
 
# ---- Includes and Linking --------------------------------------------------------------------------------------
if (WIN32)
    message(STATUS "Building Sumire for Windows")

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
 
# ---- Shader Target ---------------------------------------------------------------------------------------------
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

# ---- Post build Packaging ---------------------------------------------------------------------------------------
# TODO