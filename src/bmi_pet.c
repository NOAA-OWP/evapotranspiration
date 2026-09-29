#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <float.h>

#include "../include/pet.h"
#include "../include/bmi.h"
#include "../include/pet_serialization.h"
#include "../include/bmi_pet.h"
#include "../include/logger.h"


#define INPUT_VAR_NAME_COUNT 7 //
#define OUTPUT_VAR_NAME_COUNT 1 // water_potential_evaporation_flux; 

static int
Initialize (Bmi *self, const char *cfg_file)
{

#ifdef PET_USE_EWTS
    // Initialize the Error and Warning Trapping System
    #pragma message("evapotranspiration.bmi_pet.Initialize: PET_USE_EWTS ON")
    EwtsInit(PET_MODULE_ID, true);
#else
    #pragma message("evapotranspiration.bmi_pet.Initialize: PET_USE_EWTS OFF")
#endif

    if (self == NULL || self->data == NULL) {
        LOG(FATAL, "Initialize failed: self or self->data is NULL");
        return BMI_FAILURE;
    }

    pet_model *pet = (pet_model *) self->data;
    LOG(INFO, "Initializing PET with config file '%s'", cfg_file ? cfg_file : "(null)");

    int config_read_result = read_init_config_pet(pet, cfg_file);
    if (config_read_result == BMI_FAILURE) {
        LOG(FATAL, "Failed to read PET config file '%s'", cfg_file ? cfg_file : "(null)");
        return BMI_FAILURE;
    }

    pet_setup(pet);

    if (pet->bmi.verbose > 1)
        LOG(INFO, "BMI Initialization PET setup completed");

    /*
        We might be taking forcing data from the framework via BMI.
        Or we might be reading in our own forcing data. 
        We need a logical function to determine this functionality.
        We will use the pet->bmi.is_forcing_from_bmi as the boolean option.
        That should be determined in pet_setup.
    */
    if (pet->bmi.is_forcing_from_bmi == 1)
        if (pet->bmi.verbose>1)
	    LOG(INFO, "Using BMI to receive forcing data; forcing file will not be read");
    if (pet->bmi.is_forcing_from_bmi == 0){
        if (pet->bmi.verbose>1)
            LOG(INFO, "Reading forcing data from file '%s'",
                    pet->forcing_file ? pet->forcing_file : "(null)");

        // Figure out the number of lines first (also char count)
	int forcing_line_count, max_forcing_line_length;
	int count_result = read_file_line_counts_pet(pet->forcing_file, &forcing_line_count, &max_forcing_line_length);
        if (count_result == -1) {
            LOG(FATAL, "Configured forcing file could not be opened for reading: '%s'",
                    pet->forcing_file ? pet->forcing_file : "(null)");
            return BMI_FAILURE;
        }
        if (forcing_line_count == 1) {
            LOG(FATAL, "Invalid forcing file (header only): '%s'",
                    pet->forcing_file ? pet->forcing_file : "(null)");
            return BMI_FAILURE;
        }

        // Now initialize empty arrays that depend on number of time steps
        pet->forcing_data_precip_kg_per_m2 = malloc(sizeof(double) * (pet->bmi.num_timesteps + 1));
        pet->forcing_data_surface_pressure_Pa = malloc(sizeof(double) * (pet->bmi.num_timesteps + 1));
        pet->forcing_data_time = malloc(sizeof(long) * (pet->bmi.num_timesteps + 1));
        pet->forcing_data_incoming_shortwave_W_per_m2 = malloc(sizeof(double) * (pet->bmi.num_timesteps + 1));
        pet->forcing_data_incoming_longwave_W_per_m2 = malloc(sizeof(double) * (pet->bmi.num_timesteps + 1));
        pet->forcing_data_specific_humidity_2m_kg_per_kg = malloc(sizeof(double) * (pet->bmi.num_timesteps + 1));
        pet->forcing_data_air_temperature_2m_K = malloc(sizeof(double) * (pet->bmi.num_timesteps + 1));
        pet->forcing_data_u_wind_speed_10m_m_per_s = malloc(sizeof(double) * (pet->bmi.num_timesteps + 1));
        pet->forcing_data_v_wind_speed_10m_m_per_s = malloc(sizeof(double) * (pet->bmi.num_timesteps + 1));
   
        if (pet->forcing_data_precip_kg_per_m2 == NULL ||
            pet->forcing_data_surface_pressure_Pa == NULL ||
            pet->forcing_data_time == NULL ||
            pet->forcing_data_incoming_shortwave_W_per_m2 == NULL ||
            pet->forcing_data_incoming_longwave_W_per_m2 == NULL ||
            pet->forcing_data_specific_humidity_2m_kg_per_kg == NULL ||
            pet->forcing_data_air_temperature_2m_K == NULL ||
            pet->forcing_data_u_wind_speed_10m_m_per_s == NULL ||
            pet->forcing_data_v_wind_speed_10m_m_per_s == NULL) {
            LOG(FATAL, "Memory allocation failed while creating forcing arrays (num_timesteps=%ld)",
                    pet->bmi.num_timesteps);
            return BMI_FAILURE;
        }

        // Now open it again to read the forcings
        FILE* ffp = fopen(pet->forcing_file, "r");
        // Ensure still exists
        if (ffp == NULL) {
	    LOG(FATAL, "Unable to open forcing file '%s'",
                       pet->forcing_file ? pet->forcing_file : "(null)");
            return BMI_FAILURE;
        }
    
        // Read forcing file and parse forcings
        char line_str[max_forcing_line_length + 1];
        long year, month, day, hour, minute;
        double dsec;
        char* ret = NULL;
        // First read the header line
        ret = fgets(line_str, max_forcing_line_length + 1, ffp);
        if (ret == NULL){
	    LOG(FATAL, "Failed to read header line from forcing file '%s'",
                    pet->forcing_file ? pet->forcing_file : "(null)");
            return BMI_FAILURE;
        }

        if (pet->bmi.verbose > 2) 
            LOG(INFO,"the number of time steps from the forcing file is: %8.6e \n", (double)pet->bmi.num_timesteps);
    
        aorc_forcing_data_pet forcings;
        for (int i = 0; i < pet->bmi.num_timesteps; i++) {
            ret = fgets(line_str, max_forcing_line_length + 1, ffp);  // read in a line of AORC data.
            if (ret == NULL) {
		LOG(FATAL,
                        "Unexpected EOF or read failure in forcing file '%s' at timestep %d of %ld",
                        pet->forcing_file ? pet->forcing_file : "(null)",
                        i, pet->bmi.num_timesteps);
                fclose(ffp);
                return BMI_FAILURE;
            }		
            parse_aorc_line_pet(line_str, &year, &month, &day, &hour, &minute, &dsec, &forcings);
            pet->forcing_data_precip_kg_per_m2[i] = forcings.precip_kg_per_m2 * ((double)pet->bmi.time_step_size_s);
            if (pet->bmi.verbose >4)
		LOG(DEBUG, "forcing[%d] precip=%f", i, pet->forcing_data_precip_kg_per_m2[i]);
            pet->forcing_data_surface_pressure_Pa[i] = forcings.surface_pressure_Pa;
            if (pet->bmi.verbose >4)
                LOG(DEBUG, "surface pressure %f \n", pet->forcing_data_surface_pressure_Pa[i]);
            pet->forcing_data_incoming_longwave_W_per_m2[i] = forcings.incoming_longwave_W_per_m2;
            if (pet->bmi.verbose >4)
                LOG(DEBUG, "longwave %f \n", pet->forcing_data_incoming_longwave_W_per_m2[i]);
            pet->forcing_data_incoming_shortwave_W_per_m2[i] = forcings.incoming_shortwave_W_per_m2;
            if (pet->bmi.verbose >4)
                LOG(DEBUG, "shortwave %f \n", pet->forcing_data_incoming_shortwave_W_per_m2[i]);
            pet->forcing_data_specific_humidity_2m_kg_per_kg[i] = forcings.specific_humidity_2m_kg_per_kg;
            if (pet->bmi.verbose >4)
                LOG(DEBUG, "humidity %f \n", pet->forcing_data_specific_humidity_2m_kg_per_kg[i]);
            pet->forcing_data_air_temperature_2m_K[i] = forcings.air_temperature_2m_K;
            if (pet->bmi.verbose >4)
                LOG(DEBUG, "air temperature %f \n", pet->forcing_data_air_temperature_2m_K[i]);
            pet->forcing_data_u_wind_speed_10m_m_per_s[i] = forcings.u_wind_speed_10m_m_per_s;
            if (pet->bmi.verbose >4)
                LOG(DEBUG, "u wind speed %f \n", pet->forcing_data_u_wind_speed_10m_m_per_s[i]);
            pet->forcing_data_v_wind_speed_10m_m_per_s[i] = forcings.v_wind_speed_10m_m_per_s;
            if (pet->bmi.verbose >4)
                LOG(DEBUG, "v wind speed %f \n", pet->forcing_data_v_wind_speed_10m_m_per_s[i]);
    
    
            pet->forcing_data_time[i] = forcings.time;
            if (i == 0) {
                pet->bmi.current_time =forcings.time;
                pet->bmi.starting_time = forcings.time;
            }
        }
        fclose(ffp);
    }

    // Set the current time step to the first item in the forcing time series.
    // But should this be an option? Would we ever initialize to a point in the
    //     middle of a forcing file?
    pet->bmi.current_step = 0;

    LOG(INFO,
        "PET initialized successfully: forcing_from_bmi=%d num_timesteps=%ld dt=%f current_step=%ld",
            pet->bmi.is_forcing_from_bmi,
            pet->bmi.num_timesteps,
            pet->bmi.time_step_size_s,
            pet->bmi.current_step);
    return BMI_SUCCESS;
}

static int 
Update (Bmi *self)
{
    pet_model *pet = (pet_model *) self->data;
 
    if (self == NULL || self->data == NULL) {
        LOG(FATAL, "Update failed: self or self->data is NULL");
        return BMI_FAILURE;
    }
 
    if (pet->bmi.verbose > 1)
        LOG(DEBUG, "BMI Update PET: current_step=%ld current_time=%f",
                pet->bmi.current_step, pet->bmi.current_time);

    //run_pet(pet);
    if (run_pet(pet) != BMI_SUCCESS) {
        LOG(FATAL, "Update failed: run_pet returned failure");
        return BMI_FAILURE;
    }

    pet->bmi.current_time_step += pet->bmi.time_step_size_s; // Seconds since start of run
    pet->bmi.current_step +=1;                            // time steps since start of run
    pet->bmi.current_time += pet->bmi.time_step_size_s;   // Seconds since 1970

    if (pet->bmi.verbose > 3)
        LOG(DEBUG, "BMI Update PET complete: new_step=%ld new_time=%f",
                pet->bmi.current_step, pet->bmi.current_time);
    return BMI_SUCCESS;
}

// JLG: Implementing a more standard version of update_until here...

static int 
Update_until (Bmi *self, double t)
{
    // https://bmi.readthedocs.io/en/latest/#update-until
    // "the time argument can be a non-integral multiple of time steps"

    pet_model* pet = (pet_model *) self->data;
    
    double dt;
    double now;

    if (self->get_time_step(self, &dt) == BMI_FAILURE) {
        LOG(FATAL, "Update_until failed: unable to get model time step");
        return BMI_FAILURE;
    }
    if(dt == 0.0) {
	LOG(FATAL, "Update_until failed: dt=0.0");
        return BMI_FAILURE;
    }
    if (self->get_current_time(self, &now) == BMI_FAILURE) {
        LOG(FATAL, "Update_until failed: unable to get current model time");
        return BMI_FAILURE;
    }

    {
    
    int n;
    double frac;
    const double n_steps = (t - now) / dt;
    for (n=0; n<(int)n_steps; n++) {
        if (Update(self) == BMI_FAILURE) {
            LOG(FATAL, "Update_until failed during step %d of %d", n + 1, (int)n_steps);
            return BMI_FAILURE;
        }
    }
    frac = n_steps - (int)n_steps;

    if (frac > 0) {
        LOG(WARNING,
                "Update_until requested fractional timestep: target=%f current=%f dt=%f fractional_part=%f",
                t, now, dt, frac);

        pet->bmi.time_step_size_s = frac * dt;
        if (Update(self) == BMI_FAILURE) {
            LOG(FATAL, "Update_until failed while processing fractional timestep");
            pet->bmi.time_step_size_s = dt;
            return BMI_FAILURE;
        }
        pet->bmi.time_step_size_s = dt;
    }

    }

  return BMI_SUCCESS;
}
/*static int
Update_until(Bmi *self, double t)
{

    pet_model* pet = ((pet_model *) self->data);
    
    // Since this model's time units are seconds, it is assumed that the param is either a valid time in seconds, a
    // relative number of time steps into the future, or invalid

    if (pet->bmi.verbose >= 1)
        printf("running update until time %lld \n", t);

    // Don't support negative parameter values
    if (t < 0.0)
        return BMI_FAILURE;

    // jmframe troubleshooting. delete this printout
    if (pet->bmi.verbose > 3)
        printf("t is greater than zero, please delete this printout\n");

    // Don't continue if current time is at or beyond end time (or we can't determine this)
    double current_time = pet->bmi.current_time_step;
    double end_time = 0;
    int current_time_result = self->get_current_time(self, &current_time);
    if (current_time_result == BMI_FAILURE)
        return BMI_FAILURE;

    // jmframe troubleshooting. delete this printout
    if (pet->bmi.verbose > 3)
        printf("passed current time result, please delete this printout\n");

    int end_time_result = self->get_end_time(self, &end_time);

    // jmframe troubleshooting. Please delete this printout
    if (pet->bmi.verbose > 3){
        printf("end time %8.6e, please delete this printout\n", &end_time);
        printf("the current time is %8.6e \n", current_time);
        printf("the end time result is %d \n", end_time_result);
    }

    if (end_time_result == BMI_FAILURE || current_time >= end_time){
        printf("BMI Failued due to end_time_result OR current time greater than end time \n");
        if (end_time_result == BMI_FAILURE)
            printf("end_time_result failed %s \n");
        if (current_time >= end_time)
            printf("current_time %8.6e is greater than end time %8.6e \n", current_time, end_time);
        return BMI_FAILURE;
    }

    // jmframe troubleshooting. Please delete this printout
    if (pet->bmi.verbose > 3)
        printf("passed end time result, please delete this printout\n");

    // Handle easy case of t == current_time by just returning success
    if (t == current_time)
        return BMI_SUCCESS;

    // jmframe troubleshooting. Please delete this printout
    if (pet->bmi.verbose > 3)
        printf("About to extract future time, please delete this printout\n");

    // First, determine if t is some future time that will be arrived at exactly after some number of future time steps
    int is_exact_future_time = (t == end_time) ? TRUE : FALSE;
    // Compare to time step endings unless obvious that t lines up (i.e., t == end_time) or doesn't (t <= current_time)
    if (is_exact_future_time == FALSE && t > current_time) {
        int future_time_step = pet->bmi.current_time_step;
        double future_time_step_time = current_time;
        while (future_time_step < pet->bmi.num_timesteps && future_time_step_time < end_time) {
            future_time_step_time += pet->bmi.time_step_size_s;
            if (future_time_step_time == t) {
                is_exact_future_time = TRUE;
                break;
            }
        }
    }

    // jmframe troubleshooting. Please delete
    if (pet->bmi.verbose > 3)
        printf("Extracted future time, please delete this printout\n");

    // If it is an exact time, advance to that time step
    if (is_exact_future_time == TRUE) {
        while (current_time < t) {
            // jmframe troubleshooting. Please delete
            printf("Updating PET with exact time\n");
            
            self->update(self);
            
            // jmframe troubleshooting. Please delete
            if (pet->bmi.verbose > 3){
                printf("current time step %f \n", pet->bmi.current_time_step);
                printf("current step %f \n", pet->bmi.current_step);
                printf("current time %f \n", pet->bmi.current_time);
            }

        }
        return BMI_SUCCESS;
    }

    // jmframe troubleshooting. Please delete
    if (pet->bmi.verbose > 3)
        printf("Checked if exact time, please delete this printout \n");

    // If t is not an exact time, it could be a number of time step forward to proceed

    // The model doesn't support partial time step value args (i.e., fractions)
    int t_int = (int) t;
    if ((t - ((double)t_int)) != 0)
        return BMI_FAILURE;

    // jmframe troubleshooting. Please delete
    if (pet->bmi.verbose > 3)
        printf("Checked setting t_int to t, please delete this line \n");

    // Keep in mind the current_time_step hasn't been processed yet (hence, using <= for this test)
    // E.g., if (unprocessed) current_time_step = 0, t = 2, num_timesteps = 2, this is valid a valid t (run 0, run 1)
    if ((pet->bmi.current_step + t_int) <= pet->bmi.num_timesteps) {
        for (int i = 0; i < t_int; i++){

            // jmframe troubleshooting. Please delete
            if (pet->bmi.verbose > 3)
                printf("Updating PET with time step\n");
            
            // Call the update function, instead of run
            // because update also advances the bmi time
            self->update(self);

            // jmframe troubleshooting. Please delete
            if (pet->bmi.verbose > 3){
                printf("current time step %f \n", pet->bmi.current_time_step);
                printf("current step %ld \n", pet->bmi.current_step);
                printf("current time %f \n", pet->bmi.current_time);
            }
        } // end time loop
        return BMI_SUCCESS;
    }

    // If we arrive here, t wasn't an exact time at end of a time step or a valid relative time step jump, so invalid.
    return BMI_FAILURE;
    
}*/


pet_model *
new_bmi_pet()
{
    pet_model *data;
    data = (pet_model*) calloc(1, sizeof(pet_model));

    return data;
}

static int
Finalize (Bmi *self)
{
    if (self == NULL || self->data == NULL) {
        LOG(WARNING, "Finalize called with NULL self or self->data");
        return BMI_FAILURE;
    }

    pet_model *pet = (pet_model *) self->data;

    if (pet->bmi.run_unit_tests == 1) {
        LOG(INFO, "Running PET unit tests during Finalize");
        pet_unit_tests(pet);
    }

    free_pet_model(pet);
    self->data = (void*)new_bmi_pet();

    LOG(INFO, "PET finalized successfully");
    return BMI_SUCCESS;
}



//---------------------------------------------------------------------------------------------------------------------
static int 
Get_start_time (Bmi *self, double * time)
{
    // TODO: Get the real starting time, not zero. 
    // jmframe: I think this should be the time of the first line in the forcing file. In seconds since 1970
    *time = 0.0;
    return BMI_SUCCESS;
}

//---------------------------------------------------------------------------------------------------------------------
static const char *output_var_names[OUTPUT_VAR_NAME_COUNT] = {
  "water_potential_evaporation_flux",
};

//---------------------------------------------------------------------------------------------------------------------
static const char *output_var_types[OUTPUT_VAR_NAME_COUNT] = {
  "double",
};

//---------------------------------------------------------------------------------------------------------------------
static const int output_var_item_count[OUTPUT_VAR_NAME_COUNT] = {
  1,
};

//---------------------------------------------------------------------------------------------------------------------
static const char *output_var_units[OUTPUT_VAR_NAME_COUNT] = {
  "m s-1",
};

//---------------------------------------------------------------------------------------------------------------------
static const int output_var_grids[OUTPUT_VAR_NAME_COUNT] = {
        0,
};

//---------------------------------------------------------------------------------------------------------------------
static const char *output_var_locations[OUTPUT_VAR_NAME_COUNT] = {
        "node",
};

//---------------------------------------------------------------------------------------------------------------------
static const int input_var_item_count[INPUT_VAR_NAME_COUNT] = {
  1,
  1,
  1,
  1,
  1,
  1,
  1
};

//---------------------------------------------------------------------------------------------------------------------
static const char input_var_grids[INPUT_VAR_NAME_COUNT] = {
        0,
        0,
        0,
        0,
        0,
        0,
        0
};

//---------------------------------------------------------------------------------------------------------------------
static const char *input_var_locations[INPUT_VAR_NAME_COUNT] = {
        "node",
        "node",
        "node",
        "node",
        "node",
        "node",
        "node"
};




//---------------------------------------------------------------------------------------------------------------------
// Don't forget to update Get_value/Get_value_at_indices 
// (and setter) implementation if these are adjusted
static const char *input_var_names[INPUT_VAR_NAME_COUNT] = {
    "land_surface_radiation~incoming~longwave__energy_flux",
    "land_surface_air__pressure",
    "atmosphere_air_water~vapor__relative_saturation",
    "land_surface_radiation~incoming~shortwave__energy_flux",
    "land_surface_air__temperature",
    "land_surface_wind__x_component_of_velocity",
    "land_surface_wind__y_component_of_velocity"
};

//---------------------------------------------------------------------------------------------------------------------
static const char *input_var_types[INPUT_VAR_NAME_COUNT] = {
  "double",
  "double",
  "double",
  "double",
  "double",
  "double",
  "double"
};

//---------------------------------------------------------------------------------------------------------------------
static const char *input_var_units[INPUT_VAR_NAME_COUNT] = {
  "W m-2",
  "Pa",
  "kg kg-1",
  "W m-2",
  "K",
  "m s-1",
  "m s-1"
};

//---------------------------------------------------------------------------------------------------------------------

static int Get_end_time (Bmi *self, double * time)
{
/*  
  Get_start_time(self, time);

  *time += (((pet_model *) self->data)->bmi.num_timesteps * 
            ((pet_model *) self->data)->bmi.time_step_size_s);

  return BMI_SUCCESS;*/

    // JG EDIT
    pet_model *pet;
    pet = (pet_model *) self->data;
    Get_start_time(self, time);
    
    // see if numsteps is set via config
    if (pet->bmi.num_timesteps == 1){
        // if ==1 (not really defined) set to FLT_MAX macro via float.h
        // See https://bmi.readthedocs.io/en/latest/#get-end-time
        *time += FLT_MAX;
        return BMI_SUCCESS;
    }
    else {
        // otherwise, set via numsteps
        *time += pet->bmi.num_timesteps * pet->bmi.time_step_size_s;
        return BMI_SUCCESS;
    }
    
}


//---------------------------------------------------------------------------------------------------------------------

static int Get_time_step (Bmi *self, double * dt)
{
    *dt = ((pet_model *) self->data)->bmi.time_step_size_s;
    return BMI_SUCCESS;
}

static int Get_time_units (Bmi *self, char * units)
{
    strncpy (units, "s", BMI_MAX_UNITS_NAME);
    return BMI_SUCCESS;
}

//---------------------------------------------------------------------------------------------------------------------

static int Get_current_time (Bmi *self, double * time)
{
    Get_start_time(self, time);
    if (((pet_model *) self->data)->bmi.verbose > 2){
	LOG(DEBUG, "Current model time step: %8.6e",
                ((pet_model *) self->data)->bmi.current_time_step);
    }
    *time += (((pet_model *) self->data)->bmi.current_step * 
              ((pet_model *) self->data)->bmi.time_step_size_s);
    return BMI_SUCCESS;
} // end Get_current_time


//---------------------------------------------------------------------------------------------------------------------
/** Count the number of values in a delimited string representing an array of values. */

static int count_delimited_values(char* string_val, char* delimiter)
{
    char *copy, *copy_to_free, *value;
    int count = 0;

    // Make duplicate to avoid changing original string
    // Then work on copy, but keep 2nd pointer to copy so that memory can be freed
    copy_to_free = copy = strdup(string_val);
    while ((value = strsep(&copy, delimiter)) != NULL)
        count++;
    free(copy_to_free);
    return count;
} //end count_delimited_values

//---------------------------------------------------------------------------------------------------------------------
int read_file_line_counts_pet(const char* file_name, int* line_count, int* max_line_length)
{

    *line_count = 0;
    *max_line_length = 0;
    int current_line_length = 0;
    FILE* fp = fopen(file_name, "r");
    // Ensure exists
    if (fp == NULL) {
	LOG(FATAL, "read_file_line_counts_pet failed to open file '%s'",
                   file_name ? file_name : "(null)");
        return -1;
    }
    int seen_non_whitespace = 0;
    int c; //EOF is a negative constant...and char may be either signed OR unsigned
    //depending on the compiler, system, architecture, etc.  So there are cases
    //where this loop could go infinite comparing EOF to unsigned char
    //the return of fgetc is int, and should be stored as such!
    //https://stackoverflow.com/questions/35356322/difference-between-int-and-char-in-getchar-fgetc-and-putchar-fputc
    for (c = fgetc(fp); c != EOF; c = fgetc(fp)) {
        // keep track if this line has seen any char other than space or tab
        if (c != ' ' && c != '\t' && c != '\n')
            seen_non_whitespace++;
        // Update line count, reset non-whitespace count, adjust max_line_length (if needed), and reset current line count
        if (c == '\n') {
            *line_count += 1;
            seen_non_whitespace = 0;
            if (current_line_length > *max_line_length)
                *max_line_length = current_line_length;
            current_line_length = 0;
        }
        else {
            current_line_length += 1;
        }
    }
    fclose(fp);

    // If we saw some non-whitespace char on last line, assume last line didn't have its own \n, so count needs to be
    // incremented by 1.
    if (seen_non_whitespace > 0) {
        *line_count += 1;
    }

    // Before returning, increment the max line length by 1, since the \n will be on the line also.
    *max_line_length += 1;

    LOG(DEBUG, "Counted file '%s': line_count=%d max_line_length=%d",
        file_name ? file_name : "(null)", *line_count, *max_line_length);
    return 0;
}  // end: read_file_line_counts

//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
int read_init_config_pet(pet_model* model, const char* config_file)
{
    int config_line_count, max_config_line_length;

    if (model == NULL) {
        LOG(FATAL, "read_init_config_pet called with NULL model");
        return BMI_FAILURE;
    }

    if (config_file == NULL) {
        LOG(FATAL, "read_init_config_pet called with NULL config_file");
        return BMI_FAILURE;
    }

    int count_result = read_file_line_counts_pet(config_file, &config_line_count, &max_config_line_length);
    if (count_result == -1) {
        LOG(SEVERE, "Unable to inspect PET config file '%s'", config_file);
        return BMI_FAILURE;
    }

    FILE* fp = fopen(config_file, "r");
    if (fp == NULL) {
        LOG(SEVERE, "Unable to open PET config file '%s'", config_file);
        return BMI_FAILURE;
    }

    LOG(INFO, "Reading PET configuration from '%s'", config_file);

    char* ret = NULL;
    char config_line[max_config_line_length + 1];

    for (int i = 0; i < config_line_count; i++) {
        char *param_key, *param_value;

        ret = fgets(config_line, max_config_line_length + 1, fp);
        if (ret == NULL) {
            LOG(SEVERE, "Failed to read config line %d from '%s'", i + 1, config_file);
            fclose(fp);
            return BMI_FAILURE;
        }

        char* config_line_ptr = config_line;
        config_line_ptr = strsep(&config_line_ptr, "\n");
        param_key = strsep(&config_line_ptr, "=");
        param_value = strsep(&config_line_ptr, "=");

        if (param_key == NULL || param_value == NULL) {
            LOG(WARNING, "Skipping malformed config line %d in '%s'", i + 1, config_file);
            continue;
        }

        if (strcmp(param_key, "verbose") == 0) {
            model->bmi.verbose = (int) strtol(param_value, NULL, 10);
            if (model->bmi.verbose > 2) {
                LOG(INFO, "Verbose logging enabled: level=%d (high-detail troubleshooting)", model->bmi.verbose);
            }
            else if (model->bmi.verbose > 1) {
                LOG(INFO, "Verbose logging enabled: level=%d (basic troubleshooting)", model->bmi.verbose);
            }
            continue;
        }

        if (strcmp(param_key, "pet_method") == 0) {
            model->pet_method = (int) strtol(param_value, NULL, 10);
            if (model->bmi.verbose > 1) {
                LOG(DEBUG, "Set pet_method from config: %d", model->pet_method);
            }
            continue;
        }

        if (strcmp(param_key, "yes_aorc") == 0) {
            model->pet_options.yes_aorc = (int) strtol(param_value, NULL, 10);
            if (model->bmi.verbose >= 2) {
                LOG(DEBUG, "Set yes_aorc from config: %d", model->pet_options.yes_aorc);
            }
            continue;
        }

        if (strcmp(param_key, "forcing_file") == 0) {
            model->forcing_file = strdup(param_value);
            if (model->forcing_file == NULL) {
                LOG(FATAL, "Failed to allocate forcing_file from config");
                fclose(fp);
                return BMI_FAILURE;
            }

            if (strcmp(model->forcing_file, "BMI") == 0) {
                model->bmi.is_forcing_from_bmi = 1;
                if (model->bmi.verbose >= 2) {
                    LOG(DEBUG, "Set forcing_file from config: %s (forcing will come from BMI)", model->forcing_file);
                }
            }
            else {
                if (model->bmi.verbose >= 2) {
                    LOG(DEBUG, "Set forcing_file from config: %s", model->forcing_file);
                }
            }
            continue;
        }

        if (strcmp(param_key, "wind_speed_measurement_height_m") == 0) {
            model->pet_params.wind_speed_measurement_height_m = strtod(param_value, NULL);
            if (model->bmi.verbose >= 2) {
                LOG(DEBUG, "Set wind_speed_measurement_height_m from config: %lf",
                    model->pet_params.wind_speed_measurement_height_m);
            }
            continue;
        }

        if (strcmp(param_key, "humidity_measurement_height_m") == 0) {
            model->pet_params.humidity_measurement_height_m = strtod(param_value, NULL);
            if (model->bmi.verbose >= 2) {
                LOG(DEBUG, "Set humidity_measurement_height_m from config: %lf",
                    model->pet_params.humidity_measurement_height_m);
            }
            continue;
        }

        if (strcmp(param_key, "vegetation_height_m") == 0) {
            model->pet_params.vegetation_height_m = strtod(param_value, NULL);
            if (model->bmi.verbose >= 2) {
                LOG(DEBUG, "Set vegetation_height_m from config: %lf",
                    model->pet_params.vegetation_height_m);
            }
            continue;
        }

        if (strcmp(param_key, "zero_plane_displacement_height_m") == 0) {
            model->pet_params.zero_plane_displacement_height_m = strtod(param_value, NULL);
            if (model->bmi.verbose >= 2) {
                LOG(DEBUG, "Set zero_plane_displacement_height_m from config: %lf",
                    model->pet_params.zero_plane_displacement_height_m);
            }
            continue;
        }

        if (strcmp(param_key, "shortwave_radiation_provided") == 0) {
            model->pet_options.shortwave_radiation_provided = (int) strtol(param_value, NULL, 10);
            if (model->bmi.verbose >= 2) {
                LOG(DEBUG, "Set shortwave_radiation_provided from config: %d",
                    model->pet_options.shortwave_radiation_provided);
            }
            continue;
        }

        if (strcmp(param_key, "momentum_transfer_roughness_length_m") == 0) {
            model->pet_params.momentum_transfer_roughness_length_m = strtod(param_value, NULL);
            if (model->bmi.verbose >= 2) {
                LOG(DEBUG, "Set momentum_transfer_roughness_length_m from config: %lf",
                    model->pet_params.momentum_transfer_roughness_length_m);
            }
            continue;
        }

        if (strcmp(param_key, "heat_transfer_roughness_length_m") == 0) {
            model->pet_params.heat_transfer_roughness_length_m = strtod(param_value, NULL);
            if (model->bmi.verbose >= 2) {
                LOG(DEBUG, "Set heat_transfer_roughness_length_m from config: %lf",
                    model->pet_params.heat_transfer_roughness_length_m);
            }
            continue;
        }

        if (strcmp(param_key, "surface_longwave_emissivity") == 0) {
            model->surf_rad_params.surface_longwave_emissivity = strtod(param_value, NULL);
            if (model->bmi.verbose >= 2) {
                LOG(DEBUG, "Set surface_longwave_emissivity from config: %lf",
                    model->surf_rad_params.surface_longwave_emissivity);
            }
            continue;
        }

        if (strcmp(param_key, "surface_shortwave_albedo") == 0) {
            model->surf_rad_params.surface_shortwave_albedo = strtod(param_value, NULL);
            if (model->bmi.verbose >= 2) {
                LOG(DEBUG, "Set surface_shortwave_albedo from config: %lf",
                    model->surf_rad_params.surface_shortwave_albedo);
            }
            continue;
        }

        if (strcmp(param_key, "latitude_degrees") == 0) {
            model->solar_params.latitude_degrees = strtod(param_value, NULL);
            if (model->bmi.verbose >= 2) {
                LOG(DEBUG, "Set latitude_degrees from config: %lf",
                    model->solar_params.latitude_degrees);
            }
            continue;
        }

        if (strcmp(param_key, "longitude_degrees") == 0) {
            model->solar_params.longitude_degrees = strtod(param_value, NULL);
            if (model->bmi.verbose >= 2) {
                LOG(DEBUG, "Set longitude_degrees from config: %lf",
                    model->solar_params.longitude_degrees);
            }
            continue;
        }

        if (strcmp(param_key, "site_elevation_m") == 0) {
            model->solar_params.site_elevation_m = strtod(param_value, NULL);
            if (model->bmi.verbose >= 2) {
                LOG(DEBUG, "Set site_elevation_m from config: %lf",
                    model->solar_params.site_elevation_m);
            }
            continue;
        }

        if (strcmp(param_key, "time_step_size_s") == 0) {
            model->bmi.time_step_size_s = strtod(param_value, NULL);
            if (model->bmi.verbose >= 2) {
                LOG(DEBUG, "Set time_step_size_s from config: %f",
                    model->bmi.time_step_size_s);
            }
            continue;
        }

        if (strcmp(param_key, "num_timesteps") == 0) {
            model->bmi.num_timesteps = strtol(param_value, NULL, 10);
            if (model->bmi.verbose >= 2) {
                LOG(DEBUG, "Set num_timesteps from config: %ld",
                    model->bmi.num_timesteps);
            }
            continue;
        }

        if (strcmp(param_key, "run_unit_tests") == 0) {
            model->bmi.run_unit_tests = (int) strtol(param_value, NULL, 10);
            if (model->bmi.verbose >= 2) {
                LOG(DEBUG, "Set run_unit_tests from config: %d",
                    model->bmi.run_unit_tests);
            }
            continue;
        }

        LOG(WARNING, "Unrecognized config key '%s' in '%s'", param_key, config_file);
    }

    fclose(fp);
    return BMI_SUCCESS;
}

static int Get_var_type (Bmi *self, const char *name, char * type)
{
    // special serialization messages
    if (strcmp(name, "serialization_create") == 0) {
        strncpy(type, "uint64_t", BMI_MAX_TYPE_NAME);
        return BMI_SUCCESS;
    } else if (strcmp(name, "serialization_size") == 0) {
        strncpy(type, "uint64_t", BMI_MAX_TYPE_NAME);
        return BMI_SUCCESS;
    } else if (strcmp(name, "serialization_state") == 0) {
        strncpy(type, "char", BMI_MAX_TYPE_NAME);
        return BMI_SUCCESS;
    } else if (strcmp(name, "serialization_free") == 0) {
        strncpy(type, "int", BMI_MAX_TYPE_NAME);
        return BMI_SUCCESS;
    } else if (strcmp(name, "reset_time") == 0) {
        strncpy(type, "double", BMI_MAX_TYPE_NAME);
        return BMI_SUCCESS;
    }
    // Check to see if in output array first
    for (int i = 0; i < OUTPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, output_var_names[i]) == 0) {
            strncpy(type, output_var_types[i], BMI_MAX_TYPE_NAME);
            return BMI_SUCCESS;
        }
    }
    // Then check to see if in input array
    for (int i = 0; i < INPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, input_var_names[i]) == 0) {
            strncpy(type, input_var_types[i], BMI_MAX_TYPE_NAME);
            return BMI_SUCCESS;
        }
    }
    // If we get here, it means the variable name wasn't recognized
    type[0] = '\0';
    LOG(FATAL, "Get_var_type failed: unknown or inaccessible variable '%s'",
                name ? name : "(null)");
    return BMI_FAILURE;
}

static int Get_var_itemsize (Bmi *self, const char *name, int * size)
{
    char type[BMI_MAX_TYPE_NAME];
    int type_result = Get_var_type(self, name, type);
    if (type_result != BMI_SUCCESS) {
	LOG(FATAL, "Get_var_itemsize failed: unknown or inaccessible variable '%s'",
                name ? name : "(null)");
        return BMI_FAILURE;
    }

    if (strcmp (type, "double") == 0) {
        *size = sizeof(double);
        return BMI_SUCCESS;
    }
    else if (strcmp (type, "float") == 0) {
        *size = sizeof(float);
        return BMI_SUCCESS;
    }
    else if (strcmp (type, "int") == 0) {
        *size = sizeof(int);
        return BMI_SUCCESS;
    }
    else if (strcmp (type, "short") == 0) {
        *size = sizeof(short);
        return BMI_SUCCESS;
    }
    else if (strcmp (type, "long") == 0) {
        *size = sizeof(long);
        return BMI_SUCCESS;
    }
    else if (strcmp(type, "uint64_t") == 0) {
        *size = sizeof(uint64_t);
        return BMI_SUCCESS;
    }
    else if (strcmp(type, "char") == 0) {
        *size = sizeof(char);
        return BMI_SUCCESS;
    }
    else {
        *size = 0;
	LOG(FATAL, "Get_var_itemsie failed: unknown or inaccessible variable '%s'",
                name ? name : "(null)");
        return BMI_FAILURE;
    }
}



//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
static int Get_var_location (Bmi *self, const char *name, char * location)
{
    // Check to see if in output array first
    for (int i = 0; i < OUTPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, output_var_names[i]) == 0) {
            strncpy(location, output_var_locations[i], BMI_MAX_LOCATION_NAME);
            return BMI_SUCCESS;
        }
    }
    // Then check to see if in input array
    for (int i = 0; i < INPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, input_var_names[i]) == 0) {
            strncpy(location, input_var_locations[i], BMI_MAX_LOCATION_NAME);
            return BMI_SUCCESS;
        }
    }
    // If we get here, it means the variable name wasn't recognized
    location[0] = '\0';
    LOG(FATAL, "Get_var_location failed: unknown or inaccessible variable '%s'",
                name ? name : "(null)");
    return BMI_FAILURE;
}

//------------------------------------------------------------------------------
static int Get_var_grid(Bmi *self, const char *name, int *grid)
{

    // Check to see if in output array first
    for (int i = 0; i < OUTPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, output_var_names[i]) == 0) {
            *grid = output_var_grids[i];
            return BMI_SUCCESS;
        }
    }
    // Then check to see if in input array
    for (int i = 0; i < INPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, input_var_names[i]) == 0) {
            *grid = input_var_grids[i];
            return BMI_SUCCESS;
        }
    }
    // If we get here, it means the variable name wasn't recognized
    // grid[0] = '\0';
    *grid = -1;
    LOG(FATAL, "Get_var_grid failed: unknown or inaccessible variable '%s'",
                name ? name : "(null)");
    return BMI_FAILURE;
}

// ***********************************************************
// ********* BMI: VARIABLE GETTER & SETTER FUNCTIONS *********
// ***********************************************************
static int Get_value_ptr (Bmi *self, const char *name, void **dest)
{
    void *src = NULL;
    /***********************************************************/
    /***********    OUTPUT   ***********************************/
    /***********************************************************/
    if (strcmp (name, "water_potential_evaporation_flux") == 0) {
        pet_model *pet;
        pet = (pet_model *) self->data;
        src = (void*)&pet-> pet_m_per_s;
        *dest = src;
        return BMI_SUCCESS;
    }

    /***********************************************************/
    /***********    INPUT    ***********************************/
    /***********************************************************/
    if (strcmp (name, "land_surface_radiation~incoming~longwave__energy_flux") == 0) {
        pet_model *pet;
        pet = (pet_model *) self->data;
        src = (void*)&pet->aorc.incoming_longwave_W_per_m2;
        *dest = src;
        return BMI_SUCCESS;
    }
    if (strcmp (name, "land_surface_radiation~incoming~shortwave__energy_flux") == 0) {
        pet_model *pet;
        pet = (pet_model *) self->data;
        src = (void*)&pet->aorc.incoming_shortwave_W_per_m2;
        *dest = src;
        return BMI_SUCCESS;
    }
    if (strcmp (name, "land_surface_air__pressure") == 0) {
        pet_model *pet;
        pet = (pet_model *) self->data;
        src = (void*)&pet->aorc.surface_pressure_Pa;
        *dest = src;
        return BMI_SUCCESS;
    }
    if (strcmp (name, "atmosphere_air_water~vapor__relative_saturation") == 0) {
        pet_model *pet;
        pet = (pet_model *) self->data;
        src = (void*)&pet->aorc.specific_humidity_2m_kg_per_kg;
        *dest = src;
        return BMI_SUCCESS;
    }
    if (strcmp (name, "land_surface_air__temperature") == 0) {
        pet_model *pet;
        pet = (pet_model *) self->data;
        src = (void*)&pet->aorc.air_temperature_2m_K;
        *dest = src;
        return BMI_SUCCESS;
    }
    if (strcmp (name, "land_surface_wind__x_component_of_velocity") == 0) {
        pet_model *pet;
        pet = (pet_model *) self->data;
        src = (void*)&pet->aorc.u_wind_speed_10m_m_per_s;
        *dest = src;
        return BMI_SUCCESS;
    }
    if (strcmp (name, "land_surface_wind__y_component_of_velocity") == 0) {
        pet_model *pet;
        pet = (pet_model *) self->data;
        src = (void*)&pet->aorc.v_wind_speed_10m_m_per_s;
        *dest = src;
        return BMI_SUCCESS;
    }

    // serialiation
    if (strcmp(name, "serialization_size") == 0) {
        pet_model *pet = (pet_model *)self->data;
        if (pet->serialized == NULL) {
            // Log(SEVERE, "PET serialization has not been.");
	    LOG(FATAL, "serialization_size requested before serialization state was created");
            return BMI_FAILURE;
        }
        *dest = &pet->serialized_length;
        return BMI_SUCCESS;
    }
    if (strcmp(name, "serialization_state") == 0) {
        pet_model *pet = (pet_model *)self->data;
        if (pet->serialized == NULL) {
            // Log(SEVERE, "PET serialization has not been.");
	    LOG(FATAL, "serialization_state requested before serialization state was created");
            return BMI_FAILURE;
        }
        *dest = pet->serialized;
        return BMI_SUCCESS;
    }

    LOG(FATAL, "Get_value_ptr failed: unknown or inaccessible variable '%s'",
                name ? name : "(null)");
    return BMI_FAILURE;
}

//------------------------------------------------------------------------------
static int Get_value_at_indices (Bmi *self, const char *name, void *dest, int * inds, int len)
{
    void *src = NULL;
    int itemsize = 0;

    if (self->get_value_ptr(self, name, &src) == BMI_FAILURE){
	LOG(FATAL, "Get_value_at_indices failed: unknown or inaccessible variable '%s'",
                name ? name : "(null)");
        return BMI_FAILURE;
    }	

    if (self->get_var_itemsize(self, name, &itemsize) == BMI_FAILURE){
	LOG(FATAL, "Get_value_at_indices failed: unknown item size for variable '%s'",
                name ? name : "(null)");
        return BMI_FAILURE;
    }

    { /* Copy the data */
        size_t i;
        size_t offset;
        char * ptr;
        for (i=0, ptr=(char*)dest; i<len; i++, ptr+=itemsize) {
            offset = inds[i] * itemsize;
            memcpy (ptr, (char*)src + offset, itemsize);
        }
    }

    return BMI_SUCCESS;
}

//------------------------------------------------------------------------------
static int Get_value(Bmi * self, const char * name, void *dest)
{
    void *src = NULL;
    int nbytes = 0;

    if (self->get_value_ptr (self, name, &src) == BMI_FAILURE) {
        LOG(FATAL, "Get_value failed: unknown or inaccessible variable '%s'",
                name ? name : "(null)");
        return BMI_FAILURE;
    }

    if (self->get_var_nbytes (self, name, &nbytes) == BMI_FAILURE){
        LOG(FATAL, "Get_value failed: unable to determine size for variable '%s'",
                name ? name : "(null)");
        return BMI_FAILURE;
    }
    memcpy(dest, src, nbytes);

    return BMI_SUCCESS;
}

static int Set_value (Bmi *self, const char *name, void *array)
{
    // special cases for serialization
    if (strcmp(name, "serialization_create") == 0) {
        return new_serialized_pet(self);
    }
    else if (strcmp(name, "serialization_state") == 0) {
        return load_serialized_pet(self, (char*)array);
    }
    else if (strcmp(name, "serialization_free") == 0) {
        return free_serialized_pet(self);
    } else if (strcmp(name, "reset_time") == 0) {
        pet_model *model = (pet_model *)self->data;
        // set to 0 in init; used in indexing into forcing data
        model->bmi.current_step = 0;
        // just used in reporting
        model->bmi.current_time_step = 0.0;
        // recover starting time from the config; doesn't seem to be used in processing
        model->bmi.current_time = model->bmi.starting_time;
        return BMI_SUCCESS;
    }

    void *dest = NULL;
    int nbytes = 0;

    if (self->get_value_ptr(self, name, &dest) == BMI_FAILURE) {
        LOG(FATAL, "Set_value failed: unknown or inaccessible variable '%s'",
                name ? name : "(null)");
        return BMI_FAILURE;
    }

    if (self->get_var_nbytes(self, name, &nbytes) == BMI_FAILURE) {
        LOG(FATAL, "Set_value failed: unable to determine size for variable '%s'",
                name ? name : "(null)");
        return BMI_FAILURE;
    }

    memcpy(dest, array, nbytes);

    return BMI_SUCCESS;
}


//------------------------------------------------------------------------------
static int Set_value_at_indices (Bmi *self, const char *name, int * inds, int len, void *src)
{
    void * to = NULL;
    int itemsize = 0;

    if (self->get_value_ptr (self, name, &to) == BMI_FAILURE){
	LOG(FATAL, "Set_value_at_indices failed: unknown or inaccessible variable '%s'",
                name ? name : "(null)");
        return BMI_FAILURE;
    }
    if (self->get_var_itemsize(self, name, &itemsize) == BMI_FAILURE){
	LOG(FATAL, "Set_value_at_indices failed: unable to determine size for variable '%s'",
                name ? name : "(null)");
        return BMI_FAILURE;
    }	

    { /* Copy the data */
        size_t i;
        size_t offset;
        char * ptr;
        for (i=0, ptr=(char*)src; i<len; i++, ptr+=itemsize) {
            offset = inds[i] * itemsize;
            memcpy ((char*)to + offset, ptr, itemsize);
        }
    }
    return BMI_SUCCESS;
}

// ***********************************************************
// ************ BMI: MODEL INFORMATION FUNCTIONS *************
// ***********************************************************
static int Get_component_name (Bmi *self, char * name)
{
    strncpy (name, "Potential Evapotranspiration", BMI_MAX_COMPONENT_NAME);
    return BMI_SUCCESS;
}

//----------------------------------------------------------------------
static int Get_input_item_count (Bmi *self, int * count)
{
    *count = INPUT_VAR_NAME_COUNT;
    return BMI_SUCCESS;
}

//----------------------------------------------------------------------
static int Get_input_var_names (Bmi *self, char ** names)
{
    for (int i = 0; i < INPUT_VAR_NAME_COUNT; i++) {
        strncpy (names[i], input_var_names[i], BMI_MAX_VAR_NAME);
    }
    return BMI_SUCCESS;
}

//----------------------------------------------------------------------
static int Get_output_item_count (Bmi *self, int * count)
{
    *count = OUTPUT_VAR_NAME_COUNT;
    return BMI_SUCCESS;
}

//----------------------------------------------------------------------
static int Get_output_var_names (Bmi *self, char ** names)
{
    for (int i = 0; i < OUTPUT_VAR_NAME_COUNT; i++) {
        strncpy (names[i], output_var_names[i], BMI_MAX_VAR_NAME);
    }
    return BMI_SUCCESS;
}

//----------------------------------------------------------------------
static int Get_var_units (Bmi *self, const char *name, char * units)
{
    // Check to see if in output array first
    for (int i = 0; i < OUTPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, output_var_names[i]) == 0) {
            strncpy(units, output_var_units[i], BMI_MAX_UNITS_NAME);
            return BMI_SUCCESS;
        }
    }
    // Then check to see if in input array
    for (int i = 0; i < INPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, input_var_names[i]) == 0) {
            strncpy(units, input_var_units[i], BMI_MAX_UNITS_NAME);
            return BMI_SUCCESS;
        }
    }
    // If we get here, it means the variable name wasn't recognized
    units[0] = '\0';
    LOG(FATAL, "Get_var_units failed: unknown or inaccessible variable '%s'",
                name ? name : "(null)");
    return BMI_FAILURE;
}

//----------------------------------------------------------------------
static int Get_var_nbytes (Bmi *self, const char *name, int * nbytes)
{
    // special serialization messages
    if (strcmp(name, "serialization_create") == 0 || strcmp(name, "serialization_size") == 0) {
        *nbytes = sizeof(uint64_t);
        return BMI_SUCCESS;
    } else if (strcmp(name, "serialization_state") == 0) {
        pet_model *model = (pet_model*)self->data;
        *nbytes = model->serialized_length;
        return BMI_SUCCESS;
    } else if (strcmp(name, "serialization_free") == 0) {
        *nbytes = sizeof(int);
        return BMI_SUCCESS;
    } else if (strcmp(name, "reset_time") == 0) {
        *nbytes = sizeof(double);
        return BMI_SUCCESS;
    }
    int item_size;
    int item_size_result = Get_var_itemsize(self, name, &item_size);
    if (item_size_result != BMI_SUCCESS) {
        LOG(FATAL, "Get_var_nbytes failed: unable to determine size for variable '%s'",
                name ? name : "(null)");
        return BMI_FAILURE;
    }
    int item_count = -1;
    for (int i = 0; i < INPUT_VAR_NAME_COUNT; i++) {
        if (strcmp(name, input_var_names[i]) == 0) {
            item_count = input_var_item_count[i];
            break;
        }
    }
    if (item_count < 1) {
        for (int i = 0; i < OUTPUT_VAR_NAME_COUNT; i++) {
            if (strcmp(name, output_var_names[i]) == 0) {
                item_count = output_var_item_count[i];
                break;
            }
        }
    }
    if (item_count < 1)
        item_count = ((pet_model *) self->data)->bmi.num_timesteps;

    *nbytes = item_size * item_count;
    return BMI_SUCCESS;
}


// ***********************************************************
// **************** BMI: MODEL GRID FUNCTIONS ****************
// ***********************************************************
/* Grid information */
static int Get_grid_rank (Bmi *self, int grid, int * rank)
{
    if (grid == 0) {
        *rank = 1;
        return BMI_SUCCESS;
    }
    else {
        *rank = -1;
	LOG(FATAL,
            "Get_grid_rank failed: unable to determine rank for grid %d", grid);
        return BMI_FAILURE;
    }
}

//----------------------------------------------------------------------
static int Get_grid_size(Bmi *self, int grid, int * size)
{
    if (grid == 0) {
        *size = 1;
        return BMI_SUCCESS;
    }
    else {
        *size = -1;
	LOG(FATAL,
            "Get_grid_size failed: unable to determine size for grid %d", grid);
        return BMI_FAILURE;
    }
}

//---------------------------------------------------------------------------------------------------------------------
static int Get_grid_type (Bmi *self, int grid, char * type)
{
    int status = BMI_FAILURE;

    if (grid == 0) {
        strncpy(type, "scalar", BMI_MAX_TYPE_NAME);
        status = BMI_SUCCESS;
    }
    else {
        type[0] = '\0';
	LOG(FATAL,
            "Get_grid_type failed: unable to determine type for grid %d", grid);
        status = BMI_FAILURE;
    }
    return status;
}

//---------------------------------------------------------------------------------------------------------------------
/* Uniform rectilinear (grid type) */
static int Get_grid_shape(Bmi *self, int grid, int *shape)
{
    LOG(FATAL,
        "Get_grid_shape failed: unable to determine shape for grid %d", grid);
    return BMI_FAILURE;
}

//---------------------------------------------------------------------------------------------------------------------
static int Get_grid_spacing(Bmi *self, int grid, double *spacing)
{
    LOG(FATAL,
        "Get_grid_spacing failed: unable to determine spacing for grid %d", grid);
    return BMI_FAILURE;
}

//---------------------------------------------------------------------------------------------------------------------
static int Get_grid_origin(Bmi *self, int grid, double *origin)
{
    LOG(FATAL,
        "Get_grid_origin failed: unable to determine origin for grid %d", grid);
    return BMI_FAILURE;
}

//---------------------------------------------------------------------------------------------------------------------
/* Non-uniform rectilinear, curvilinear (grid type)*/
static int Get_grid_x(Bmi *self, int grid, double *x)
{
    LOG(FATAL,
            "Get_grid_x failed: unable to determine x value for grid %d", grid);
    return BMI_FAILURE;
}

//---------------------------------------------------------------------------------------------------------------------
static int Get_grid_y(Bmi *self, int grid, double *y)
{
    LOG(FATAL,
        "Get_grid_y failed: unable to determine y value for grid %d", grid);
    return BMI_FAILURE;
}

//---------------------------------------------------------------------------------------------------------------------
static int Get_grid_z(Bmi *self, int grid, double *z)
{
    LOG(FATAL,
        "Get_grid_z failed: unable to determine z value for grid %d", grid);
    return BMI_FAILURE;
}

//---------------------------------------------------------------------------------------------------------------------
/*Unstructured (grid type)*/
static int Get_grid_node_count(Bmi *self, int grid, int *count)
{
    LOG(FATAL,
        "Get_grid_node_count failed: unable to determine node count for grid %d", grid);
    return BMI_FAILURE;
}

//---------------------------------------------------------------------------------------------------------------------
static int Get_grid_edge_count(Bmi *self, int grid, int *count)
{
    LOG(FATAL,
        "Get_grid_edge_count failed: unable to determine edge count for grid %d", grid);
    return BMI_FAILURE;
}

//---------------------------------------------------------------------------------------------------------------------
static int Get_grid_face_count(Bmi *self, int grid, int *count)
{
    LOG(FATAL,
        "Get_grid_face_count failed: unable to determine face count for grid %d", grid);
    return BMI_FAILURE;
}

//---------------------------------------------------------------------------------------------------------------------
static int Get_grid_edge_nodes(Bmi *self, int grid, int *edge_nodes)
{
    LOG(FATAL,
        "Get_grid_edge_nodes failed: unable to determine edge nodes for grid %d", grid);
    return BMI_FAILURE;
}

//---------------------------------------------------------------------------------------------------------------------
static int Get_grid_face_edges(Bmi *self, int grid, int *face_edges)
{
    LOG(FATAL,
        "Get_grid_face_edges failed: unable to determine face edges for grid %d", grid);
    return BMI_FAILURE;
}

//---------------------------------------------------------------------------------------------------------------------
static int Get_grid_face_nodes(Bmi *self, int grid, int *face_nodes)
{
    LOG(FATAL,
        "Get_grid_face_nodes failed: unable to determine face node for grid %d", grid);
    return BMI_FAILURE;
}

//---------------------------------------------------------------------------------------------------------------------
static int Get_grid_nodes_per_face(Bmi *self, int grid, int *nodes_per_face)
{
    LOG(FATAL,
        "Get_grid_nodes_per_face failed: unable to determine node per face for grid %d", grid);
    return BMI_FAILURE;
}

//---------------------------------------------------------------------------------------------------------------------
Bmi* 
register_bmi_pet(Bmi *model)
{
    if (model) {
        model->data = (void*)new_bmi_pet();
        model->initialize = Initialize;
        model->update = Update;
        model->update_until = Update_until;
        model->finalize = Finalize;

        model->get_component_name = Get_component_name;
        model->get_input_item_count = Get_input_item_count;
        model->get_output_item_count = Get_output_item_count;
        model->get_input_var_names = Get_input_var_names;
        model->get_output_var_names = Get_output_var_names;

        model->get_var_grid = Get_var_grid;
        model->get_var_type = Get_var_type;
        model->get_var_itemsize = Get_var_itemsize;
        model->get_var_units = Get_var_units;
        model->get_var_nbytes = Get_var_nbytes;
        model->get_var_location = Get_var_location;

        model->get_current_time = Get_current_time;
        model->get_start_time = Get_start_time;
        model->get_end_time = Get_end_time;
        model->get_time_units = Get_time_units;
        model->get_time_step = Get_time_step;

        model->get_value = Get_value;
        model->get_value_ptr = Get_value_ptr;   
        model->get_value_at_indices = Get_value_at_indices;

        model->set_value = Set_value;
        model->set_value_at_indices = Set_value_at_indices;

        model->get_grid_size = Get_grid_size;    
        model->get_grid_rank = Get_grid_rank;    
        model->get_grid_type = Get_grid_type;    
        model->get_grid_shape = Get_grid_shape;
        model->get_grid_spacing = Get_grid_spacing;
        model->get_grid_origin = Get_grid_origin;
        model->get_grid_x = Get_grid_x;
        model->get_grid_y = Get_grid_y;
        model->get_grid_z = Get_grid_z;
        model->get_grid_node_count = Get_grid_node_count;
        model->get_grid_edge_count = Get_grid_edge_count;
        model->get_grid_face_count = Get_grid_face_count;
        model->get_grid_edge_nodes = Get_grid_edge_nodes;
        model->get_grid_face_edges = Get_grid_face_edges;
        model->get_grid_face_nodes = Get_grid_face_nodes;
        model->get_grid_nodes_per_face = Get_grid_nodes_per_face;

    }

    return model;
}  // end: register_bmi_et
