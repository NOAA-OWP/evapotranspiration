#ifndef PET_SERIALIZATION_HPP
#define PET_SERIALIZATION_HPP

#ifdef __cplusplus
extern "C" {
#endif

#include "bmi.h"

int free_serialized_pet(Bmi* bmi);
int new_serialized_pet(Bmi* bmi);
int load_serialized_pet(Bmi* bmi, const char* data);

#ifdef __cplusplus
}
#endif

#endif
