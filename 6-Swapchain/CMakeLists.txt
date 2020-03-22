cmake_minimum_required(VERSION 2.6)

set(project_name 6-Swapchain)
project(${project_name})

set(HAVE_LIBC TRUE)
set(src
    Swapchain.cpp

)
 
set(headers

)

include_directories(../Vk-Helper/include)

add_executable(${project_name} ${src} ${headers})

target_link_libraries(${project_name} Vk-Helper)


find_package(Vulkan)

target_link_libraries(${project_name} SDL2main)
target_link_libraries(${project_name} SDL2-static)

if(Vulkan_FOUND)
	target_include_directories(${project_name} PRIVATE Vulkan::Vulkan)
	target_link_libraries(${project_name} Vulkan::Vulkan)
endif()