CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

PROJECT(vACDM VERSION "2.0.0")
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
IF (MSVC)
    IF (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        STRING(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    ELSE ()
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    ENDIF ()
    IF (NOT CMAKE_CXX_FLAGS MATCHES "/MP")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
        SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /MP")
    ENDIF ()

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /sdl /permissive- /DNOMINMAX")
    SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /sdl /permissive- /DNOMINMAX")
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFESTUAC:NO /ignore:4099")
    ADD_DEFINITIONS(/D_USRDLL)
ENDIF ()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    ADD_DEFINITIONS(-DDEBUG_BUILD=1)
endif()

CONFIGURE_FILE(
    ${CMAKE_SOURCE_DIR}/src/Version.h.in
    ${CMAKE_BINARY_DIR}/Version.h
)

INCLUDE_DIRECTORIES(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src/
    ${CMAKE_SOURCE_DIR}/external/include
    ${CMAKE_SOURCE_DIR}
)

ADD_DEFINITIONS(
    -D_CRT_SECURE_NO_WARNINGS
    -DSQLITE_THREADSAFE=0
    -DSQLITE_DEFAULT_FILE_FORMAT=4
    -DSQLITE_DEFAULT_SYNCHRONOUS=0
    -DSQLITE_DEFAULT_WAL_SYNCHRONOUS=0
    -DSQLITE_WIN32_MALLOC
    -DSQLITE_THREADSAFE=0
)

SET(SOURCE_FILES
    src/com/Server.h
    src/com/Server.cpp
    src/com/Airport.h
    src/com/Airport.cpp
    src/config/ConfigParser.h
    src/config/ConfigParser.cpp
    src/core/TagItems.h
    src/core/TagItems.cpp
    src/core/TagItemsColor.h
    src/core/TagItemsColor.cpp
    src/log/Logger.h
    src/log/Logger.cpp
    src/utils/Date.h
    src/utils/Date.cpp
    src/vACDM.cpp
    src/vACDM.h
    src/main.cpp
    src/Version.h
)

ADD_LIBRARY(vACDM SHARED ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(vACDM ${CMAKE_SOURCE_DIR}/external/lib/EuroScopePlugInDLL.lib crypt32.lib ws2_32.lib Shlwapi.lib)
TARGET_LINK_LIBRARIES(vACDM debug ${CMAKE_SOURCE_DIR}/external/lib/jsoncpp_d.lib)
TARGET_LINK_LIBRARIES(vACDM debug ${CMAKE_SOURCE_DIR}/external/lib/libcurl-d.lib)
TARGET_LINK_LIBRARIES(vACDM debug ${CMAKE_SOURCE_DIR}/external/lib/Geographic_d.lib)
TARGET_LINK_LIBRARIES(vACDM optimized ${CMAKE_SOURCE_DIR}/external/lib/jsoncpp.lib)
TARGET_LINK_LIBRARIES(vACDM optimized ${CMAKE_SOURCE_DIR}/external/lib/libcurl.lib)
TARGET_LINK_LIBRARIES(vACDM optimized ${CMAKE_SOURCE_DIR}/external/lib/Geographic.lib)