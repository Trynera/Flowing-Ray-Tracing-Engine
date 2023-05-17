cmake_minimum_required(VERSION 3.10)

# Project name
project(flowing_rendering_engine)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

set(SOURCE_FILES src/main.cpp src/simple_logger.cpp src/vkbase/vulkan_device.cpp src/vkbase/vulkan_swapchain.cpp src/vkbase/vulkan_renderpass.cpp src/vkbase/vulkan_pipeline.cpp src/vkbase/vulkan_utils.cpp src/utils/mesh.cpp)

# Find SDL2
add_subdirectory(libs/SDL2)

# Find GLM
add_subdirectory(libs/glm)

# Find tinyobjloader
add_subdirectory(libs/tinyobjloader)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Sets the variables for the Shaders (depending on your operating system)
if (UNIX)
add_custom_target(build_shaders ALL
    COMMAND "${PROJECT_SOURCE_DIR}/shaders/compile.sh"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/shaders"
)
endif (UNIX)

if (WIN32)
add_custom_target(build_shaders ALL
    COMMAND "${PROJECT_SOURCE_DIR}/shaders/compile.bat"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/shaders"
)
endif(WIN32)

# FRE Exectuable
add_executable(flowing_rendering_engine ${SOURCE_FILES})
add_dependencies(flowing_rendering_engine build_shaders)
target_include_directories(flowing_rendering_engine PUBLIC libs/SDL2/include)
target_link_libraries(flowing_rendering_engine PUBLIC SDL2-static)
target_include_directories(flowing_rendering_engine PUBLIC libs/glm)
target_link_libraries(flowing_rendering_engine PUBLIC glm::glm)
include_directories(PUBLIC libs/tinyobjloader)
target_include_directories(flowing_rendering_engine PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(flowing_rendering_engine PUBLIC ${Vulkan_LIBRARIES})