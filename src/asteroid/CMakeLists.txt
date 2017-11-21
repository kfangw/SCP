add_executable(asteroid
        main.cpp
        stellarkv.cpp
        )

target_link_libraries(asteroid
        rpc-layer
        scp
        ssl
        crypto
        )