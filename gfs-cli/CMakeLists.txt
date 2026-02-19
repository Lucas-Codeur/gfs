add_executable(gfs-cli
    main.cpp
)

target_link_libraries(gfs-cli
    PRIVATE
        gfs::gfs
)