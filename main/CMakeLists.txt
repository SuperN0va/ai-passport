idf_component_register(
    SRCS "dex_app.c" "dex_ui.c" "dex_model.c" "dex_data.c"
         "dex_sprite.c" "dex_visual_assets.c" "vendor/lodepng.c"
         "../assets/fonts/dex_font_16.c" "../assets/fonts/dex_font_20.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_timer nvs_flash
)
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    LODEPNG_NO_COMPILE_ENCODER LODEPNG_NO_COMPILE_DISK)
target_add_binary_data(${COMPONENT_LIB} "../assets/pokedex/sprites.bin" BINARY)
