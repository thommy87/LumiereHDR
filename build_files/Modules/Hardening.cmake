# Exploit mitigations for every binary we build.
#
# LHDR_HARDENING   ASLR, DEP, CFG, stack protector, FORTIFY, RELRO (default ON)
# LHDR_SANITIZE    sanitizer list for test builds, e.g. "address;undefined"

option(LHDR_HARDENING "Build with exploit mitigations" ON)
set(LHDR_SANITIZE "" CACHE STRING "Sanitizers to enable, e.g. address;undefined")

macro(lhdr_append_link_flags FLAGS)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${FLAGS}")
endmacro()

if(LHDR_HARDENING)
    message(STATUS "Hardening: enabled")
    if(MSVC)
        add_compile_options(/GS /guard:cf)
        lhdr_append_link_flags("/guard:cf /DYNAMICBASE /HIGHENTROPYVA /NXCOMPAT /CETCOMPAT")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fstack-protector-strong)
        if(WIN32)
            # MinGW: opt in to ASLR (with 64-bit entropy) and DEP; older
            # binutils left these off, so 2.6.0 loaded at a fixed address
            lhdr_append_link_flags("-Wl,--dynamicbase -Wl,--high-entropy-va -Wl,--nxcompat")
            if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
                # Control Flow Guard is only available with LLVM MinGW
                add_compile_options(-mguard=cf)
                lhdr_append_link_flags("-mguard=cf")
            endif()
        else()
            set(CMAKE_POSITION_INDEPENDENT_CODE ON)
            add_compile_options(
                $<$<NOT:$<CONFIG:Debug>>:-U_FORTIFY_SOURCE>
                $<$<NOT:$<CONFIG:Debug>>:-D_FORTIFY_SOURCE=2>)
            if(NOT APPLE)
                add_compile_options(-fstack-clash-protection)
                lhdr_append_link_flags("-pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack")
            endif()
        endif()
    endif()
endif()

if(LHDR_SANITIZE)
    string(REPLACE ";" "," LHDR_SANITIZE_LIST "${LHDR_SANITIZE}")
    message(STATUS "Sanitizers: ${LHDR_SANITIZE_LIST}")
    add_compile_options(-fsanitize=${LHDR_SANITIZE_LIST} -fno-omit-frame-pointer)
    lhdr_append_link_flags("-fsanitize=${LHDR_SANITIZE_LIST}")
endif()
