cmake_minimum_required(VERSION 3.7)
project(sandbox
	VERSION 0.0.1
	LANGUAGES C CXX)

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
message(STATUS "${SOURCES}")

add_executable(sandbox ${SOURCES})

if(WIN32)
	include_directories("C:/msys64/ucrt64/include/SDL2" "C:/msys64/ucrt64/include/GL" ".")
else ()
	include_directories("/usr/include/SDL2" "/opt/local/include" "/usr/include/GL" ".")
endif ()

target_include_directories(sandbox PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}.")

if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
	set(WASM_OPTS
		-Wall
		-Wformat
		-O2
		# -g
		-std=c++17
		"SHELL:-s DISABLE_EXCEPTION_CATCHING=1"
		"SHELL:-s FULL_ES2"
		"SHELL:-s USE_SDL=2"
		"SHELL:-s USE_FREETYPE=1"
		"SHELL:-s USE_SDL_MIXER=2"
		# "SHELL: -fdebug-compilation-dir='.'"
		# "SHELL: -gseparate-dwarf='sandbox.dbg.wasm'"
		)

	target_compile_options(sandbox PRIVATE ${WASM_OPTS}
		-Wno-unused-command-line-argument
		-DUSE_OPENGL_ES)
	target_link_options(sandbox PRIVATE ${WASM_OPTS}
		--use-preload-plugins
		--no-heap-copy
		"SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/assets@assets"
		"SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/ext/emscripten/shell_minimal.html"
		"SHELL:-s WASM=1"
		"SHELL:-s ALLOW_MEMORY_GROWTH=1"
		"SHELL:-s NO_EXIT_RUNTIME=0"
		"SHELL:-s ASSERTIONS=1")

	add_subdirectory(ext/datachannel-wasm EXCLUDE_FROM_ALL)

	set_target_properties(sandbox PROPERTIES OUTPUT_NAME "sandbox")
	set_target_properties(sandbox PROPERTIES SUFFIX ".html")
	# target_link_libraries(sandbox)

else()
	find_package(SDL2 REQUIRED)

	set(OPTS -std=c++17 -g -O2 -Wall -Wformat -D_REENTRANT  ${SDL2_CFLAGS})

	target_compile_options(sandbox PRIVATE ${OPTS})
	target_link_options(sandbox PRIVATE ${OPTS})


	if(WIN32)
		target_link_libraries(sandbox opengl32 glew32 ${SDL2_LIBRARIES})
	else ()
		target_link_libraries(sandbox GL GLEW ${SDL2_LIBRARIES})
	endif ()
endif()