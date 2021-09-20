#!/bin/bash
gcc -lm ./src/main_unit_test_bmi.c ./src/bmi_pet.c ./src/pet.c -o run_pet_bmi_test
#./run_pet_bmi_test ./configs/pet_config_unit_test1.txt
#./run_pet_bmi_test ./configs/pet_config_unit_test2.txt
#./run_pet_bmi_test ./configs/pet_config_unit_test3.txt
#./run_pet_bmi_test ./configs/pet_config_unit_test4.txt
#./run_pet_bmi_test ./configs/pet_config_unit_test5.txt
./run_pet_bmi_test ./configs/pet_config_cat_67.txt