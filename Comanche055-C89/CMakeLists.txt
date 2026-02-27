cmake_minimum_required(VERSION 3.15)

project(comanche055 LANGUAGES C)

add_executable(comanche055
    main.c
    agc_math.c
    agc_cpu.c
    executive.c
    waitlist.c
    dsky.c
    dsky_gui.c
    dsky_web.c
    pinball.c
    alarm.c
    timer.c
    programs.c
    navigation.c
    service.c
)

if(WIN32)
    target_link_libraries(comanche055 PRIVATE user32 gdi32 ws2_32)
endif()

set_property(TARGET comanche055 PROPERTY C_STANDARD 90)
set_property(TARGET comanche055 PROPERTY C_STANDARD_REQUIRED ON)
set_property(TARGET comanche055 PROPERTY C_EXTENSIONS OFF)

if(MSVC)
    set_property(TARGET comanche055 PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(comanche055 PRIVATE /W4 /WX)
else()
    target_compile_options(comanche055 PRIVATE -pedantic-errors -Wall -Wextra -Werror -Wno-long-long)
endif()
