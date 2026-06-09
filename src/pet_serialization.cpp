extern "C" {
#include "../include/pet.h"
#include "../include/logger.h"
}

#include "../include/pet_serialization.h"
#include "../include/vecbuf.hpp"

#include <sstream>

#include <boost/serialization/serialization.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

class BmiPetSerialization {
    public:
        pet_model* model;
        BmiPetSerialization(Bmi* bmi)
            : model{(pet_model*)bmi->data} {}
        ~BmiPetSerialization() = default;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version);
};


template<class Archive>
void BmiPetSerialization::
serialize(Archive& ar, const unsigned int version) {
    ar & model->pet_m_per_s;
    ar & model->bmi.current_step;
    ar & model->bmi.current_time;
    ar & model->bmi.current_time_step;

    // pet params
    ar & model->pet_params.vegetation_height_m;
    ar & model->pet_params.wind_speed_measurement_height_m;

    // aorc
    ar & model->aorc.incoming_longwave_W_per_m2;
    ar & model->aorc.incoming_shortwave_W_per_m2;
    ar & model->aorc.surface_pressure_Pa;
    ar & model->aorc.specific_humidity_2m_kg_per_kg;
    ar & model->aorc.air_temperature_2m_K;
    ar & model->aorc.u_wind_speed_10m_m_per_s;
    ar & model->aorc.v_wind_speed_10m_m_per_s;
    ar & model->aorc.latitude;
    ar & model->aorc.longitude;

    // pet forcing
    ar & model->pet_forcing.air_temperature_C;
    ar & model->pet_forcing.relative_humidity_percent;
    ar & model->pet_forcing.specific_humidity_2m_kg_per_kg;
    ar & model->pet_forcing.air_pressure_Pa;
    ar & model->pet_forcing.wind_speed_m_per_s;
    ar & model->pet_forcing.net_radiation_W_per_sq_m;
    ar & model->pet_forcing.water_temperature_C;

    // solar radiation
    ar & model->solar_options.cloud_base_height_known;
    ar & model->solar_results.solar_elevation_angle_degrees;
    ar & model->solar_results.solar_azimuth_angle_degrees;
    ar & model->solar_results.solar_local_hour_angle_degrees;
    ar & model->solar_results.solar_radiation_flux_W_per_sq_m;
    ar & model->solar_results.solar_radiation_horizontal_flux_W_per_sq_m;
    ar & model->solar_results.solar_radiation_cloudy_flux_W_per_sq_m;
    ar & model->solar_results.solar_radiation_horizontal_cloudy_flux_W_per_sq_m;

    // intermediate variables
    ar & model->inter_vars.liquid_water_density_kg_per_m3;
    ar & model->inter_vars.water_latent_heat_of_vaporization_J_per_kg;
    ar & model->inter_vars.air_saturation_vapor_pressure_Pa;
    ar & model->inter_vars.air_actual_vapor_pressure_Pa;
    ar & model->inter_vars.vapor_pressure_deficit_Pa;
    ar & model->inter_vars.moist_air_gas_constant_J_per_kg_K;
    ar & model->inter_vars.moist_air_density_kg_per_m3;
    ar & model->inter_vars.slope_sat_vap_press_curve_Pa_s;
    ar & model->inter_vars.psychrometric_constant_Pa_per_C;

    // surface radiation forcing
    ar & model->surf_rad_forcing.incoming_shortwave_radiation_W_per_sq_m;
    ar & model->surf_rad_forcing.incoming_longwave_radiation_W_per_sq_m;
    ar & model->surf_rad_forcing.air_temperature_C;
    ar & model->surf_rad_forcing.relative_humidity_percent;
}


extern "C" {
int free_serialized_pet(Bmi* bmi) {
    if (bmi == nullptr || bmi->data == nullptr) {
        LOG(FATAL, "free_serialized_pet called with NULL bmi or bmi->data");
        return BMI_FAILURE;
    }

    pet_model* model = (pet_model*)bmi->data;

    if (model->serialized != NULL) {
        free(model->serialized);
        model->serialized = NULL;
    }
    model->serialized_length = 0;

    LOG(DEBUG, "Freed PET serialized state");
    return BMI_SUCCESS;
}

int new_serialized_pet(Bmi* bmi) {
    if (bmi == nullptr || bmi->data == nullptr) {
        LOG(FATAL, "new_serialized_pet called with NULL bmi or bmi->data");
        return BMI_FAILURE;
    }

    BmiPetSerialization serializer(bmi);
    vecbuf<char> stream;
    boost::archive::binary_oarchive archive(stream);

    try {
        archive << serializer;
    } catch (const std::exception &e) {
        LOG(FATAL, "Serializing PET failed: %s", e.what());
        return BMI_FAILURE;
    }

    pet_model* model = (pet_model*)bmi->data;

    if (model->serialized != NULL) {
        free(model->serialized);
        model->serialized = NULL;
    }
    // set size and allocate memory
    uint64_t serialized_size = stream.size();
    model->serialized_length = serialized_size + sizeof(uint64_t);
    model->serialized = (char*)malloc(model->serialized_length);

    if (model->serialized == NULL) {
        LOG(FATAL, "Serializing PET failed: memory allocation failed for %zu bytes",
                (size_t)model->serialized_length);
        model->serialized_length = 0;
        return BMI_FAILURE;
    }
    // copy size of stream data and stream data to new allocation
    memcpy(model->serialized, &serialized_size, sizeof(uint64_t));
    memcpy(model->serialized + sizeof(uint64_t), stream.data(), serialized_size);
    return BMI_SUCCESS;
}

int load_serialized_pet(Bmi* bmi, char* data) {
    if (bmi == nullptr || bmi->data == nullptr) {
        LOG(FATAL, "load_serialized_pet called with NULL bmi or bmi->data");
        return BMI_FAILURE;
    }

    if (data == nullptr) {
        LOG(FATAL, "load_serialized_pet called with NULL data");
        return BMI_FAILURE;
    }

    BmiPetSerialization serializer(bmi);
    uint64_t size;
    memcpy(&size, data, sizeof(uint64_t));
    membuf stream(data + sizeof(uint64_t), size);
    boost::archive::binary_iarchive archive(stream);

    try {
        archive >> serializer;
        LOG(DEBUG, "Loaded PET serialized state successfully");
        return BMI_SUCCESS;
    } catch (const std::exception &e) {
        LOG(FATAL, "Deserializing PET failed: %s", e.what());
        return BMI_FAILURE;
    }
}

}

