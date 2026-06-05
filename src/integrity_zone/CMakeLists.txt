include_directories(
  ${PostgreSQL_INCLUDE_DIRS}
  ${PostgreSQL_SERVER_INCLUDE_DIRS}
  include
  ../include
)

set(interface integrity_zone)

file(GLOB SRC
  "extension.cpp"
  "client.cpp"
  "int.cpp"
  "float.cpp"
  "text.cpp"
  "timestamp.cpp"
  "../utils/util.cpp"
  )

add_postgresql_extension(
  kvdb
  VERSION 0.1
  SOURCES ${SRC}
  SCRIPTS kvdb--0.1.sql
)
target_compile_definitions(kvdb PRIVATE KVDB_USE_POSTGRES_ELOG)

if(COMMAND target_precompile_headers)
  target_precompile_headers(kvdb PRIVATE include/stdafx.hpp)
endif()

add_dependencies(kvdb ${interface})
target_link_libraries(kvdb PRIVATE ${interface})
