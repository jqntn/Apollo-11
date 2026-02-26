cmake_minimum_required(VERSION 3.10)
project(comanche055 C)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall)
endif()

add_executable(comanche055
    main.c
    agc_math.c
    agc_cpu.c
    executive.c
    waitlist.c
    dsky.c
    pinball.c
    alarm.c
    timer.c
    programs.c
    navigation.c
    service.c
)
