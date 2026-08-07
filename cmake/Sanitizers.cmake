# Sanitizers.cmake — runtime sanitizer instrumentation.
#
# Defines `modulo_enable_sanitizers(<target>)`, which honors the
# MODULO_SANITIZERS cache variable: a comma-separated -fsanitize= value such
# as "address,undefined" (as set by the `dev-asan` preset). When the variable
# is empty (the default) this function is a no-op, so plain builds carry no
# instrumentation cost.

include_guard(GLOBAL)

# Instrument a first-party target with the sanitizers named in MODULO_SANITIZERS.
function(modulo_enable_sanitizers target)
    if(NOT MODULO_SANITIZERS)
        return()
    endif()

    # -fno-omit-frame-pointer keeps sanitizer stack traces readable.
    target_compile_options(${target} PRIVATE -fsanitize=${MODULO_SANITIZERS} -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=${MODULO_SANITIZERS})
endfunction()
