#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/bmi.h"
#include "../include/et.h"
#include "../include/bmi_et.h"

#include "../../AORC/include/aorc.h"
#include "../../AORC/include/bmi_aorc.h"

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

  /************************************************************************
      allocating memory to store the entire BMI structure for ET
  ************************************************************************/
  printf("allocating memory to store entire BMI ET structure\n");
  Bmi *et_bmi_model = (Bmi *) malloc(sizeof(Bmi));

  /************************************************************************
      Registering the BMI model for ET
  ************************************************************************/
  printf("Registering BMI ET model\n");
  register_bmi_et(et_bmi_model);

  /************************************************************************
      Initializing the BMI model for ET
  ************************************************************************/
  printf("Initializeing BMI ET model\n");
  const char *cfg_file_et = argv[1];
  et_bmi_model->initialize(et_bmi_model, cfg_file_et);

  /************************************************************************
      Defining the ET data as a new variable, so we can use its contents
  ************************************************************************/
  printf("Get the information from the configuration here in Main\n");
  et_model *et;
  et = (et_model *) et_bmi_model->data;
  if (et->bmi.is_forcing_from_bmi == 0)
      printf("reading forcing file for the ET module %s\n", et->forcing_file);
  else{
      #define DO_BMI_AORC_FORCING
      #ifdef DO_BMI_AORC_FORCING
          // A configuration file is required for running this model through BMI
          if(argc<=2){
            printf("make sure to include a path to the AORC config file\n");
            exit(1);
          }
          printf("Using BMI to pass in forcing information\n");
          printf("allocating memory to store entire BMI AORC structure\n");
          Bmi *aorc_bmi_model = (Bmi *) malloc(sizeof(Bmi));
          printf("Registering BMI AORC model\n");
          register_bmi_et(aorc_bmi_model);
          printf("Initializeing BMI AORC model\n");
          const char *cfg_file_aorc = argv[2];
          aorc_bmi_model->initialize(aorc_bmi_model, cfg_file_aorc);
      #endif
  }

  printf("Updating models\n");

  #ifdef DO_BMI_AORC_FORCING
      aorc_bmi_model->update(aorc_bmi_model);
      pass_forcing_from_aorc_to_et(et_bmi_model, aorc_bmi_model);
  #endif

  et_bmi_model->update(et_bmi_model);

  printf("looping through and calling update\n");
  int i=0;
  for (i = 0; i < 2; i++){
    // The unit test only runs one time step.
    if (et->bmi.run_unit_tests == 0){
      et_bmi_model->update(et_bmi_model);
      printf("potential evapotranspiration (m s-1): %8.4e\n", et->et_m_per_s);
      printf("--------------------- END OF TIMESTEP ---------------------------------\n");
    }
  }

  printf("Finalizing models\n");
  et_bmi_model->finalize(et_bmi_model);

  return 0;
}  /************    END MAIN PROGRAM    ***********************************************/

/***************************************************************
    Function to pass the forcing data from AORC to ET using BMI.
    This requires a lot of getters and setters, 
    so no need to clutter up main program
***************************************************************/
#ifdef DO_BMI_AORC_FORCING
    void pass_forcing_from_aorc_to_et(bmi *et_bmi_model, bmi *aorc_bmi_model){
        printf("getting AORC from BMI and setting in ET\n");
        double *var = NULL;
        var = (double*) malloc (sizeof (double)*1);
        const char *var_name = "land_surface_air__temperature";
        aorc_bmi_model->get_value(aorc_bmi_model, var_name, &(var[0]));
        et_bmi_model->set_value(et_bmi_model, var_name, &(var[0]));
    
    }
#else
    int pass_forcing_from_aorc_to_et(bmi *et_bmi_model, bmi *aorc_bmi_model){
        return 0;
    }
#endif
