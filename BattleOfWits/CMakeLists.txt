cmake_minimum_required(VERSION 3.20)
project(BattleOfWits VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use folders in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# FetchContent for third-party libraries
include(FetchContent)

# nlohmann/json
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

# cpp-httplib (header-only)
FetchContent_Declare(
  cpp-httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(cpp-httplib)

# Optional: SFML for GUI (support both module and config packages)
# Do not pin to 2.x; vcpkg installs SFML 3 which is not version-compatible with 2.x in CONFIG mode
find_package(SFML COMPONENTS Graphics Audio Window System CONFIG QUIET)

# Optional: OpenSSL for HTTPS support in httplib
find_package(OpenSSL QUIET)

# Sources
file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/include/*.h
  ${CMAKE_SOURCE_DIR}/include/*.hpp
)
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/*.cpp
)

add_library(battle_of_wits_lib ${PROJECT_HEADERS} ${PROJECT_SOURCES})
add_library(BattleOfWits::lib ALIAS battle_of_wits_lib)

target_include_directories(battle_of_wits_lib
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${cpp-httplib_SOURCE_DIR}
)

target_link_libraries(battle_of_wits_lib
  PRIVATE
    nlohmann_json::nlohmann_json
)

if(OpenSSL_FOUND)
  target_compile_definitions(battle_of_wits_lib PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
  target_link_libraries(battle_of_wits_lib PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

# Executable
add_executable(battle_of_wits src/main.cpp)

if(SFML_FOUND)
  # Enable SFML UI in both library and executable
  target_compile_definitions(battle_of_wits_lib PRIVATE HAS_SFML=1)
  target_compile_definitions(battle_of_wits PRIVATE HAS_SFML=1)
  if(TARGET SFML::Graphics)
    target_link_libraries(battle_of_wits_lib PUBLIC SFML::Graphics SFML::Audio SFML::Window SFML::System)
    target_link_libraries(battle_of_wits PRIVATE battle_of_wits_lib)
  else()
    target_link_libraries(battle_of_wits_lib PUBLIC sfml-graphics sfml-audio sfml-window sfml-system)
    target_link_libraries(battle_of_wits PRIVATE battle_of_wits_lib)
  endif()
else()
  target_link_libraries(battle_of_wits PRIVATE battle_of_wits_lib)
endif()

# Group sources for organization
source_group(TREE ${CMAKE_SOURCE_DIR}/include PREFIX "Header Files" FILES ${PROJECT_HEADERS})
source_group(TREE ${CMAKE_SOURCE_DIR}/src PREFIX "Source Files" FILES ${PROJECT_SOURCES})

# Install rules (optional)
install(TARGETS battle_of_wits RUNTIME DESTINATION bin)

# Copy assets directory next to the executable after build (so the app finds assets/logo.png)
add_custom_command(TARGET battle_of_wits POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:battle_of_wits>/assets"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:battle_of_wits>/assets"
  VERBATIM)
