if (EXISTS ${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/config.cmake)
    include(${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/config.cmake)
else()
    include(${PROJECT_SOURCE_DIR}/build/$ENV{CFG_BUILDPLATFORM}/${CMAKE_PROJECT_NAME}/config.cmake)
endif()

set(DIFF_NEW_PROJECT_PATH "${PROJECT_SOURCE_DIR}/build/$ENV{CFG_BUILDPLATFORM}/${CMAKE_PROJECT_NAME}/project/diff_project")

# create diff_project/new
file(REMOVE_RECURSE ${DIFF_NEW_PROJECT_PATH})

# copy bootloader
if((DEFINED CFG_UPGRADE_BOOTLOADER_EXTERNAL_PROJECT) AND (DEFINED CFG_UPGRADE_BOOTLOADER_EXTERNAL_PROJECT_PATH))
    set(bootloader_proj_name ${CFG_UPGRADE_BOOTLOADER_EXTERNAL_PROJECT_PATH})
else()
    set(bootloader_proj_name bootloader)
endif()
if(DEFINED CFG_UPGRADE_BOOTLOADER)
    file(MAKE_DIRECTORY ${DIFF_NEW_PROJECT_PATH}/bootloader)
    file(COPY ${PROJECT_SOURCE_DIR}/build/$ENV{CFG_BUILDPLATFORM}/${CMAKE_PROJECT_NAME}/project/${bootloader_proj_name}/kproc.sys DESTINATION ${DIFF_NEW_PROJECT_PATH}/bootloader)
endif()
# copy image
if(DEFINED CFG_UPGRADE_IMAGE)
    file(MAKE_DIRECTORY ${DIFF_NEW_PROJECT_PATH}/image)
    file(COPY ${PROJECT_SOURCE_DIR}/build/$ENV{CFG_BUILDPLATFORM}/${CMAKE_PROJECT_NAME}/project/${CMAKE_PROJECT_NAME}/kproc.sys DESTINATION ${DIFF_NEW_PROJECT_PATH}/image)
endif()
# copy data
if(DEFINED CFG_UPGRADE_DATA)
    file(MAKE_DIRECTORY ${DIFF_NEW_PROJECT_PATH}/data)
    file(COPY ${PROJECT_SOURCE_DIR}/build/$ENV{CFG_BUILDPLATFORM}/${CMAKE_PROJECT_NAME}/data DESTINATION ${DIFF_NEW_PROJECT_PATH}/)
endif()

#tar diff_project
if (EXISTS ${DIFF_NEW_PROJECT_PATH}/bootloader AND EXISTS ${DIFF_NEW_PROJECT_PATH}/image AND EXISTS ${DIFF_NEW_PROJECT_PATH}/data)
    set(TAR_OUTPUT_FILENAME "${PROJECT_SOURCE_DIR}/build/openrtos/${CMAKE_PROJECT_NAME}/project/${CMAKE_PROJECT_NAME}_${CFG_UPGRADE_PACKAGE_VERSION}")
    
    execute_process(
        COMMAND tar "cfv" "${TAR_OUTPUT_FILENAME}.tar" "bootloader" "image" "data"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/build/openrtos/${CMAKE_PROJECT_NAME}/project/diff_project"
    )
endif()
