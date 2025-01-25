find_package(ODBC REQUIRED)

if("x64" IN_LIST CMAKE_GENERATOR_PLATFORM)
   message (STATUS " arch is x64 ")
else()
   message (FATAL_ERROR  " arch is not supported")
endif()


set (TARGET_NAME "OdbcDetour")

# configure a header file to pass the version number only
configure_file(OdbcDetour.h.in OdbcDetour.h)

add_library( ${TARGET_NAME}  SHARED
               OdbcDetourApi.cpp
               OdbcDetourAPI.def
               Resource.h
               Resource.rc
               OdbcDetour.h
               Logging.cpp
               InfoTypeMapping.cpp
               SqlInfoType.cpp
               DllMain.cpp
)

target_compile_definitions(${TARGET_NAME} PUBLIC UNICODE)
target_include_directories(${TARGET_NAME} PUBLIC
                    "${PROJECT_BINARY_DIR}"
                    )

# state that anybody linking to us needs to include the current source dir, to find the include files
target_include_directories(${TARGET_NAME}
INTERFACE
 $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
 $<INSTALL_INTERFACE:include>
)

# use JadaOdbc_compiler_flags
target_link_libraries(${TARGET_NAME} PUBLIC JadaOdbc_compiler_flags)

target_link_libraries(${TARGET_NAME} PRIVATE  odbccp32.lib legacy_stdio_definitions.lib)

if(MSVC)
  target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

