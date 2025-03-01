# Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>

if (NOT BUILD_MISCSCRIPTS)
    message("\tMiscScripts was skipped because BUILD_MISCSCRIPTS is unchecked.")
    return()
endif ()

# set up our project name
project(MiscScripts CXX)

# sources - contains source files for our library.
set(sources
    City_Dalaran.cpp
    Creatures.cpp
    GameObjects.cpp
    GameObjectTeleportTable.cpp
    RandomScripts.cpp
    Setup.cpp
    # required for linker
    ../../world/Server/ServerState.cpp
)

set(headers
    Setup.h
)

include_directories(
    ${OPENSSL_INCLUDE_DIR}
    ${PCRE_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/dep/recastnavigation/Detour/Include
    ${CMAKE_SOURCE_DIR}/src/collision
    ${CMAKE_SOURCE_DIR}/src/collision/Management
    ${CMAKE_SOURCE_DIR}/src/collision/Maps
    ${CMAKE_SOURCE_DIR}/src/collision/Models
    ${CMAKE_SOURCE_DIR}/dep/g3dlite/include
    ${CMAKE_SOURCE_DIR}/src/shared
    ${CMAKE_SOURCE_DIR}/src/world
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZLIB_INCLUDE_DIRS}
)
add_library(${PROJECT_NAME} SHARED ${sources} ${headers})

target_link_libraries(${PROJECT_NAME} world)

if (USE_PCH)
    ##gen_cxx_pch(${PROJECT_NAME} Setup.h ${CMAKE_CURRENT_SOURCE_DIR}/Setup.cpp)
endif ()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${ASCEMU_SCRIPTLIB_PATH} LIBRARY DESTINATION ${ASCEMU_SCRIPTLIB_PATH})

if (USE_PCH)
    target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    Setup.h
    <Server/Script/CreatureAIScript.h>
)
endif ()

unset(sources)
unset(headers)
