cmake_minimum_required(VERSION 3.22)
project(S2NEAndroid LANGUAGES C CXX)

set(S2NE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
set(S2NE_DEPENDENCIES "" CACHE PATH "Android dependency prefix (arm64-android-dynamic)")
set(S2NE_JSON_INCLUDE "" CACHE PATH "Directory containing nlohmann/json.hpp")
if(NOT EXISTS "${S2NE_DEPENDENCIES}/include/SDL3/SDL.h")
    message(FATAL_ERROR "Set S2NE_DEPENDENCIES to the Android dependency prefix")
endif()
if(NOT EXISTS "${S2NE_JSON_INCLUDE}/nlohmann/json.hpp")
    message(FATAL_ERROR "Set S2NE_JSON_INCLUDE to the nlohmann-json include directory")
endif()
foreach(library IN ITEMS SDL3 SDL3_ttf SDL3_mixer fluidsynth)
    add_library(${library} SHARED IMPORTED)
    set_target_properties(${library} PROPERTIES IMPORTED_LOCATION "${S2NE_DEPENDENCIES}/lib/lib${library}.so")
endforeach()
add_library(main SHARED
    "${S2NE_ROOT}/src/main.cpp"
    "${S2NE_ROOT}/src/app_runtime.cpp"
    "${S2NE_ROOT}/src/app_sdl.cpp"
    "${S2NE_ROOT}/src/app_options.cpp"
    "${S2NE_ROOT}/src/app_campaign.cpp"
    "${S2NE_ROOT}/src/ZipFile2.cpp"
    "${S2NE_ROOT}/src/filefunc.cpp")
target_compile_features(main PRIVATE cxx_std_23)
target_include_directories(main PRIVATE "${S2NE_ROOT}/src" "${S2NE_DEPENDENCIES}/include" "${S2NE_JSON_INCLUDE}")
target_link_libraries(main PRIVATE SDL3 SDL3_ttf SDL3_mixer fluidsynth android log dl z)
target_link_options(main PRIVATE "-Wl,-z,max-page-size=16384")
