cmake_minimum_required(VERSION 3.13)
project(linux_fs_integrity_os_assignment4 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

find_package(OpenSSL REQUIRED)
if (OPENSSL_FOUND)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()

add_executable(linux_fs_integrity_os_assignment4
        mytests/testMerkle.c
        base.c
        filesys.c
        filesys.h
        )


target_link_libraries(${PROJECT_NAME} OpenSSL::Crypto)