cmake_minimum_required(VERSION 3.3)
project(PC2)



set(CMAKE_C_STANDARD 99)
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
LINK_LIBRARIES(${MPI_CXX_LIBRARIES})

SET(CMAKE_C_COMPILER mpicc)
SET(CMAKE_CXX_COMPILER mpicxx)
SET(SOURCE_FILES main.c main.c)
SET(CMAKE_CXX_FLAGS -Wall -Wextra -Wconversion)

add_executable(PC2 ${SOURCE_FILES})

# Include MPI includes
include_directories(${MPI_INCLUDE_PATH})
if(MPI_COMPILE_FLAGS)
    set_target_properties(PC2 PROPERTIES
            COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()
if(MPI_LINK_FLAGS)
    set_target_properties(PC2 PROPERTIES
            LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()





