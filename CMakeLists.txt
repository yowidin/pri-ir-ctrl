cmake_minimum_required(VERSION 3.16)
project(cpp_project)

set(CMAKE_CXX_STANDARD 17)
set(BUILD_SHARED_LIBS OFF CACHE STRING "Build static!" FORCE)

include(FetchContent)
FetchContent_Declare(
   pigpio
   GIT_REPOSITORY https://github.com/joan2937/pigpio
   GIT_TAG        c33738a320a3e28824af7807edafda440952c05d
)
FetchContent_MakeAvailable(pigpio)

# include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
# conan_basic_setup(NO_OUTPUT_DIRS)

find_package(Threads REQUIRED)
find_package(Boost COMPONENTS system regex REQUIRED)

add_executable(ir-ctrl
   main.cpp

   src/button.cpp
   src/led.cpp
   src/wave.cpp
   src/necx.cpp
   src/server.cpp
)

target_link_libraries(ir-ctrl PRIVATE pigpio rt Threads::Threads Boost::system Boost::regex)
target_include_directories(ir-ctrl
   PRIVATE ${pigpio_SOURCE_DIR}
   include/
)
