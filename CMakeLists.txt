# GoldcoinPoP - The Greatest Money in the Galaxy
# Copyright (c) 2025 MicroGuy / Goldcoin Developers
# Modern C++23 build with CMake 4.1.0

cmake_minimum_required(VERSION 3.28)
project(GoldcoinPoP 
    VERSION 2.0.0
    DESCRIPTION "Goldcoin with Proof of Participation - Zero fees, AI autonomous"
    LANGUAGES CXX C
)

# Require C++23 and GCC 15.2
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler requirements
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.2")
        message(FATAL_ERROR "GCC 15.2 or higher required")
    endif()
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags for GCC 15.2
set(CMAKE_CXX_FLAGS_COMMON "-Wall -Wextra -Wpedantic -march=native -mtune=native")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fsanitize=address,undefined -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -flto")

# NO BOOST! We use modern C++ standard library instead
# Boost functionality replaced with:
# - std::filesystem (C++17)
# - std::chrono (C++11)
# - std::thread (C++11)
# - std::format (C++20)
# - std::expected (C++23)
# - std::ranges (C++20)

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Qt 6.9 for GUI
find_package(Qt6 6.9 REQUIRED COMPONENTS Core Widgets Network)
qt6_standard_project_setup()

# Berkeley DB 18.1.40
set(BDB_ROOT "/usr/local/BerkeleyDB.18.1")
find_path(BDB_INCLUDE_DIR db_cxx.h PATHS ${BDB_ROOT}/include)
find_library(BDB_LIBRARY db_cxx PATHS ${BDB_ROOT}/lib)

if(NOT BDB_INCLUDE_DIR OR NOT BDB_LIBRARY)
    message(FATAL_ERROR "Berkeley DB 18.1.40 not found at ${BDB_ROOT}")
endif()

# Source files - organized by component
set(CORE_SOURCES
    util.cpp
    script.cpp
    db.cpp
    net.cpp
    main.cpp
    init.cpp
    sha.cpp
    key.cpp
    serialize.cpp
)

set(POP_SOURCES
    participation.cpp
    vrf.cpp
    pop_security.cpp
    pop_checkpoints.cpp
    hardfork.cpp
)

set(FEATURES_SOURCES
    zerofee.cpp
    ai_autonomy.cpp
    consensus_preservation.cpp
)

set(RPC_SOURCES
    rpc.cpp
    rpcnet.cpp
    rpcblockchain.cpp
    rpcwallet.cpp
)

# Headers
set(HEADERS
    main_simplified.h
    uint256.h
    bignum.h
    serialize.h
    util.h
    key.h
    script.h
    db.h
    net.h
    sha.h
    participation.h
    vrf.h
    goldcoin.h
    goldcoin_specs.h
)

# Core daemon executable
add_executable(goldcoinpopd 
    ${CORE_SOURCES}
    ${POP_SOURCES}
    ${FEATURES_SOURCES}
    ${RPC_SOURCES}
)

target_include_directories(goldcoinpopd PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${BDB_INCLUDE_DIR}
)

target_link_libraries(goldcoinpopd PRIVATE
    ${BDB_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    stdc++fs  # std::filesystem
)

# Qt GUI executable
set(GUI_SOURCES
    qt/main_gui.cpp
    qt/mainwindow.cpp
    qt/sendcoinsdialog.cpp
    qt/addressbookpage.cpp
    qt/transactionview.cpp
    qt/overviewpage.cpp
    qt/participationdialog.cpp
    qt/zerofeewidget.cpp
    qt/aistatuswidget.cpp
)

qt6_add_resources(GUI_RESOURCES qt/goldcoin.qrc)

add_executable(goldcoinpop-qt
    ${GUI_SOURCES}
    ${GUI_RESOURCES}
    ${CORE_SOURCES}
    ${POP_SOURCES}
    ${FEATURES_SOURCES}
)

target_include_directories(goldcoinpop-qt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${BDB_INCLUDE_DIR}
)

target_link_libraries(goldcoinpop-qt PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    ${BDB_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    stdc++fs
)

# Installation
install(TARGETS goldcoinpopd goldcoinpop-qt
    RUNTIME DESTINATION bin
)

install(FILES
    doc/README.md
    doc/pop-whitepaper.md
    DESTINATION share/doc/goldcoinpop
)

# Testing
enable_testing()

add_executable(test_pop
    test/test_participation.cpp
    test/test_vrf.cpp
    test/test_zerofee.cpp
    ${POP_SOURCES}
    ${FEATURES_SOURCES}
)

target_link_libraries(test_pop PRIVATE
    ${BDB_LIBRARY}
    OpenSSL::Crypto
    Threads::Threads
)

add_test(NAME PopTests COMMAND test_pop)

# CPack for distribution
set(CPACK_PACKAGE_NAME "GoldcoinPoP")
set(CPACK_PACKAGE_VERSION "2.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The Greatest Money in the Galaxy")
set(CPACK_PACKAGE_VENDOR "MicroGuy")
set(CPACK_PACKAGE_CONTACT "microguy@goldcoin.org")

include(CPack)

# Print build configuration
message(STATUS "")
message(STATUS "GoldcoinPoP Build Configuration")
message(STATUS "================================")
message(STATUS "C++ Standard: C++23")
message(STATUS "Compiler: GCC ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Berkeley DB: ${BDB_ROOT}")
message(STATUS "Qt Version: 6.9")
message(STATUS "Boost: NOT USED (modern C++ instead)")
message(STATUS "")
message(STATUS "Features:")
message(STATUS "  - Proof of Participation consensus")
message(STATUS "  - Zero transaction fees")
message(STATUS "  - AI autonomy framework")
message(STATUS "  - VRF lottery selection")
message(STATUS "  - 99.99% energy reduction")
message(STATUS "")
message(STATUS "Philosophy: Channeling Satoshi's Vision")
message(STATUS "================================")