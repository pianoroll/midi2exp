cmake_minimum_required(VERSION 2.8)

project(midi2exp CXX)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CheckIncludeFiles)

include_directories(include include/midifile)


##############################
##
## Operating-system specific settings:
##

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
endif()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

    option(STATIC_CRT "Use static CRT libraries" OFF)

    # Rewrite command line flags to use /MT if necessary
    if(STATIC_CRT)
        foreach(flag_var
                CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
            if(${flag_var} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            endif(${flag_var} MATCHES "/MD")
        endforeach(flag_var)
    endif()
endif()


##############################
##
## Library:
##

set(SRCS
    src/midifile/Options.cpp
    src/midifile/Binasc.cpp
    src/midifile/MidiEvent.cpp
    src/midifile/MidiEventList.cpp
    src/midifile/MidiFile.cpp
    src/midifile/MidiMessage.cpp
    src/Expressionizer.cpp
)

set(HDRS
    include/midifile/Binasc.h
    include/midifile/MidiEvent.h
    include/midifile/MidiEventList.h
    include/midifile/MidiFile.h
    include/midifile/MidiMessage.h
    include/midifile/Options.h
    include/Expressionizer.h
)

add_library(expression STATIC ${SRCS} ${HDRS})

##############################
##
## Programs:
##

add_executable(midi2exp tools/midi2exp.cpp)

target_link_libraries(midi2exp expression)



