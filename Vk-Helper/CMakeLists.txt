cmake_minimum_required(VERSION 2.6)

set(project_name Vk-Helper)
project(${project_name})

set(HAVE_LIBC TRUE)
set(src
    src/VkInitializers.cpp

)
 
set(headers
    include/VkInitializers.hpp
)

include_directories(include)

add_library(${project_name} STATIC ${src} ${headers})

find_package(Vulkan)

if(Vulkan_FOUND)
	target_include_directories(${project_name} PRIVATE Vulkan::Vulkan)
	target_link_libraries(${project_name} Vulkan::Vulkan)
endif()