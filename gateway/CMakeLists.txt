cmake_minimum_required(VERSION 3.5)

project(scae-proto2-gateway LANGUAGES CXX VERSION 1.0.0 DESCRIPTION "Smart Card Authentication Enhancement Server")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#include_directories(${PROJECT_SOURCE_DIR}/libs/miracl/include)

set(SOURCES
    ../common/QTimings.cpp
    ../common/CommonUtils.cpp
    src/Gateway.cpp
    src/main.cpp
)

#find_library(
#    LIBMIRACL
#    NAMES miracl
#    PATHS ${PROJECT_SOURCE_DIR}/libs/miracl/lib
#    NO_DEFAULT_PATH
#)

find_package(Qt5 COMPONENTS Network REQUIRED)

add_executable(scae-proto2-gateway ${SOURCES})

include_directories(scae-proto2-gateway "include" "../common")

target_link_libraries(scae-proto2-gateway Qt5::Network)

include(GNUInstallDirs)

install(TARGETS scae-proto2-gateway
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
