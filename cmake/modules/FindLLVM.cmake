# Detect LLVM and set various variables to link against the different 
# components of LLVM
#
# NOTE: This is a modified version of the module originally found in the
# OpenGTL project at www.opengtl.org
#
# LLVM_BIN_DIR : directory with LLVM binaries
# LLVM_LIB_DIR : directory with LLVM library
# LLVM_INCLUDE_DIR : directory with LLVM include
#
# LLVM_COMPILE_FLAGS : compile flags to build a program using LLVM headers
# LLVM_LDFLAGS : ldflags needed to link
# LLVM_LIBS_CORE : ldflags needed to link against a LLVM core library

if (LLVM_INCLUDE_DIR)
  set(LLVM_FOUND TRUE)
else (LLVM_INCLUDE_DIR)

# Set up variables to point to an LLVM installation for the desired target
# (x86 or arm). For now, on Shannon builds, we build 32 vs 64 x86 based on the
# machine we're building on. Future work is to specify a 32/64-bit x86 build
# independent on the machine we're building on.

# Use uname to get build platform. The paths to the target LLVM builds have
# arm/x86/x86_64 in the name so munge to match our convention
EXEC_PROGRAM(uname ARGS -m OUTPUT_VARIABLE BUILD_PROCESSOR)
STRING(REGEX MATCH "^arm" IS_ARM_HOST ${BUILD_PROCESSOR})
if(IS_ARM_HOST)
  set(LLVM_HOST_PROCESSOR arm)
endif()

STRING(REGEX MATCH "^i686" IS_X86_HOST ${BUILD_PROCESSOR})
if(IS_X86_HOST)
  set(LLVM_HOST_PROCESSOR x86)
endif()

STRING(REGEX MATCH "^x86_64" IS_X86_64_HOST ${BUILD_PROCESSOR})
if(IS_X86_64_HOST)
   # For Hawking just use the x86 version when running clang
   if (HAWKING_BUILD)
     set(LLVM_HOST_PROCESSOR x86)
   else()
     set(LLVM_HOST_PROCESSOR x86_64)
   endif()      
endif()

# Version of LLVM we are currently based off of
set(LLVM_VERSION 360)

if (NOT SHAMROCK_BUILD)
# Set up llvm paths, using environment variables if defined
if ("$ENV{ARM_LLVM_DIR}" STREQUAL "")
   set(ARM_LLVM_DIR ${DEFAULT_DEV_INSTALL_DIR}/llvm${LLVM_VERSION}-install-arm)
   MESSAGE(STATUS "Environment variable ARM_LLVM_DIR not set. "
         "Assuming that the OpenCL ARM LLVM installation is at ${ARM_LLVM_DIR}")
else()
   set (ARM_LLVM_DIR $ENV{ARM_LLVM_DIR})
endif()

if (HAWKING_CROSS_COMPILE OR SHANNON)
if ("$ENV{X86_LLVM_DIR}" STREQUAL "")
   set (X86_LLVM_DIR ${DEFAULT_DEV_INSTALL_DIR}/llvm${LLVM_VERSION}-install-${LLVM_HOST_PROCESSOR})
   MESSAGE(STATUS "Environment variable X86_LLVM_DIR not set. "
         "Assuming that the OpenCL x86 LLVM installation is at ${X86_LLVM_DIR}")
else()
   set (X86_LLVM_DIR $ENV{X86_LLVM_DIR})
endif()
endif()

# Set llvm path to appropriate target llvm install
if (HAWKING_BUILD)
   set (LLVM_INSTALL_DIR ${ARM_LLVM_DIR})
elseif(SHANNON_BUILD) 
   set (LLVM_INSTALL_DIR ${X86_LLVM_DIR})
endif()
message(STATUS "LLVM installation is in ${LLVM_INSTALL_DIR}")
endif(NOT SHAMROCK_BUILD)

# Find appropriate llvm-config executable
if (HAWKING_CROSS_COMPILE)
  set(LLVM_CONFIG_NAME llvm-config-host)
else()
  set(LLVM_CONFIG_NAME llvm-config)
endif()

find_program(LLVM_CONFIG_EXECUTABLE
  NAMES ${LLVM_CONFIG_NAME}
  PATHS
  ${LLVM_INSTALL_DIR}/bin
  ${LLVM_CONFIG_PATH}
  /usr/bin
  /usr/local/bin
  /opt/local/bin
)

# Sanity check to ensure we're pointing at an LLVM version we think we are
exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --version OUTPUT_VARIABLE REPORTED_LLVM_VERSION )

STRING(REPLACE "." ""  REPORTED_LLVM_VERSION ${REPORTED_LLVM_VERSION})
if(NOT ${REPORTED_LLVM_VERSION} STREQUAL ${LLVM_VERSION})
    message(FATAL_ERROR  "ERROR!: llvm-config reports different version that what is expected \(${REPORTED_LLVM_VERSION} != ${LLVM_VERSION}" \))
endif()

# Macro to build up list of llvm libraries
MACRO(FIND_LLVM_LIBS LLVM_CONFIG_EXECUTABLE _libname_ LIB_VAR OBJECT_VAR)
  exec_program( ${LLVM_CONFIG_EXECUTABLE} ARGS --libs ${_libname_}  OUTPUT_VARIABLE ${LIB_VAR} )
  STRING(REGEX MATCHALL "[^ ]*[.]o[ $]"  ${OBJECT_VAR} ${${LIB_VAR}})
  SEPARATE_ARGUMENTS(${OBJECT_VAR})
  STRING(REGEX REPLACE "[^ ]*[.]o[ $]" ""  ${LIB_VAR} ${${LIB_VAR}})
ENDMACRO(FIND_LLVM_LIBS)


# Set up LLVM paths
exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --version OUTPUT_VARIABLE LLVM_VERSION )
exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --bindir OUTPUT_VARIABLE LLVM_BIN_DIR )
exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --libdir OUTPUT_VARIABLE LLVM_LIB_DIR )
exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --includedir OUTPUT_VARIABLE LLVM_INCLUDE_DIR )

MESSAGE(STATUS "LLVM VERSION " ${LLVM_VERSION})
MESSAGE(STATUS "LLVM BIN DIR " ${LLVM_BIN_DIR})
MESSAGE(STATUS "LLVM LIB DIR " ${LLVM_LIB_DIR})
MESSAGE(STATUS "LLVM INCLUDE_DIR DIR " ${LLVM_INCLUDE_DIR})

# Set up compile/link flags
exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --ldflags   OUTPUT_VARIABLE LLVM_LDFLAGS )
STRING(REPLACE " -lz" ""  LLVM_LDFLAGS ${LLVM_LDFLAGS})


exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --cxxflags  OUTPUT_VARIABLE LLVM_COMPILE_FLAGS )

STRING(REPLACE " -fno-rtti" ""       LLVM_COMPILE_FLAGS ${LLVM_COMPILE_FLAGS})
STRING(REPLACE " -fno-exceptions" "" LLVM_COMPILE_FLAGS ${LLVM_COMPILE_FLAGS})
STRING(REPLACE " -Wcast-qual" ""     LLVM_COMPILE_FLAGS ${LLVM_COMPILE_FLAGS})

# Do a case insensitive check for "Debug" build, then remove "-O3" if so:
SET( temp ${CMAKE_BUILD_TYPE})
STRING(TOLOWER "${temp}" temp_lower)
if( temp_lower STREQUAL "debug" )
    STRING(REPLACE " -O3" ""         LLVM_COMPILE_FLAGS ${LLVM_COMPILE_FLAGS})
ENDIF(temp_lower STREQUAL "debug" )

MESSAGE(STATUS "LLVM CXX flags: " ${LLVM_COMPILE_FLAGS})
MESSAGE(STATUS "LLVM LD flags: " ${LLVM_LDFLAGS})


# Generate list of LLVM libraries to link against
if (SHANNON_BUILD)
  set (LLVM_LIB_TARGET X86)
elseif(HAWKING_BUILD OR SHARMROCK_BUILD)
  set (LLVM_LIB_TARGET ARM)
endif()

exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --libs  ${LLVM_LIB_TARGET} asmparser native bitwriter tablegen mcjit debuginfo interpreter linker irreader instrumentation ipo mcdisassembler option objcarcopts profiledata OUTPUT_VARIABLE LLVM_LIBS_CORE )
MESSAGE(STATUS "LLVM core libs: " ${LLVM_LIBS_CORE})

if(LLVM_INCLUDE_DIR)
  set(LLVM_FOUND TRUE)
endif(LLVM_INCLUDE_DIR)

if(LLVM_FOUND)
  message(STATUS "Found LLVM: ${LLVM_INCLUDE_DIR}")
else(LLVM_FOUND)
  if(LLVM_FIND_REQUIRED)
    message(FATAL_ERROR "Could NOT find LLVM")
  endif(LLVM_FIND_REQUIRED)
endif(LLVM_FOUND)

endif (LLVM_INCLUDE_DIR)
