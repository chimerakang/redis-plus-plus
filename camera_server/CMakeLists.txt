project(camera_server)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    cmake_minimum_required(VERSION 3.0.0)
else()
    cmake_minimum_required(VERSION 2.8.0)
endif()

if (NOT DEFINED CAMERA_SERVER_CXX_STANDARD)
    set(CAMERA_SERVER_CXX_STANDARD 17)
endif()

message(STATUS "CAMERA_SERVER_CXX_STANDARD CXX standard is c++${CAMERA_SERVER_CXX_STANDARD}")

if (NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++${CAMERA_SERVER_CXX_STANDARD} ")
endif()


#set(PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/sw/redis++)

#file(GLOB PROJECT_SOURCE_FILES "${PROJECT_SOURCE_DIR}/*.cpp")

#add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_FILES})

message(STATUS "The CXX standard is c++${REDIS_PLUS_PLUS_CXX_STANDARD}")

# opencv dependency
find_package(OpenCV REQUIRED core opencv_imgcodecs opencv_highgui opencv_imgproc opencv_videoio)
if (NOT OPENCV_FOUND)
    message(FATAL_ERROR, "OpenCV not found.")
endif()

# hiredis dependency
# libhiredis dependency
# find_path(HIREDIS_HEADER hiredis)
# find_library(HIREDIS_LIB hiredis)

#INCLUDE("${CMAKE_CURRENT_LIST_DIR}/../cmake/hiredis.cmake")
#target_include_directories(${PROJECT_NAME} PUBLIC ${HIREDIS_INCLUDE_DIRS})
#target_link_libraries(${PROJECT_NAME} ${HIREDIS_LIBRARIES})

find_package(hiredis REQUIRED)
if (NOT HIREDIS_FOUND)
    message(FATAL_ERROR, "hiredis not found.")
endif()

find_package(ev REQUIRED)
if (NOT EV_FOUND)
    message(FATAL_ERROR, "ev not found.")
endif()

if (REDIS_PLUS_PLUS_USE_TLS)
    find_package(OpenSSL REQUIRED)
    find_library(HIREDIS_TLS_STATIC_LIB libhiredis_ssl.a)
    target_link_libraries(${PROJECT_NAME} ${HIREDIS_TLS_STATIC_LIB} ${OPENSSL_LIBRARIES})
endif()

# redis++ dependency
#target_include_directories(${PROJECT_NAME} PUBLIC ../src/sw/redis++)

# solaris socket dependency
if (CMAKE_SYSTEM_NAME MATCHES "(Solaris|SunOS)")
    target_link_libraries(${PROJECT_NAME} -lsocket)
endif(CMAKE_SYSTEM_NAME MATCHES "(Solaris|SunOS)")

# Windows socket dependency
if (WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32)
endif()

find_package(Threads REQUIRED)

find_package(ev REQUIRED)
if (NOT EV_FOUND)
    message(FATAL_ERROR "ev not found.")
endif()    

find_package(alpr REQUIRED)
if (NOT ALPR_FOUND)
    message(FATAL_ERROR "alpr not found.")
endif()

find_package(mysql REQUIRED)
if (NOT MYSQL_FOUND)
    message(FATAL_ERROR "mysql not found.")
endif()


if (REDIS_PLUS_PLUS_BUILD_STATIC)
    set(REDIS_PLUS_PLUS_LIB redis-plus-plus-static)
else()
    set(REDIS_PLUS_PLUS_LIB redis-plus-plus-shared)
endif()

set(GLOB HEADERS ../src/sw/redis++/image_helper/*.h)	

set(CLI_SOURCES
        client.cpp
        MJPEGWriter.cpp
        MJPEGWriter.h
        json.hpp
        Config.hpp
        Config.cpp
        ConfigFile.hpp
        ConfigFile.cpp
        Logging.hpp
        concurrentqueue.h
        SSocket.h
        SSocket.cpp
        yazi-mysql/mysql/Database.cpp
        yazi-mysql/mysql/Row.cpp
        yazi-mysql/mysql/Table.cpp
        yazi-mysql/mysql/Database.h
        yazi-mysql/mysql/Row.h
        yazi-mysql/mysql/Table.h
	)

set(SERV_SOURCES
        RedisCameraServer.cpp
        server.cpp
        RedisCameraServer.h)

add_executable (redis-camera-client ${CLI_SOURCES} ${HEADERS} )
target_link_libraries(redis-camera-client ${OpenCV_LIBS} ${REDIS_PLUS_PLUS_LIB} ${HIREDIS_LIBS} ${EV_LIBS} ${ALPR_LIBS} ${MYSQL_LIBS} curl pthread  )

add_executable (redis-camera-server ${SERV_SOURCES} ${HEADERS})
target_link_libraries(redis-camera-server ${OpenCV_LIBS} ${REDIS_PLUS_PLUS_LIB} ${HIREDIS_LIBS} ${EV_LIBS})
