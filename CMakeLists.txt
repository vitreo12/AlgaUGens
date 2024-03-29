####################################################################################################
# CMakeLists file for AlgaUGens
# Generated by Francesco Cameli
# 2021-10-20
####################################################################################################

####################################################################################################
# basic project config
cmake_minimum_required(VERSION 3.12)
set(project_name "AlgaUGens")
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_modules ${CMAKE_MODULE_PATH})
set(CMAKE_CXX_STANDARD 17)

####################################################################################################
# load modules
include(SuperColliderServerPlugin RESULT_VARIABLE server_plugin_found)
if(NOT server_plugin_found)
    message(FATAL_ERROR "Could not find server plugin functions module")
endif()

include(SuperColliderCompilerConfig RESULT_VARIABLE compiler_config_found)
if(NOT compiler_config_found)
    message(FATAL_ERROR "Could not find compiler config module")
endif()

# Windows - puts redistributable DLLs in install directory
include(InstallRequiredSystemLibraries)

sc_check_sc_path("${SC_PATH}")
message(STATUS "Found SuperCollider: ${SC_PATH}")
set(SC_PATH "${SC_PATH}" CACHE PATH
    "Path to SuperCollider source. Relative paths are treated as relative to this script" FORCE)

include("${SC_PATH}/SCVersion.txt")
message(STATUS "Building plugins for SuperCollider version: ${SC_VERSION}")

# set project here to avoid SCVersion.txt clobbering our version info
project(${project_name})
sc_do_initial_compiler_config() # do after setting project so compiler ID is available

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR NOT CMAKE_INSTALL_PREFIX)
    message(WARNING "No install prefix provided, defaulting to $BUILD_DIR/install/AlgaUGens")
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install/AlgaUGens" CACHE PATH "Install prefix" FORCE)
endif()

message(STATUS "Install directory set to: ${CMAKE_INSTALL_PREFIX}")

####################################################################################################
# options
option(SUPERNOVA "Build plugins for supernova" ON)
option(SCSYNTH "Build plugins for scsynth" ON)
option(NATIVE "Optimize for native architecture" OFF)
option(STRICT "Use strict warning flags" OFF)
option(NOVA_SIMD "Build plugins with nova-simd support." ON)

####################################################################################################
# include libraries

if (NOVA_SIMD)
	add_definitions(-DNOVA_SIMD)
	include_directories(${SC_PATH}/external_libraries/nova-simd)
endif()

####################################################################################################
# Begin target AlgaAudioControl

set(AlgaAudioControl_cpp_files
    plugins/AlgaAudioControl/AlgaAudioControl.cpp
)
set(AlgaAudioControl_sc_files
    plugins/AlgaAudioControl/AlgaAudioControl.sc
)
set(AlgaAudioControl_schelp_files
    plugins/AlgaAudioControl/AlgaAudioControl.schelp
)
set(AlgaAudioControl_systemoverwrites_files
    plugins/AlgaAudioControl/SystemOverwrites/extAudioControl.sc
)

sc_add_server_plugin(
    "AlgaAudioControl/AlgaAudioControl" # desination directory
    "AlgaAudioControl" # target name
    "${AlgaAudioControl_cpp_files}"
    "${AlgaAudioControl_sc_files}"
    "${AlgaAudioControl_schelp_files}"
    "${AlgaAudioControl_systemoverwrites_files}"
)

# End target AlgaAudioControl
####################################################################################################

####################################################################################################
# Begin target AlgaDynamicIEnvGen

set(AlgaDynamicIEnvGen_cpp_files
    plugins/AlgaDynamicIEnvGen/AlgaDynamicIEnvGen.cpp
)
set(AlgaDynamicIEnvGen_sc_files
    plugins/AlgaDynamicIEnvGen/AlgaDynamicIEnvGen.sc
)
set(AlgaDynamicIEnvGen_schelp_files
    plugins/AlgaDynamicIEnvGen/AlgaDynamicIEnvGen.schelp
)

sc_add_server_plugin(
    "AlgaDynamicIEnvGen/AlgaDynamicIEnvGen" # desination directory
    "AlgaDynamicIEnvGen" # target name
    "${AlgaDynamicIEnvGen_cpp_files}"
    "${AlgaDynamicIEnvGen_sc_files}"
    "${AlgaDynamicIEnvGen_schelp_files}"
    ""
)

# End target AlgaDynamicIEnvGen
####################################################################################################

####################################################################################################
# Begin target AlgaDynamicIEnvGenBuf

set(AlgaDynamicIEnvGenBuf_cpp_files
    plugins/AlgaDynamicIEnvGenBuf/AlgaDynamicIEnvGenBuf.cpp
)
set(AlgaDynamicIEnvGenBuf_sc_files
    plugins/AlgaDynamicIEnvGenBuf/AlgaDynamicIEnvGenBuf.sc
)
set(AlgaDynamicIEnvGenBuf_schelp_files
    plugins/AlgaDynamicIEnvGenBuf/AlgaDynamicIEnvGenBuf.schelp
)

sc_add_server_plugin(
    "AlgaDynamicIEnvGenBuf/AlgaDynamicIEnvGenBuf" # desination directory
    "AlgaDynamicIEnvGenBuf" # target name
    "${AlgaDynamicIEnvGenBuf_cpp_files}"
    "${AlgaDynamicIEnvGenBuf_sc_files}"
    "${AlgaDynamicIEnvGenBuf_schelp_files}"
    ""
)

# End target AlgaDynamicIEnvGenBuf
####################################################################################################

####################################################################################################
# END PLUGIN TARGET DEFINITION
####################################################################################################

message(STATUS "Generating plugin targets done")
