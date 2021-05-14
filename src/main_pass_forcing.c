#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/bmi.h"
#include "../include/et.h"
#include "../include/bmi_et.h"

#include "../../aorc-bmi/include/aorc.h"
#include "../../aorc-bmi/include/bmi_aorc.h"

/***************************************************************
    Function to pass the forcing data from AORC to ET using BMI.
    This requires a lot of getters and setters, 
    so no need to clutter up main program
***************************************************************/
void pass_forcing_from_aorc_to_et(Bmi *et_bmi_model, Bmi *aorc_bmi_model){

    /********************************************************************
        TODO: Get variable names through BMI, then loop through those
              so we don't re-write the get/set functions over and over
    ********************************************************************/

    double *var = NULL;
    var = (double*) malloc (sizeof (double)*1);

//    printf("getting AORC from BMI and setting in ET\n");
    const char* var_name1 = "land_surface_air__temperature";
    aorc_bmi_model->get_value(aorc_bmi_model, var_name1, &(var[0]));
    et_bmi_model->set_value(et_bmi_model, var_name1, &(var[0]));

    const char* var_name2 = "land_surface_air__pressure";
    aorc_bmi_model->get_value(aorc_bmi_model, var_name2, &(var[0]));
    et_bmi_model->set_value(et_bmi_model, var_name2, &(var[0]));

    const char* var_name3 = "atmosphere_air_water~vapor__relative_saturation";
    aorc_bmi_model->get_value(aorc_bmi_model, var_name3, &(var[0]));
    et_bmi_model->set_value(et_bmi_model, var_name3, &(var[0]));

    const char* var_name4 = "atmosphere_water__liquid_equivalent_precipitation_rate";
    aorc_bmi_model->get_value(aorc_bmi_model, var_name4, &(var[0]));
    et_bmi_model->set_value(et_bmi_model, var_name4, &(var[0]));

    const char* var_name5 = "land_surface_radiation~incoming~shortwave__energy_flux";
    aorc_bmi_model->get_value(aorc_bmi_model, var_name5, &(var[0]));
    et_bmi_model->set_value(et_bmi_model, var_name5, &(var[0]));

    const char* var_name6 = "land_surface_radiation~incoming~longwave__energy_flux";
    aorc_bmi_model->get_value(aorc_bmi_model, var_name6, &(var[0]));
    et_bmi_model->set_value(et_bmi_model, var_name6, &(var[0]));

    const char* var_name7 = "land_surface_wind__x_component_of_velocity";
    aorc_bmi_model->get_value(aorc_bmi_model, var_name7, &(var[0]));
    et_bmi_model->set_value(et_bmi_model, var_name7, &(var[0]));

    const char* var_name8 = "land_surface_wind__y_component_of_velocity";
    aorc_bmi_model->get_value(aorc_bmi_model, var_name8, &(var[0]));
    et_bmi_model->set_value(et_bmi_model, var_name8, &(var[0]));
}

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
    printf("make sure to include a path to the ET config file\n");
    exit(1);
  }
  if(argc<=2){
    printf("make sure to include a path to the AORC config file\n");
    exit(1);
  }
  
  
  /************************************************************************
      allocating memory to store the entire BMI structure for ET
  ************************************************************************/
//  printf("allocating memory to store entire BMI ET structure\n");
  Bmi *et_bmi_model = (Bmi *) malloc(sizeof(Bmi));
//  printf("allocating memory to store entire BMI AORC structure\n");
  Bmi *aorc_bmi_model = (Bmi *) malloc(sizeof(Bmi));

  /************************************************************************
      Registering the BMI model for ET
  ************************************************************************/
//  printf("Registering BMI ET model\n");
  register_bmi_et(et_bmi_model);
//  printf("Registering BMI AORC model\n");
  register_bmi_aorc(aorc_bmi_model);

  /************************************************************************
      Initializing the BMI model for ET
  ************************************************************************/
//  printf("Initializeing BMI ET model\n");
  const char *cfg_file_et = argv[1];
  et_bmi_model->initialize(et_bmi_model, cfg_file_et);
//  printf("Initializeing BMI AORC model\n");
  const char *cfg_file_aorc = argv[2];
  aorc_bmi_model->initialize(aorc_bmi_model, cfg_file_aorc);

  /************************************************************************
    Get the information from the configuration here in Main
  ************************************************************************/
  et_model *et;
  et = (et_model *) et_bmi_model->data;

  /************************************************************************
    Update the AORC forcing data
  ************************************************************************/
  aorc_bmi_model->update(aorc_bmi_model);
  
  /************************************************************************
    Getting forcing from AORC and setting forcing for ET
  ************************************************************************/
  pass_forcing_from_aorc_to_et(et_bmi_model, aorc_bmi_model);

//  printf("Updating BMI ET model\n");
  et_bmi_model->update(et_bmi_model);

//  printf("looping through and calling update\n");
  int i=0;
  for (i = 0; i < 1; i++){
    // The unit test only runs one time step.
    aorc_bmi_model->update(aorc_bmi_model);
    pass_forcing_from_aorc_to_et(et_bmi_model, aorc_bmi_model);
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

  printf("Finalizing models\n");
  et_bmi_model->finalize(et_bmi_model);
  aorc_bmi_model->finalize(aorc_bmi_model);

  return 0;
}  /************    END MAIN PROGRAM    ***********************************************/

