cmake_minimum_required (VERSION 3.30)

project ("statorEditor")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "-g")
set(CMAKE_CXX_FLAGS "-g")
set(VCPKG_ROOT $ENV{VCPKG_ROOT})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(APP_ROOT ${CMAKE_CURRENT_LIST_DIR})

set(IMNODEFLOW_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/ImNodeFlow)
include(FetchContent)

set(cpps
	srcs/main.cpp

  srcs/statorGui.cpp
  srcs/statorGuiSetup.cpp
)

set(hpps
  srcs/statorGui.hpp

  srcs/guiInfo.hpp

  srcs/nodeGui.hpp

  srcs/stator/stator.hpp
)


FetchContent_Declare(ImNodeFlow
     GIT_REPOSITORY "https://github.com/Fattorino/ImNodeFlow.git"
     GIT_TAG "master"
     SOURCE_DIR ${IMNODEFLOW_DIR}
)
FetchContent_GetProperties(ImNodeFlow)
FetchContent_MakeAvailable(ImNodeFlow)

list(APPEND imnode_flow_sources
  ${IMNODEFLOW_DIR}/src/ImNodeFlow.cpp)

add_executable(${PROJECT_NAME} ${cpps} ${hpps} ${imnode_flow_sources})


target_link_libraries(${PROJECT_NAME} PRIVATE hephaestus)

include(FindVulkan)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)

find_package(Boost 1.67 REQUIRED COMPONENTS json serialization)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES} ${llvm_libs})

include(${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui::imgui)

find_package(libzip CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE libzip::zip)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

target_include_directories(${PROJECT_NAME}
	PRIVATE
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_BINARY_DIR}
  ${IMNODEFLOW_DIR}/include
	srcs
)
target_compile_definitions(${PROJECT_NAME} PRIVATE IMGUI_DEFINE_MATH_OPERATORS)
