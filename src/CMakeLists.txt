if(NOT DEFINED ENV{YOUR_LLVM_PATH})

    add_llvm_library( LLVMAddressInterception
        # List your source files here.
        AddressIntercept.cpp
        AllocaRecognize.cpp
        MemoryOperationRecognize.cpp
        InlineInstrumentation.cpp
        Logger.cpp

      ADDITIONAL_HEADER_DIRS
      ${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms

      DEPENDS
      intrinsics_gen
      )

else ()

    add_library(AddressInterceptorPassModule MODULE
        # List your source files here.
        AddressIntercept.cpp
        AllocaRecognize
        MemoryOperationRecognize
        InlineInstrumentation
        Logger
    )

    add_definitions(-DADIN_LOAD_MODULE)

    # Use C++11 to compile your pass (i.e., supply -std=c++11).
    target_compile_features(AddressInterceptorPassModule PRIVATE cxx_range_for cxx_auto_type)

    # LLVM is (typically) built with no C++ RTTI. We need to match that;
    # otherwise, we'll get linker errors about missing RTTI data.
    set_target_properties(AddressInterceptorPassModule PROPERTIES
        COMPILE_FLAGS "-fno-rtti"
    )

    # Get proper shared-library behavior (where symbols are not necessarily
    # resolved when the shared library is linked) on OS X.
    if(APPLE)
        set_target_properties(AddressInterceptorPassModule PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup"
        )
    endif(APPLE)

endif()
