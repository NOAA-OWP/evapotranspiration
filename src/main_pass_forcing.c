#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../include/bmi.h"
#include "../include/pet.h"
#include "../include/bmi_pet.h"
#include "../include/logger.h"

#include "../extern/forcing_code/include/aorc.h"
#include "../extern/forcing_code/include/bmi_aorc.h"

/***************************************************************
    Function to pass the forcing data from AORC to PET using BMI.
    This requires a lot of getters and setters,
    so no need to clutter up main program
***************************************************************/
void pass_forcing_from_aorc_to_pet(Bmi *pet_bmi_model, Bmi *aorc_bmi_model)
{
    if (pet_bmi_model == NULL || aorc_bmi_model == NULL) {
        LOG(SEVERE, "pass_forcing_from_aorc_to_pet called with NULL BMI model");
        return;
    }

    double *var = (double *) malloc(sizeof(double) * 1);
    if (var == NULL) {
        LOG(FATAL, "Failed to allocate temporary forcing buffer");
        return;
    }

    const char* var_name1 = "land_surface_air__temperature";
    if (aorc_bmi_model->get_value(aorc_bmi_model, var_name1, &(var[0])) != BMI_SUCCESS) {
        LOG(SEVERE, "Failed to get AORC variable '%s'", var_name1);
    }
    else if (pet_bmi_model->set_value(pet_bmi_model, var_name1, &(var[0])) != BMI_SUCCESS) {
        LOG(SEVERE, "Failed to set PET variable '%s'", var_name1);
    }

    const char* var_name2 = "land_surface_air__pressure";
    if (aorc_bmi_model->get_value(aorc_bmi_model, var_name2, &(var[0])) != BMI_SUCCESS) {
        LOG(SEVERE, "Failed to get AORC variable '%s'", var_name2);
    }
    else if (pet_bmi_model->set_value(pet_bmi_model, var_name2, &(var[0])) != BMI_SUCCESS) {
        LOG(SEVERE, "Failed to set PET variable '%s'", var_name2);
    }

    const char* var_name3 = "atmosphere_air_water~vapor__relative_saturation";
    if (aorc_bmi_model->get_value(aorc_bmi_model, var_name3, &(var[0])) != BMI_SUCCESS) {
        LOG(SEVERE, "Failed to get AORC variable '%s'", var_name3);
    }
    else if (pet_bmi_model->set_value(pet_bmi_model, var_name3, &(var[0])) != BMI_SUCCESS) {
        LOG(SEVERE, "Failed to set PET variable '%s'", var_name3);
    }

    const char* var_name5 = "land_surface_radiation~incoming~shortwave__energy_flux";
    if (aorc_bmi_model->get_value(aorc_bmi_model, var_name5, &(var[0])) != BMI_SUCCESS) {
        LOG(SEVERE, "Failed to get AORC variable '%s'", var_name5);
    }
    else if (pet_bmi_model->set_value(pet_bmi_model, var_name5, &(var[0])) != BMI_SUCCESS) {
        LOG(SEVERE, "Failed to set PET variable '%s'", var_name5);
    }

    const char* var_name6 = "land_surface_radiation~incoming~longwave__energy_flux";
    if (aorc_bmi_model->get_value(aorc_bmi_model, var_name6, &(var[0])) != BMI_SUCCESS) {
        LOG(SEVERE, "Failed to get AORC variable '%s'", var_name6);
    }
    else if (pet_bmi_model->set_value(pet_bmi_model, var_name6, &(var[0])) != BMI_SUCCESS) {
        LOG(SEVERE, "Failed to set PET variable '%s'", var_name6);
    }

    const char* var_name7 = "land_surface_wind__x_component_of_velocity";
    if (aorc_bmi_model->get_value(aorc_bmi_model, var_name7, &(var[0])) != BMI_SUCCESS) {
        LOG(SEVERE, "Failed to get AORC variable '%s'", var_name7);
    }
    else if (pet_bmi_model->set_value(pet_bmi_model, var_name7, &(var[0])) != BMI_SUCCESS) {
        LOG(SEVERE, "Failed to set PET variable '%s'", var_name7);
    }

    const char* var_name8 = "land_surface_wind__y_component_of_velocity";
    if (aorc_bmi_model->get_value(aorc_bmi_model, var_name8, &(var[0])) != BMI_SUCCESS) {
        LOG(SEVERE, "Failed to get AORC variable '%s'", var_name8);
    }
    else if (pet_bmi_model->set_value(pet_bmi_model, var_name8, &(var[0])) != BMI_SUCCESS) {
        LOG(SEVERE, "Failed to set PET variable '%s'", var_name8);
    }

    free(var);
}

/************************************************************************
    This main program is a mock framework.
    This is not part of BMI, but acts as the driver that calls the model.
************************************************************************/
int main(int argc, const char *argv[])
{

#ifdef PET_USE_EWTS
    // Initialize the Error and Warning Trapping System
    #pragma message("evapotranspiration.main_pass_forcing.main: PET_USE_EWTS ON")
    EwtsInit(PET_MODULE_ID, true);
#else
    #pragma message("evapotranspiration.main_pass_forcing.main: PET_USE_EWTS OFF")
#endif

    if (argc <= 1) {
        LOG(FATAL, "Missing PET config file argument");
        exit(1);
    }
    if (argc <= 2) {
        LOG(FATAL, "Missing AORC config file argument");
        exit(1);
    }

    Bmi *pet_bmi_model = (Bmi *) malloc(sizeof(Bmi));
    Bmi *aorc_bmi_model = (Bmi *) malloc(sizeof(Bmi));

    if (pet_bmi_model == NULL || aorc_bmi_model == NULL) {
        LOG(FATAL, "Failed to allocate BMI model structures");
        exit(1);
    }

    register_bmi_pet(pet_bmi_model);
    register_bmi_aorc(aorc_bmi_model);

    const char *cfg_file_pet = argv[1];
    const char *cfg_file_aorc = argv[2];

    LOG(INFO, "Initializing PET with config '%s'", cfg_file_pet);
    if (pet_bmi_model->initialize(pet_bmi_model, cfg_file_pet) != BMI_SUCCESS) {
        LOG(FATAL, "Failed to initialize PET model");
        exit(1);
    }

    LOG(INFO, "Initializing AORC with config '%s'", cfg_file_aorc);
    if (aorc_bmi_model->initialize(aorc_bmi_model, cfg_file_aorc) != BMI_SUCCESS) {
        LOG(FATAL, "Failed to initialize AORC model");
        exit(1);
    }

    pet_model *pet = (pet_model *) pet_bmi_model->data;
    if (pet == NULL) {
        LOG(FATAL, "PET model data is NULL after initialization");
        exit(1);
    }

    if (aorc_bmi_model->update(aorc_bmi_model) != BMI_SUCCESS) {
        LOG(FATAL, "Initial AORC update failed");
        exit(1);
    }

    pass_forcing_from_aorc_to_pet(pet_bmi_model, aorc_bmi_model);

    if (pet_bmi_model->update(pet_bmi_model) != BMI_SUCCESS) {
        LOG(FATAL, "Initial PET update failed");
        exit(1);
    }

    for (int i = 0; i < 100; i++) {
        if (aorc_bmi_model->update(aorc_bmi_model) != BMI_SUCCESS) {
            LOG(SEVERE, "AORC update failed at loop iteration %d", i);
            break;
        }

        pass_forcing_from_aorc_to_pet(pet_bmi_model, aorc_bmi_model);

        if (pet_bmi_model->update(pet_bmi_model) != BMI_SUCCESS) {
            LOG(SEVERE, "PET update failed at loop iteration %d", i);
            break;
        }

        LOG(INFO, "LWDOWN after set value %lf", pet->aorc.incoming_longwave_W_per_m2);
        LOG(INFO, "SWDOWN before set value %lf", pet->aorc.incoming_shortwave_W_per_m2);
        LOG(INFO, "surface_pressure_Pa %lf", pet->aorc.surface_pressure_Pa);
        LOG(INFO, "specific_humidity_2m_kg_per_kg %lf", pet->aorc.specific_humidity_2m_kg_per_kg);
        LOG(INFO, "air_temperature_2m_K %lf", pet->aorc.air_temperature_2m_K);
        LOG(INFO, "u_wind_speed_10m_m_per_s %lf", pet->aorc.u_wind_speed_10m_m_per_s);
        LOG(INFO, "v_wind_speed_10m_m_per_s %lf", pet->aorc.v_wind_speed_10m_m_per_s);
        LOG(INFO, "potential evapotranspiration (m s-1): %8.4e", pet->pet_m_per_s);
        LOG(INFO, "--------------------- END OF TIMESTEP ---------------------------------");
    }

    LOG(INFO, "Finalizing models");

    if (pet_bmi_model->finalize(pet_bmi_model) != BMI_SUCCESS) {
        LOG(SEVERE, "PET finalize failed");
    }
    if (aorc_bmi_model->finalize(aorc_bmi_model) != BMI_SUCCESS) {
        LOG(SEVERE, "AORC finalize failed");
    }

    free(pet_bmi_model);
    free(aorc_bmi_model);

    return 0;
}
