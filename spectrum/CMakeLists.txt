add_executable(spectrum
        main.cpp
)

kailux_copy_resources(spectrum)

target_precompile_headers(spectrum REUSE_FROM kailux)

target_link_libraries(spectrum PRIVATE
        kailux
)
