add_executable(gfs-cli
    main.cpp
    main.hpp

    commands.cpp
)

target_link_libraries(gfs-cli
    PRIVATE
        gfs::gfs
)