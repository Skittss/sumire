include(./.env.cmake OPTIONAL RESULT_VARIABLE LOCAL_ENV)
message(STATUS "Local .env.cmake: ${LOCAL_ENV}")
 
cmake_minimum_required(VERSION 3.11.0)
 
set(NAME sumire)
 
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
 
project(${NAME} VERSION 0.1.0)
 
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
		set(GLFW_LIB "${GLFW_PATH}/lib-vc2019") # Make sure this matches MSVC version
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
 
#include_directories(external)
 
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

file(GLOB_RECURSE MIKKTSPACE_SOURCES ${MIKKTSPACE_PATH}/*.c)
file(GLOB_RECURSE IMGUI_SOURCES ${IMGUI_PATH}/*.cpp)
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp) # find cpp files (nested) -> store in ${SOURCES}
 
add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SOURCES} ${MIKKTSPACE_SOURCES})
 
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17) # c++ 17 standard
 
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
 
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
			${GLFW_INCLUDE_DIRS}
			${GLM_PATH}
		PRIVATE 
			${PROJECT_SOURCE_DIR}/src
	)
 
	# Link libraries to probject
	target_link_directories(${PROJECT_NAME} PUBLIC
		${Vulkan_LIBRARIES}
		${GLFW_LIB}
	)
 
	target_link_libraries(${PROJECT_NAME} glfw3 vulkan-1)

elseif (UNIX)
	message(STATUS "CREATING BUILD FOR UNIX")
	target_include_directories(${PROJECT_NAME} 
	PUBLIC
		${TINYOBJ_PATH}
		${TINYGLTF_PATH}
		${IMGUI_PATH}
		${MIKKTSPACE_PATH}
	PRIVATE
		${PROJECT_SOURCE_DIR}/src
	)
	target_link_libraries(${PROJECT_NAME} glfw ${Vulkan_LIBRARIES})

endif()
 
 
############## Build SHADERS #######################
 
# Find all vertex and fragment sources within shaders directory
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
 
# get all .vert and .frag files in shaders directory
file(GLOB_RECURSE GLSL_SOURCE_FILES
	"${PROJECT_SOURCE_DIR}/shaders/*.frag"
	"${PROJECT_SOURCE_DIR}/shaders/*.vert"
)
 
foreach(GLSL ${GLSL_SOURCE_FILES})
	get_filename_component(FILE_NAME ${GLSL} NAME)
	get_filename_component(FILE_DIR ${GLSL} DIRECTORY)
	set(SPIRV "${FILE_DIR}/${FILE_NAME}.spv")
	add_custom_command(
		OUTPUT ${SPIRV}
		COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
		DEPENDS ${GLSL})
	list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)
 
add_custom_target(
	Shaders
	DEPENDS ${SPIRV_BINARY_FILES}
)