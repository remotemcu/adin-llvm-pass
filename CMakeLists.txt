cmake_minimum_required(VERSION 3.4)

message("################################## YOUR_LLVM_PATH PATH : $ENV{YOUR_LLVM_PATH} ##################################")

# we need YOUR_LLVM_PATH in order not automatically set LLVM_DIR
if(NOT DEFINED ENV{YOUR_LLVM_PATH})
    message(AUTHOR_WARNING "ENV{YOUR_LLVM_PATH} is not defined")
    message(AUTHOR_WARNING "Pass is built in to llvm project")
else ()
    set(ENV{LLVM_DIR} $ENV{YOUR_LLVM_PATH}/lib/cmake/llvm)
    find_package(LLVM REQUIRED CONFIG)
    add_definitions(${LLVM_DEFINITIONS})
    include_directories(${LLVM_INCLUDE_DIRS})
    link_directories(${LLVM_LIBRARY_DIRS})
endif()

add_subdirectory(src)  # Use your pass name here.
