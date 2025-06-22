cmake_minimum_required(VERSION 3.12)
project(Engine)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -Wall -Wextra")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_PREFIX_PATH "/usr/local/lib")
#find_package(Vulkan REQUIRED PATHS "/usr/local" NO_DEFAULT_PATH)
find_library(Vulkan NAMES vulkan REQUIRED)
find_package(glfw3 REQUIRED)

set(VulkanMemoryAllocator_DIR "libs/VulkanMemoryAllocator/build/install/share/cmake/VulkanMemoryAllocator/")

find_package(VulkanMemoryAllocator CONFIG REQUIRED)

find_package(X11 REQUIRED)
set(EXTRA_LIBS ${X11_LIBRARIES} Xxf86vm Xrandr Xi)

set(IMGUI_SOURCES
    libs/imgui/imgui.cpp
    libs/imgui/imgui_demo.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/backends/imgui_impl_vulkan.cpp
    libs/imgui/backends/imgui_impl_glfw.cpp  
)

set(BACKEND_SOURCES
    backend/glfw_backend.hpp
    backend/glfw_backend.cpp
  )

set(ENGINE_SOURCES
   engine/vulkan_renderer.hpp
   engine/vulkan_renderer.cpp
   engine/frame.hpp
   engine/frame.cpp
   engine/swapchain.hpp
   engine/swapchain.cpp
   engine/ResourceImage.hpp
   engine/ResourceImage.cpp
   engine/Buffer.hpp
   engine/Buffer.cpp
  )

set(PIPELINE_SOURCES
   pipelines/pipeline.hpp
   pipelines/pipeline.cpp
   )


set(TARGET_SOURCES
   targets/simpleMesh.hpp
   targets/simpleMesh.cpp
   targets/objLoader.hpp
   targets/objLoader.cpp
   )

set(APP_SOURCES
   App/app.hpp
   App/app.cpp
   App/camera.hpp
   App/camera.cpp
   )

set(CONFIG_SOURCES
   config/config.hpp
   config/Logger.hpp
   config/Logger.cpp
   )


add_executable(engine main.cpp ${BACKEND_SOURCES} ${ENGINE_SOURCES} ${PIPELINE_SOURCES} ${TARGET_SOURCES} ${APP_SOURCES} ${CONFIG_SOURCES} ${IMGUI_SOURCES}  ) 

target_include_directories(engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/backend 
                                          ${CMAKE_CURRENT_SOURCE_DIR}/engine 
                                          ${CMAKE_CURRENT_SOURCE_DIR}/pipelines
                                          ${CMAKE_CURRENT_SOURCE_DIR}/targets
                                          ${CMAKE_CURRENT_SOURCE_DIR}/config
                                          ${CMAKE_CURRENT_SOURCE_DIR}/App
                                          ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui
                                          ${Vulkan_INCLUDE_DIRS}
                                          )


                                       target_link_libraries(engine ${Vulkan} glfw dl pthread GPUOpen::VulkanMemoryAllocator ${EXTRA_LIBS} )


