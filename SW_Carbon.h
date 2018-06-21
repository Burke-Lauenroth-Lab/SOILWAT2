/**
 * @file   SW_Carbon.h
 * @author Zachary Kramer, Charles Duso
 * @brief  Defines functions, constants, and variables that deal with the effects of CO2 on transpiration and biomass.
 * @date   23 January 2017
 */
#ifndef CARBON
  #define CARBON             /**< A macro that only lets the variables in @f SW_Carbon.h be defined once. */

  #include "SW_Defines.h"


  /**
   * @brief The main structure holding all CO2-related data.
   */
  typedef struct {
    int
      use_wue_mult,                      /**< A boolean integer indicating if WUE multipliers should be calculated. */
      use_bio_mult;                      /**< A boolean integer indicating if biomass multipliers should be calculated. */

    char
      scenario[64];                      /**< A 64-char array holding the scenario name for which we are extracting CO2 data from the carbon.in file. */

    double
      ppm[MAX_NYEAR];                 /**< A 1D array holding atmospheric CO2 concentration values (units ppm) that are indexed by calendar year. Is typically only populated for the years that are being simulated. `ppm[index]` is the CO2 value for the calendar year `index + 1` */

  } SW_CARBON;

  /* Function Declarations */
  void SW_CBN_construct(void);
  void SW_CBN_deconstruct(void);
  void SW_CBN_read(void);
  void calculate_CO2_multipliers(void);
#endif
