#ifndef BMI_ET_H
#define BMI_ET_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "bmi.h"
#include "et.h"

Bmi* register_bmi_et(Bmi *model);

et_model * new_bmi_et();


int read_file_line_counts_et(const char* file_name, 
                                 int* line_count, 
                                 int* max_line_length);
int read_init_config_et(et_model* model, const char* config_file);

#if defined(__cplusplus)
}
#endif

#endif