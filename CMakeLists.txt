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
add_subdirectory("0-Instance")
add_subdirectory("1-PhysicalDevice")
add_subdirectory("2-Device")
add_subdirectory("3-CommandPools")
add_subdirectory("4-Buffers")
add_subdirectory("5-Descriptors")
add_subdirectory("6-Swapchain")
add_subdirectory("7-Renderpass")
add_subdirectory("8-Present")
add_subdirectory("9-GraphicsPipeline")
add_subdirectory("10-Texturing")
add_subdirectory("11-Camera")
add_subdirectory("12-IndirectDrawing")
add_subdirectory("13-ComputePipeline")
add_subdirectory("14-ComputeParticleSystem")

