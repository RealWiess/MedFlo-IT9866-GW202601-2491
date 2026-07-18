#
# For Public domain
#
function(ite_sdk_append_library_ library)
    set_property(GLOBAL APPEND PROPERTY ITE_SDK_LIBS ${library})
endfunction()

macro(ite_sdk_add_library library)
    add_library(${library})
    target_link_libraries(${library} PUBLIC ite_sdk_interface)
    ite_sdk_append_library_(${library})
endmacro()

macro(ite_sdk_add_external_library library)
    if (TARGET ${library})
        get_target_property(imported ${library} IMPORTED)
        if (NOT ${imported})
            target_link_libraries(${library} PUBLIC ite_sdk_interface)
        endif()
        ite_sdk_append_library_(${library})
    endif()
endmacro()

function(ite_sdk_include_directories)
    foreach(dir ${ARGN})
        target_include_directories(ite_sdk_interface INTERFACE ${dir})
    endforeach()
endfunction()

function(ite_sdk_include_directories_ifdef feature_toggle item)
    if (DEFINED ${feature_toggle})
        target_include_directories(ite_sdk_interface INTERFACE ${item} ${ARGN})
    endif()
endfunction()

function(ite_sdk_include_directories_ifndef feature_toggle item)
    if (NOT DEFINED ${feature_toggle})
        target_include_directories(ite_sdk_interface INTERFACE ${item} ${ARGN})
    endif()
endfunction()

function(ite_sdk_compile_options item)
    target_compile_options(ite_sdk_interface INTERFACE ${item} ${ARGN})
endfunction()

function(ite_sdk_compile_options_ifdef feature_toggle item)
    if (DEFINED ${feature_toggle})
        target_compile_options(ite_sdk_interface INTERFACE ${item} ${ARGN})
    endif()
endfunction()

function(ite_sdk_compile_options_ifndef feature_toggle item)
    if (NOT DEFINED ${feature_toggle})
        target_compile_options(ite_sdk_interface INTERFACE ${item} ${ARGN})
    endif()
endfunction()

function(ite_sdk_compile_definitions item)
    target_compile_definitions(ite_sdk_interface INTERFACE ${item} ${ARGN})
endfunction()

function(ite_sdk_compile_definitions_ifdef feature_toggle item)
    if (DEFINED ${feature_toggle})
        target_compile_definitions(ite_sdk_interface INTERFACE ${item} ${ARGN})
    endif()
endfunction()

function(ite_sdk_compile_definitions_ifndef feature_toggle item)
    if (NOT DEFINED ${feature_toggle})
        target_compile_definitions(ite_sdk_interface INTERFACE ${item} ${ARGN})
    endif()
endfunction()

function(ite_sdk_ld_options)
    foreach(opt ${ARGN})
        target_link_libraries(ite_sdk_interface INTERFACE ${opt})
    endforeach()
endfunction()

#
# For certain target
#
function(target_sources_ifdef feature_toggle target scope item)
    if (DEFINED ${feature_toggle})
        target_sources(${target} ${scope} ${item} ${ARGN})
    endif()
endfunction()

function(target_sources_ifndef feature_toggle target scope item)
    if (NOT DEFINED ${feature_toggle})
        target_sources(${target} ${scope} ${item} ${ARGN})
    endif()
endfunction()

function(target_include_directories_ifdef feature_toggle target scope item)
    if (DEFINED ${feature_toggle})
        target_include_directories(${target} ${scope} ${item} ${ARGN})
    endif()
endfunction()

function(target_include_directories_ifndef feature_toggle target scope item)
    if (NOT DEFINED ${feature_toggle})
        target_include_directories(${target} ${scope} ${item} ${ARGN})
    endif()
endfunction()

function(target_compile_definitions_ifdef feature_toggle target scope item)
    if (DEFINED ${feature_toggle})
        target_compile_definitions(${target} ${scope} ${item} ${ARGN})
    endif()
endfunction()

function(target_compile_definitions_ifndef feature_toggle target scope item)
    if (NOT DEFINED ${feature_toggle})
        target_compile_definitions(${target} ${scope} ${item} ${ARGN})
    endif()
endfunction()

function(target_compile_options_ifdef feature_toggle target scope item)
    if (DEFINED ${feature_toggle})
        target_compile_options(${target} ${scope} ${item} ${ARGN})
    endif()
endfunction()

function(target_compile_options_ifndef feature_toggle target scope item)
    if (NOT DEFINED ${feature_toggle})
        target_compile_options(${target} ${scope} ${item} ${ARGN})
    endif()
endfunction()

function(target_link_libraries_ifdef feature_toggle target scope item)
    if (DEFINED ${feature_toggle})
        target_link_libraries(${target} ${scope} ${item} ${ARGN})
    endif()
endfunction()

function(target_link_libraries_ifndef feature_toggle target scope item)
    if (NOT DEFINED ${feature_toggle})
        target_link_libraries(${target} ${scope} ${item} ${ARGN})
    endif()
endfunction()

function(target_link_all_ite_sdk_libraries target)
    get_property(ITE_SDK_LIBS_PROPERTY GLOBAL PROPERTY ITE_SDK_LIBS)

    #include(CMakePrintHelpers)
    #cmake_print_variables(ITE_SDK_LIBS_PROPERTY)

    target_link_libraries(${target}
    PUBLIC
        ite_sdk_interface
        ${ITE_SDK_LIBS_PROPERTY}
    )
endfunction()

function(add_subdirectory_ifdef feature_toggle item)
    if (DEFINED ${feature_toggle})
        add_subdirectory(${target} ${item} ${ARGN})
    endif()
endfunction()

# usage example:
#   copy_build_scripts_for_complex_projects(sdk_name project_name_1 project_name_2)
macro(copy_build_scripts_for_complex_projects sdk_name)
    foreach(project_name IN ITEMS ${ARGN})
        # openrtos projects
        file(COPY
            ${PROJECT_SOURCE_DIR}/build/openrtos/${project_name}.cmd
            ${PROJECT_SOURCE_DIR}/build/openrtos/${project_name}_all.cmd
            DESTINATION ${CMAKE_BINARY_DIR}/${sdk_name}/build/openrtos
        )

        # linux projects
        file(COPY
            ${PROJECT_SOURCE_DIR}/build/linux/${project_name}.sh
            ${PROJECT_SOURCE_DIR}/build/linux/${project_name}_all.sh
            DESTINATION ${CMAKE_BINARY_DIR}/${sdk_name}/build/linux
        )
    endforeach()
endmacro()

# usage example:
#   copy_build_scripts_for_simple_projects(sdk_name project_name_1 project_name_2)
macro(copy_build_scripts_for_simple_projects sdk_name)
    foreach(project_name IN ITEMS ${ARGN})
        # openrtos projects
        file(COPY
            ${PROJECT_SOURCE_DIR}/build/openrtos/${project_name}.cmd
            DESTINATION ${CMAKE_BINARY_DIR}/${sdk_name}/build/openrtos
        )

        # linux projects
        file(COPY
            ${PROJECT_SOURCE_DIR}/build/linux/${project_name}.sh
            DESTINATION ${CMAKE_BINARY_DIR}/${sdk_name}/build/linux
        )
    endforeach()
endmacro()
