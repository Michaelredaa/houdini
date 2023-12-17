cmake_minimum_required( VERSION 3.6 )

project( HDK_Project )

# CMAKE_PREFIX_PATH must contain the path to the toolkit/cmake subdirectory of
# the Houdini installation. See the "Compiling with CMake" section of the HDK
# documentation for more details, which describes several options for
# specifying this path.
set(PYTHON_VERSION "37") 
set(PYTHON_INCLUDE "${HFS}/python${PYTHON_VERSION}/include")
set(PYTHON_LIBRARIES "${HFS}/python${PYTHON_VERSION}/libs")

list(APPEND CMAKE_PREFIX_PATH "$ENV{HFS}/toolkit/cmake" )


# Add Python paths to the include and library directories
include_directories(${PYTHON_INCLUDE})
link_directories(${PYTHON_LIBRARIES})

# Locate Houdini's libraries and header files.
# Registers an imported library target named 'Houdini'.
find_package( Houdini REQUIRED )

set( library_name LOP_Grid )

# Code generation for the embedded DS file in LOP_Grid.C.
houdini_generate_proto_headers( FILES LOP_Grid.C )

# Add a library and its source files.
add_library( ${library_name} SHARED
    LOP_Grid.C
    LOP_Grid.h
)

# Link against the Houdini libraries, and add required include directories and
# compile definitions.
target_link_libraries( ${library_name} Houdini ${PYTHON_LIBRARIES})

# Include ${CMAKE_CURRENT_BINARY_DIR} for the generated header.
target_include_directories( ${library_name} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Sets several common target properties, such as the library's output directory.
houdini_configure_target( ${library_name} )
