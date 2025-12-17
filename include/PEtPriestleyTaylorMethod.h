#ifndef PET_PRIESTLEY_TAYLOR_METHOD_H
#define PET_PRIESTLEY_TAYLOR_METHOD_H

// FUNCTION AND SUBROUTINE PROTOTYPES

double pevapotranspiration_priestley_taylor_method(pet_model *model);

//############################################################*
// subroutine to calculate evapotranspiration using           *
// Chow, Maidment, and Mays textbook                          *
// F.L. Ogden, NOAA National Weather Service, 2020            *
//############################################################*
double pevapotranspiration_priestley_taylor_method(pet_model *model)
{
  // local varibles
  double psychrometric_constant_Pa_per_C;
  double slope_sat_vap_press_curve_Pa_s;
  double water_latent_heat_of_vaporization_J_per_kg;
  double moist_air_specific_humidity_kg_per_m3;
  double liquid_water_density_kg_per_m3;
  double lambda_pet;
  double radiation_balance_pevapotranspiration_rate_m_per_s;
  double instantaneous_pet_rate_m_per_s;
  double mass_flux;
  double delta;
  double gamma;

  calculate_intermediate_variables(model);

  liquid_water_density_kg_per_m3 = model->inter_vars.liquid_water_density_kg_per_m3;
  slope_sat_vap_press_curve_Pa_s=model->inter_vars.slope_sat_vap_press_curve_Pa_s;
  water_latent_heat_of_vaporization_J_per_kg=model->inter_vars.water_latent_heat_of_vaporization_J_per_kg;
  psychrometric_constant_Pa_per_C=model->inter_vars.psychrometric_constant_Pa_per_C;

  delta=slope_sat_vap_press_curve_Pa_s;
  gamma=psychrometric_constant_Pa_per_C;

  if( (model->pet_options.use_aerodynamic_method == FALSE ) && (model->pet_options.use_penman_monteith_method==FALSE) )
  {
    // This is equation 3.5.9 from Chow, Maidment, and Mays textbook.
    lambda_pet=model->pet_forcing.net_radiation_W_per_sq_m;
    radiation_balance_pevapotranspiration_rate_m_per_s=lambda_pet/
                                        (liquid_water_density_kg_per_m3*water_latent_heat_of_vaporization_J_per_kg);
  }
  instantaneous_pet_rate_m_per_s=1.3*delta/(delta+gamma)*radiation_balance_pevapotranspiration_rate_m_per_s;
  return(instantaneous_pet_rate_m_per_s);
}

#endif // PET_PRIESTLEY_TAYLOR_METHOD_H
