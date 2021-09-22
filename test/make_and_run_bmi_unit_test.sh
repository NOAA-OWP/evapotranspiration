#!/bin/bash
gcc -lm ./main_unit_test_bmi.c ../src/bmi_pet.c ../src/pet.c -o run_pet_bmi_test
./run_pet_bmi_test ./configs/pet_config_bmi_method1.txt
./run_pet_bmi_test ./configs/pet_config_bmi_method2.txt
./run_pet_bmi_test ./configs/pet_config_bmi_method3.txt
./run_pet_bmi_test ./configs/pet_config_bmi_method4.txt
./run_pet_bmi_test ./configs/pet_config_bmi_method5.txt
./run_pet_bmi_test ./configs/pet_config_cat_67_method1.txt
./run_pet_bmi_test ./configs/pet_config_cat_67_method2.txt
./run_pet_bmi_test ./configs/pet_config_cat_67_method3.txt
./run_pet_bmi_test ./configs/pet_config_cat_67_method4.txt
./run_pet_bmi_test ./configs/pet_config_cat_67_method5.txt