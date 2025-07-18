if(PROJECT_IS_TOP_LEVEL)
  set(
      CMAKE_INSTALL_INCLUDEDIR "include/dsk-${PROJECT_VERSION}"
      CACHE STRING ""
  )
  set_property(CACHE CMAKE_INSTALL_INCLUDEDIR PROPERTY TYPE PATH)
endif()

# Project is configured with no languages, so tell GNUInstallDirs the lib dir
set(CMAKE_INSTALL_LIBDIR lib CACHE PATH "")

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package dsk)

install(
    DIRECTORY include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT dsk_Development
)

install(
    TARGETS dsk_basic dsk_compr dsk_tbb dsk_grpc dsk_asio dsk_http dsk_curl dsk_pq dsk_mysql dsk_sqlite3 dsk_guni dsk_redis dsk_jser
    EXPORT dskTargets
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)

# Allow package maintainers to freely override the path for the configs
set(
    dsk_INSTALL_CMAKEDIR "${CMAKE_INSTALL_DATADIR}/${package}"
    CACHE STRING "CMake package config location relative to the install prefix"
)
set_property(CACHE dsk_INSTALL_CMAKEDIR PROPERTY TYPE PATH)
mark_as_advanced(dsk_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${dsk_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
    COMPONENT dsk_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${dsk_INSTALL_CMAKEDIR}"
    COMPONENT dsk_Development
)

install(
    EXPORT dskTargets
    NAMESPACE dsk::
    DESTINATION "${dsk_INSTALL_CMAKEDIR}"
    COMPONENT dsk_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
