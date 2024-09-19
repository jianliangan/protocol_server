cmake_minimum_required(VERSION 3.4.1)
project(huagertp C CXX)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/app src_app_LIST)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/base src_base_LIST)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/h264parse src_h264parse_LIST)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/common src_common_LIST)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/threads src_threads_LIST)

aux_source_directory(${PROJECT_SOURCE_DIR}/src/net src2_net_LIST)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/net/rtp src2_net_rtp_LIST)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/net/rtp/common sc2_net_rtp_common_LIST)

aux_source_directory(${PROJECT_SOURCE_DIR}/src/net/threads src2_net_thre_LIST)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/net/common src2_net_com_LIST)

MESSAGE( STATUS "----${APP_LIST}----")
include_directories(
	${PROJECT_SOURCE_DIR}/src
#    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
#CMAKE_MODULE_LINKER_FLAGS
#CMAKE_SHARED_LINKER_FLAGS
#CMAKE_STATIC_LINKER_FLAGS
if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -O0 -std=c++11 -fPIC -Wl,--rpath=lib -L. -Werror -g3 -gdwarf-2")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}   -O0 -fPIC -Wl,--rpath=lib -L. -Werror -g3 -gdwarf-2")
else()
    add_definitions(-D_SCL_SECURE_NO_WARNINGS
        D_CRT_SECURE_NO_WARNINGS
        -D_WIN32_WINNT=0x601
        -D_WINSOCK_DEPRECATED_NO_WARNINGS)
endif()
add_executable(huagertp ${src_app_LIST} ${src_base_LIST} ${src_h264parse_LIST} ${src_common_LIST} ${src_threads_LIST} ${src2_net_LIST} ${src2_net_rtp_LIST} ${sc2_net_rtp_common_LIST} ${src2_net_thre_LIST} ${src2_net_com_LIST}  ${PROJECT_SOURCE_DIR} ${sys_LIST} ./src/main.cpp)
target_link_libraries(huagertp pthread rt)
