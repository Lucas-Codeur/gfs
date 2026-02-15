file(GLOB_RECURSE GFS_HEADERS CONFIGURE_DEPENDS *.hpp)

add_library(gfs INTERFACE)
add_library(gfs::gfs ALIAS gfs)

target_include_directories(gfs
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)