#!/bin/bash
set -e

EWTS_PREFIX="${EWTS_PREFIX:-/tmp/ewts_install}"

${CC:-gcc} -Og -std=gnu99 -Wall \
  -DBMI_ACTIVE \
  -I../include \
  -I"${EWTS_PREFIX}/include" \
  -c ./main_unit_test_bmi.c \
  -o main_unit_test_bmi.o

${CC:-gcc} -Og -std=gnu99 -Wall \
  -DBMI_ACTIVE \
  -I../include \
  -I"${EWTS_PREFIX}/include" \
  -c ../src/bmi_pet.c \
  -o bmi_pet.o

${CC:-gcc} -Og -std=gnu99 -Wall \
  -DBMI_ACTIVE \
  -I../include \
  -I"${EWTS_PREFIX}/include" \
  -c ../src/pet.c \
  -o pet.o

${CXX:-g++} -Og -std=c++17 -Wall \
  -DBMI_ACTIVE \
  -I../include \
  -I"${EWTS_PREFIX}/include" \
  -c ../src/pet_serialization.cpp \
  -o pet_serialization.o

${CXX:-g++} \
  main_unit_test_bmi.o \
  bmi_pet.o \
  pet.o \
  pet_serialization.o \
  -L"${EWTS_PREFIX}/lib" \
  -Wl,-rpath,"${EWTS_PREFIX}/lib" \
  -lewts_c \
  -lboost_serialization \
  -lm \
  -o run_pet_bmi_test

./run_pet_bmi_test ../configs/pet_config_bmi_unit_test.txt
test_pass=$?

rm -f main_unit_test_bmi.o bmi_pet.o pet.o pet_serialization.o
rm -f run_pet_bmi_test
rm -rf run_pet_bmi_test.dSYM

if [ "$test_pass" -eq 0 ]; then
  echo "PET BMI unit test passed"
else
  echo "PET BMI unit test failed with exit code ${test_pass}"
fi

exit $test_pass
