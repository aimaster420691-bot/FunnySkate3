cmake_minimum_required(VERSION 3.25)
project(Skate3FunTrainer LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "This package targets the Windows v2.0.2 release.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(Skate3FunTrainer SHARED src/trainer.cpp)
target_link_libraries(Skate3FunTrainer PRIVATE user32)
target_compile_definitions(Skate3FunTrainer PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
set_target_properties(Skate3FunTrainer PROPERTIES OUTPUT_NAME "Skate3FunTrainer")

add_executable(Skate3TrainerLauncher src/launcher.cpp)
target_link_libraries(Skate3TrainerLauncher PRIVATE user32)
target_compile_definitions(Skate3TrainerLauncher PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
