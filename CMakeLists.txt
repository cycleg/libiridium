PROJECT(iridium)
CMAKE_MINIMUM_REQUIRED(VERSION 3.7)

OPTION(IRIDIUM_BUILD_SHARED "Build shared library, if on." OFF)
OPTION(IRIDIUM_BUILD_STATIC "Build static library, if on." ON)

IF((IRIDIUM_BUILD_SHARED AND IRIDIUM_BUILD_STATIC) OR (NOT IRIDIUM_BUILD_SHARED AND NOT IRIDIUM_BUILD_STATIC))
  MESSAGE(FATAL_ERROR "Build shared OR static library!")
ENDIF((IRIDIUM_BUILD_SHARED AND IRIDIUM_BUILD_STATIC) OR (NOT IRIDIUM_BUILD_SHARED AND NOT IRIDIUM_BUILD_STATIC))

SET(CMAKE_C_STANDARD 99)
SET(CMAKE_C_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(CMAKE_INSTALL_PREFIX "/usr/local")

IF(CMAKE_BUILD_TYPE STREQUAL "Release")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s")
ENDIF(CMAKE_BUILD_TYPE STREQUAL "Release")

SET(CMAKE_C_FLAGS "-pedantic -Wall -Wextra ${CMAKE_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "-pedantic -Wall -Wextra -Wnon-virtual-dtor -fstack-protector-all ${CMAKE_CXX_FLAGS}")

# GNU filesystem layout conventions
INCLUDE(GNUInstallDirs)
INCLUDE(FindThreads)
IF(EXISTS "/etc/debian_version")
    EXECUTE_PROCESS(COMMAND dpkg-architecture -q DEB_BUILD_GNU_TYPE
        OUTPUT_VARIABLE BUILD_HOST_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    IF(NOT (CMAKE_LIBRARY_ARCHITECTURE STREQUAL BUILD_HOST_ARCH))
        IF(IRIDIUM_BUILD_STATIC)
            SET(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/${CMAKE_LIBRARY_ARCHITECTURE}")
        ELSE()
            SET(CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}/${CMAKE_LIBRARY_ARCHITECTURE}")
        ENDIF()
    ENDIF()
ENDIF()

SET(IRIDIUM_VERSION "0.1.0")

SET(Boost_USE_MULTITHREADED ON)
IF(IRIDIUM_BUILD_STATIC)
    SET(Boost_USE_STATIC_LIBS ON)
ENDIF(IRIDIUM_BUILD_STATIC)
FIND_PACKAGE(Boost 1.62 REQUIRED)

SET(HEADERS
    include/iridium/Codec.hpp
    include/iridium/IEMoConfirmation.hpp
    include/iridium/IEMoHeader.hpp
    include/iridium/IEMoLocationInfo.hpp
    include/iridium/IEMoPayload.hpp
    include/iridium/IEMtConfirmationMsg.hpp
    include/iridium/IEMtHeader.hpp
    include/iridium/IEMtPayload.hpp
    include/iridium/IEMtPriority.hpp
    include/iridium/IncomingSbdSession.hpp
    include/iridium/InformationElement.hpp
    include/iridium/JobUnitQueue.hpp
    include/iridium/Message.hpp
    include/iridium/Modem.hpp
    include/iridium/SbdReceiver.hpp
    include/iridium/SbdTransmitter.hpp
)

SET(SOURCES
    src/Codec.cpp
    src/IEMoConfirmation.cpp
    src/IEMoHeader.cpp
    src/IEMoLocationInfo.cpp
    src/IEMoPayload.cpp
    src/IEMtConfirmationMsg.cpp
    src/IEMtHeader.cpp
    src/IEMtPayload.cpp
    src/IEMtPriority.cpp
    src/IncomingSbdSession.cpp
    src/InformationElement.cpp
    src/Message.cpp
    src/Modem.cpp
    src/SbdReceiver.cpp
    src/SbdTransmitter.cpp
)

ADD_LIBRARY(objlib OBJECT ${HEADERS} ${SOURCES})

TARGET_INCLUDE_DIRECTORIES(objlib PRIVATE
    ${Boost_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

IF(IRIDIUM_BUILD_SHARED)
    SET_PROPERTY(TARGET objlib PROPERTY POSITION_INDEPENDENT_CODE ON)
    ADD_LIBRARY(${PROJECT_NAME} SHARED $<TARGET_OBJECTS:objlib>)
    INSTALL(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
ENDIF(IRIDIUM_BUILD_SHARED)

IF(IRIDIUM_BUILD_STATIC)
    ADD_LIBRARY(${PROJECT_NAME}_static STATIC $<TARGET_OBJECTS:objlib>)
    SET_TARGET_PROPERTIES(${PROJECT_NAME}_static PROPERTIES
        OUTPUT_NAME ${PROJECT_NAME}
    )
    INSTALL(TARGETS ${PROJECT_NAME}_static
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )

    INSTALL(DIRECTORY include/${PROJECT_NAME} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

    SET(PKG_CONFIG_LIBS "-l${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT}")
    SET(PKG_CONFIG_LIBS_PRIVATE "${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY}")
    CONFIGURE_FILE("${PROJECT_NAME}.pc.in" "${PROJECT_NAME}.pc" @ONLY)
    INSTALL(
       FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
       DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
    )
ENDIF(IRIDIUM_BUILD_STATIC)

### CPack

SET(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
IF(IRIDIUM_BUILD_STATIC)
    SET(CPACK_PACKAGE_NAME "${PROJECT_NAME}-dev")
ENDIF(IRIDIUM_BUILD_STATIC)

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Iridium SBD exchange library")
SET(CPACK_PACKAGE_VENDOR "Alexei Golovin")
SET(CPACK_PACKAGE_CONTACT "golovin.alexei@gmail.com")
# SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
SET(CPACK_PACKAGE_VERSION "${IRIDIUM_VERSION}")
STRING(REGEX MATCHALL "[0-9]+|-([A-Za-z0-9_]+)" VERSION_PARTS ${CPACK_PACKAGE_VERSION})
LIST(GET VERSION_PARTS 0 CPACK_PACKAGE_VERSION_MAJOR)
LIST(GET VERSION_PARTS 1 CPACK_PACKAGE_VERSION_MINOR)
LIST(GET VERSION_PARTS 2 CPACK_PACKAGE_VERSION_PATCH)
SET(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})

FIND_PROGRAM(RPM_CMD rpmbuild)
IF(RPM_CMD)
    SET(CPACK_GENERATOR ${CPACK_GENERATOR};RPM)
ELSE(RPM_CMD)
    MESSAGE( STATUS "rpmbuild binary not found, not building RPM package" )
ENDIF(RPM_CMD)

IF(CPACK_GENERATOR STREQUAL "RPM") # linux (rpm)
    SET(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
    SET(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
        ${CMAKE_INSTALL_PREFIX}
        ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}
        ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
        ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/pkgconfig
    )
    SET(CPACK_RPM_PACKAGE_RELEASE 3)
    SET(CPACK_RPM_PACKAGE_LICENSE "MIT")
    # SET(CPACK_RPM_PACKAGE_REQUIRES "")
ENDIF(CPACK_GENERATOR STREQUAL "RPM")

FIND_PROGRAM(DPKG_CMD dpkg)
IF(DPKG_CMD)
    SET(CPACK_GENERATOR ${CPACK_GENERATOR} DEB)
ELSE(DPKG_CMD)
    MESSAGE( STATUS "dpkg binary not found, not building debian package" )
ENDIF(DPKG_CMD)

IF(CPACK_GENERATOR STREQUAL "DEB") # linux (deb)
    SET(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
    SET(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
    SET(CPACK_DEBIAN_PACKAGE_SECTION misc)
    SET(CPACK_DEBIAN_PACKAGE_MAINTAINER
        "Alexei Golovin <golovin.alexei@gmail.com>")
ENDIF(CPACK_GENERATOR STREQUAL "DEB")

INCLUDE(CPack)
