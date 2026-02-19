cmake_minimum_required(VERSION 3.12)

# Inspired by https://github.com/g-truc/glm/blob/master/CMakeLists.txt because I had no clue how to make clean versioning
file(READ "gfs/gfs.hpp" GFS_LIB_FILE)

string(REGEX MATCH "uint32_t[ ]+GFS_VERSION_PATCH[ ]+=+[ ]+([0-9]+)" _ ${GFS_LIB_FILE})
set(GFS_VERSION_PATCH "${CMAKE_MATCH_1}")

string(REGEX MATCH "uint32_t[ ]+GFS_VERSION_MINOR[ ]+=+[ ]+([0-9]+)" _ ${GFS_LIB_FILE})
set(GFS_VERSION_MINOR "${CMAKE_MATCH_1}")

string(REGEX MATCH "uint32_t[ ]+GFS_VERSION_MAJOR[ ]+=+[ ]+([0-9]+)" _ ${GFS_LIB_FILE})
set(GFS_VERSION_MAJOR "${CMAKE_MATCH_1}")

set(GFS_VERSION ${GFS_VERSION_MAJOR}.${GFS_VERSION_MINOR}.${GFS_VERSION_PATCH})
project(gfs VERSION ${GFS_VERSION} LANGUAGES CXX)
message(STATUS "GFS : version " ${GFS_VERSION})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(gfs)
add_subdirectory(gfs-cli)