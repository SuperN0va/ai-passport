cmake_minimum_required(VERSION 3.16)
project(pokedex_preview C)
set(CMAKE_C_STANDARD 11)
if(NOT LVGL_SOURCE_DIR)
    message(FATAL_ERROR "Set LVGL_SOURCE_DIR to an LVGL 9.5.0 checkout")
endif()
set(APP_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
file(GLOB_RECURSE LVGL_SOURCES "${LVGL_SOURCE_DIR}/src/*.c")
add_library(lvgl STATIC ${LVGL_SOURCES})
target_include_directories(lvgl PUBLIC "${LVGL_SOURCE_DIR}" "${CMAKE_CURRENT_LIST_DIR}")
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE)
add_executable(pokedex_preview
    preview.c
    "${APP_ROOT}/main/dex_ui.c" "${APP_ROOT}/main/dex_model.c"
    "${APP_ROOT}/main/dex_data.c" "${APP_ROOT}/main/dex_sprite.c"
    "${APP_ROOT}/main/dex_visual_assets.c"
    "${APP_ROOT}/main/vendor/lodepng.c"
    "${APP_ROOT}/assets/fonts/dex_font_16.c"
    "${APP_ROOT}/assets/fonts/dex_font_20.c"
    "${APP_ROOT}/components/bsp/src/bsp_display_rounding.c")
target_include_directories(pokedex_preview PRIVATE "${APP_ROOT}/main" "${APP_ROOT}/components/bsp/src")
target_compile_definitions(pokedex_preview PRIVATE LODEPNG_NO_COMPILE_ENCODER LODEPNG_NO_COMPILE_DISK)
target_compile_options(pokedex_preview PRIVATE -Wall -Wextra -Werror)
target_link_libraries(pokedex_preview PRIVATE lvgl)
