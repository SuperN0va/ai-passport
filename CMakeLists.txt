cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# This application deliberately retains the provisioned board's Recovery entry.
idf_build_set_property(BOOTLOADER_EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/bootloader_components/" APPEND)

project(FoloToy-AI-Passport)
