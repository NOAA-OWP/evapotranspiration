#!/bin/bash
gcc -lm ./src/main_pass_forcing.c ./src/pet.c ./src/bmi_pet.c ./forcing_code/src/aorc.c ./forcing_code/src/bmi_aorc.c -o run_bmi_forcings_pass
./run_bmi_forcings_pass ./configs/pet_config_bmi.txt ./configs/aorc_config_cat_67.txt 
