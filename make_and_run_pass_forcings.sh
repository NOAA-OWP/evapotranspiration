#!/bin/bash
gcc ./src/main_pass_forcing.c ./src/pet.c ./src/bmi_pet.c ./extern/forcing_code/src/aorc.c ./extern/forcing_code/src/bmi_aorc.c -lm -o run_bmi_forcings_pass
./run_bmi_forcings_pass ./configs/pet_config_bmi.txt ./configs/aorc_config_cat_67.txt 
