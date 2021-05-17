#!/bin/bash
touch run_bmi_forcings_pass
mv -f run_bmi_forcings_pass z_trash
gcc -lm ./src/main_pass_forcing.c ./src/et.c ./src/bmi_et.c ../AORC/src/aorc.c ../AORC/src/bmi_aorc.c -o run_bmi_forcings_pass
./run_bmi_forcings_pass ./configs/et_config_bmi.txt ../AORC/configs/aorc_config_cat_67.txt 
