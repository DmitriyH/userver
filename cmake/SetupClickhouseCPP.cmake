option(USERVER_DOWNLOAD_PACKAGE_CLICKHOUSECPP "Download and setup clickhouse-cpp" ${USERVER_DOWNLOAD_PACKAGES})

if (NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  if (USERVER_DOWNLOAD_PACKAGE_CLICKHOUSECPP)
    find_package(clickhouse-cpp QUIET)
  else()
    find_package(clickhouse-cpp REQUIRED)
  endif()

  if (clickhouse-cpp_FOUND)
    return()
  endif()
endif()

include(DownloadUsingCPM)
CPMAddPackage(
    NAME clickhouse-cpp
    VERSION 2.4.0
    GITHUB_REPOSITORY ClickHouse/clickhouse-cpp
)

add_library(clickhouse-cpp ALIAS clickhouse-cpp-lib)
target_compile_options(absl_int128 PUBLIC -Wno-pedantic)
