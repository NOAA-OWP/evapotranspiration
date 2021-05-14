#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/bmi.h"
#include "../include/et.h"
#include "../include/bmi_et.h"

/************************************************************************
    This main program is a mock framwork.
    This is not part of BMI, but acts as the driver that calls the model.
************************************************************************/
int
 main(int argc, const char *argv[])
{

  /************************************************************************
      A configuration file is required for running this model through BMI
  ************************************************************************/
  if(argc<=1){
//    printf("make sure to include a path to the ET config file\n");
    exit(1);
  }

  /************************************************************************
      allocating memory to store the entire BMI structure for ET
  ************************************************************************/
//  printf("allocating memory to store entire BMI ET structure\n");
  Bmi *et_bmi_model = (Bmi *) malloc(sizeof(Bmi));

  /************************************************************************
      Registering the BMI model for ET
  ************************************************************************/
//  printf("Registering BMI ET model\n");
  register_bmi_et(et_bmi_model);

  /************************************************************************
      Initializing the BMI model for ET
  ************************************************************************/
//  printf("Initializeing BMI ET model\n");
  const char *cfg_file_et = argv[1];
  et_bmi_model->initialize(et_bmi_model, cfg_file_et);

  /************************************************************************
      Defining the ET data as a new variable, so we can use its contents
  ************************************************************************/
//  printf("Get the information from the configuration here in Main\n");
  et_model *et;
  et = (et_model *) et_bmi_model->data;

  /************************************************************************
      Defining the ET data as a new variable, so we can use its contents
  ************************************************************************/
//  printf("Updating ET model\n");
  et_bmi_model->update(et_bmi_model);

//  printf("looping through and calling update\n");
  int i=0;
  for (i = 0; i < 1; i++){
    // The unit test only runs one time step.
    if (et->bmi.run_unit_tests == 0){
      et_bmi_model->update(et_bmi_model);
      printf("LWDOWN after set value %lf\n", et->aorc.incoming_longwave_W_per_m2);
      printf("SWDOWN before set value %lf\n", et->aorc.incoming_shortwave_W_per_m2);
      printf("precip_kg_per_m2 %lf \n", et->aorc.precip_kg_per_m2);
      printf("surface_pressure_Pa %lf \n", et->aorc.surface_pressure_Pa);
      printf("specific_humidity_2m_kg_per_kg %lf \n", et->aorc.specific_humidity_2m_kg_per_kg);
      printf("air_temperature_2m_K %lf \n", et->aorc.air_temperature_2m_K);
      printf("u_wind_speed_10m_m_per_s %lf \n", et->aorc.u_wind_speed_10m_m_per_s);
      printf("v_wind_speed_10m_m_per_s %lf \n", et->aorc.v_wind_speed_10m_m_per_s);
      printf("potential evapotranspiration (m s-1): %8.4e\n", et->et_m_per_s);
      printf("--------------------- END OF TIMESTEP ---------------------------------\n");
    }
  }

//  printf("Finalizing models\n");
  et_bmi_model->finalize(et_bmi_model);

  return 0;
}  /************    END MAIN PROGRAM    ***********************************************/