#!/bin/bash
gcc ./main_unit_test_bmi.c ../src/bmi_pet.c ../src/pet.c -lm -o run_pet_bmi_test
./run_pet_bmi_test ../configs/pet_config_bmi_unit_test.txt
#./run_pet_bmi_test ../configs/pet_config_cat_67.txt
