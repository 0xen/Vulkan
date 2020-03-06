cmake_minimum_required(VERSION 2.6)

set(project_name 0-Instance)
project(${project_name})

set(HAVE_LIBC TRUE)
set(src
    Instance.cpp

)
 
set(headers

)


add_executable(${project_name} ${src} ${headers})

find_package(Vulkan)


if(Vulkan_FOUND)
	target_include_directories(${project_name} PRIVATE Vulkan::Vulkan)
	target_link_libraries(${project_name} Vulkan::Vulkan)
endif()