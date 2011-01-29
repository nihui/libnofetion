project(nofetion)
cmake_minimum_required(VERSION 2.8)

find_package(OpenSSL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MINIXML REQUIRED mxml)

include_directories(${OPENSSL_INCLUDE_DIR} ${MINIXML_INCLUDE_DIRS})

add_definitions(-Wall -Wextra)

set(nofetion_SRCS
    main.c
    httppacket.c
    sipevent.c
    session.c
    sipcnet.c
    ssicnet.c
    contact.c
    user.c
    group.c
)

add_executable(nofetion ${nofetion_SRCS})
target_link_libraries(nofetion ${OPENSSL_LIBRARIES} ${MINIXML_LIBRARIES})
#${NCURSES_LIBRARIES})
