#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../include/bmi.h"
#include "../include/pet.h"
#include "../include/bmi_pet.h"
#include "../include/logger.h"

/************************************************************************
    This main program is a mock framework.
    This is not part of BMI, but acts as the driver that calls the model.
************************************************************************/
int main(int argc, const char *argv[])
{

    #ifdef EWTS_HAVE_NGEN_BRIDGE
        EwtsInit(EWTS_ID_PET, true);
    #else
        EwtsInit(EWTS_ID_PET, false);
    #endif

    if (argc <= 1) {
        LOG(FATAL, "Missing PET config file argument");
        exit(1);
    }

    Bmi *pet_bmi_model = (Bmi *) malloc(sizeof(Bmi));
    if (pet_bmi_model == NULL) {
        LOG(FATAL, "Failed to allocate BMI structure for PET");
        exit(1);
    }

    register_bmi_pet(pet_bmi_model);

    const char *cfg_file_pet = argv[1];
    LOG(INFO, "Initializing PET with config '%s'", cfg_file_pet);

    if (pet_bmi_model->initialize(pet_bmi_model, cfg_file_pet) != BMI_SUCCESS) {
        LOG(FATAL, "Failed to initialize PET model");
        free(pet_bmi_model);
        exit(1);
    }

    pet_model *pet = (pet_model *) pet_bmi_model->data;
    if (pet == NULL) {
        LOG(FATAL, "PET model data is NULL after initialization");
        free(pet_bmi_model);
        exit(1);
    }

    if (pet_bmi_model->update(pet_bmi_model) != BMI_SUCCESS) {
        LOG(FATAL, "Initial PET update failed");
        pet_bmi_model->finalize(pet_bmi_model);
        free(pet_bmi_model);
        exit(1);
    }

    for (int i = 0; i < 1; i++) {
        if (pet->bmi.run_unit_tests == 0) {
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
    }

    LOG(INFO, "Finalizing PET model");

    if (pet_bmi_model->finalize(pet_bmi_model) != BMI_SUCCESS) {
        LOG(SEVERE, "PET finalize failed");
    }

    free(pet_bmi_model);
    return 0;
}
