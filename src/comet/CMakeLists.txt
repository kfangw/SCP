add_executable(comet
        main.cpp
        client.cpp
        server.cpp
        )
target_link_libraries(comet
        rpc-layer
        scp
        ssl
        crypto
        )