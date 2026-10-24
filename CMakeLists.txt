cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version} LANGUAGES CXX)

set(LIBHEIF_ROOT "" CACHE PATH "Path to a libheif installation")

set(ENABLE_FRONTEND_API ON)
set(ENABLE_QT ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
if(LIBHEIF_ROOT)
  list(PREPEND CMAKE_PREFIX_PATH "${LIBHEIF_ROOT}")
endif()
find_package(libheif CONFIG REQUIRED)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/plugin-main.cpp)
target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_17)
target_link_libraries(
  ${CMAKE_PROJECT_NAME}
  PRIVATE OBS::libobs OBS::obs-frontend-api Qt6::Core Qt6::Gui Qt6::Widgets heif
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
