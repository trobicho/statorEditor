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
set(IMGUIFILE_DIALOG_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/ImGuiFileDialog)
include(FetchContent)

set(cpps
	srcs/main.cpp

  srcs/statorGui.cpp
  srcs/statorGuiSetup.cpp

  srcs/stator/statorNode.cpp
)

set(hpps
  srcs/statorGui.hpp

  srcs/guiInfo.hpp

  srcs/stator/data.hpp
  srcs/stator/statorNode.hpp
  srcs/stator/factory.hpp
)


FetchContent_Declare(ImNodeFlow
     GIT_REPOSITORY "https://github.com/Fattorino/ImNodeFlow.git"
     GIT_TAG "master"
     SOURCE_DIR ${IMNODEFLOW_DIR}
)
FetchContent_GetProperties(ImNodeFlow)
FetchContent_MakeAvailable(ImNodeFlow)


#FetchContent_Declare(ImGuiFileDialog
#     GIT_REPOSITORY "https://github.com/aiekick/ImGuiFileDialog.git"
#     GIT_TAG "master"
#     SOURCE_DIR ${IMGUIFILE_DIALOG_DIR}
#)
#FetchContent_GetProperties(ImGuiFileDialog)
#FetchContent_MakeAvailable(ImGuiFileDialog)

list(APPEND imnode_flow_sources
  ${IMNODEFLOW_DIR}/src/ImNodeFlow.cpp)

#list(APPEND imguifile_dialog_sources
#  ${IMGUIFILE_DIALOG_DIR}/ImGuiFileDialog.cpp)

add_executable(${PROJECT_NAME} ${cpps} ${hpps} ${imnode_flow_sources} ${IMGUIFILE_DIALOG_DIR}/ImGuiFileDialog.cpp)


target_link_libraries(${PROJECT_NAME} PRIVATE hephaestus)

include(FindVulkan)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)

include(${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)

find_package(Boost REQUIRED COMPONENTS json)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::json)

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
