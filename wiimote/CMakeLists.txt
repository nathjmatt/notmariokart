cmake_minimum_required(VERSION 3.28.3)

project(WiiMote
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "Wii Remote access and push via HTTP POST."
)

# Set C standard
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add cmake directory to module path for custom find modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Create the executable
add_executable(wiimote src/main.c)

# Add include directory
target_include_directories(wiimote PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# Find and link libcurl
find_package(CURL REQUIRED)
target_link_libraries(wiimote PRIVATE CURL::libcurl)

# Find and link wiiuse library
# If your wiiuse is installed to a custom location, set WIIUSE_ROOT:
#   cmake -DWIIUSE_ROOT=/path/to/wiiuse/install ..
find_package(WiiUse REQUIRED)
target_link_libraries(wiimote PRIVATE WiiUse::wiiuse)


# Optional: Add compiler flags for warnings
if(MSVC)
    target_compile_options(wiimote PRIVATE /W4)
else()
    target_compile_options(wiimote PRIVATE -O -Werror -Wall -Wextra -Wpedantic)
endif()

# Installation rules (optional)
install(TARGETS wiimote DESTINATION bin)

