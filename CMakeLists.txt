cmake_minimum_required(VERSION 3.10)
project(RedisKeyLight VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(redis_server src/main.cpp src/parser.cpp src/store.cpp)
