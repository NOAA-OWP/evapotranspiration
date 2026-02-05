cmake_minimum_required(VERSION 3.12)

# Uncomment this and rebuild artifacts to enable debugging
set(CMAKE_BUILD_TYPE Debug)

project(petbmi VERSION 1.0.0 DESCRIPTION "OWP PET BMI Module Shared Library")

# PET
set(PET_LIB_NAME_CMAKE petbmi)
set(PET_LIB_DESC_CMAKE "OWP PET BMI Module Shared Library")

# Make sure these are compiled with this directive
add_compile_definitions(BMI_ACTIVE)

if(WIN32)
    add_library(petbmi src/bmi_pet.c src/pet.c)
else()
    add_library(petbmi SHARED src/bmi_pet.c src/pet.c)
endif()

target_include_directories(petbmi PRIVATE include)

set_target_properties(petbmi PROPERTIES VERSION ${PROJECT_VERSION})

set_target_properties(petbmi PROPERTIES PUBLIC_HEADER include/bmi_pet.h)

# Code requires minimum of C99 standard to compile
set_target_properties(petbmi PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

install(TARGETS petbmi
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(petbmi.pc.in petbmi.pc @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/petbmi.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
