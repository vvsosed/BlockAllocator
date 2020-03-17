cmake_minimum_required(VERSION 3.5)
project(BlockAlloc)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MAIN_SRC
    main.cpp
    BlocksAllocator.cpp )

add_executable(main ${MAIN_SRC})



