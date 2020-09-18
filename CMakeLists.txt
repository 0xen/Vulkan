cmake_minimum_required(VERSION 2.6)

set(project_name Vulkan-Demo)
project(${project_name})

set(HAVE_LIBC TRUE)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

## Solution for placing in folder from https://stackoverflow.com/questions/45092198/cmake-how-do-i-change-properties-on-subdirectory-project-targets
set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)

function(get_all_targets _result _dir)
    get_property(_subdirs DIRECTORY "${_dir}" PROPERTY SUBDIRECTORIES)
    foreach(_subdir IN LISTS _subdirs)
        get_all_targets(${_result} "${_subdir}")
    endforeach()
    get_property(_sub_targets DIRECTORY "${_dir}" PROPERTY BUILDSYSTEM_TARGETS)
    set(${_result} ${${_result}} ${_sub_targets} PARENT_SCOPE)
endfunction()

function(add_subdirectory_with_folder _folder_name _folder)
    add_subdirectory(${_folder} ${ARGN})
    get_all_targets(_targets "${_folder}")
    foreach(_target IN LISTS _targets)
        set_target_properties(
            ${_target}
            PROPERTIES FOLDER "${_folder_name}"
        )
    endforeach()
endfunction()


add_subdirectory_with_folder("Third Party/SDL2" third_party/SDL2)


include_directories(third_party/SDL2/include)

add_subdirectory(third_party/glm)
include_directories(third_party/glm)

add_subdirectory("Vk-Helper")

# Basic
add_subdirectory_with_folder("GettingStarted" 0-GettingStarted/0-Instance)
add_subdirectory_with_folder("GettingStarted" 0-GettingStarted/1-PhysicalDevice)
add_subdirectory_with_folder("GettingStarted" 0-GettingStarted/2-Device)
add_subdirectory_with_folder("GettingStarted" 0-GettingStarted/3-CommandPools)
add_subdirectory_with_folder("GettingStarted" 0-GettingStarted/4-Buffers)
add_subdirectory_with_folder("GettingStarted" 0-GettingStarted/5-Descriptors)
add_subdirectory_with_folder("GettingStarted" 0-GettingStarted/6-Swapchain)
add_subdirectory_with_folder("GettingStarted" 0-GettingStarted/7-Renderpass)
add_subdirectory_with_folder("GettingStarted" 0-GettingStarted/8-Present)
add_subdirectory_with_folder("GettingStarted" 0-GettingStarted/9-GraphicsPipeline)
add_subdirectory_with_folder("GettingStarted" 0-GettingStarted/10-Texturing)
add_subdirectory_with_folder("GettingStarted" 0-GettingStarted/11-Camera)

# Advanced
add_subdirectory_with_folder("Advanced" 1-Advanced/0-IndirectDrawing)
add_subdirectory_with_folder("Advanced" 1-Advanced/1-ComputePipeline)
add_subdirectory_with_folder("Advanced" 1-Advanced/2-ComputeParticleSystem)
add_subdirectory_with_folder("Advanced" 1-Advanced/3-AccelerationStructure)
add_subdirectory_with_folder("Advanced" 1-Advanced/4-RaytracingPipeline)

