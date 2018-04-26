#include "gtest/gtest.h"
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <typeinfo>  // for 'typeid'

#include "../generic.h"
#include "../myMemory.h"
#include "../filefuncs.h"
#include "../rands.h"
#include "../Times.h"
#include "../SW_Defines.h"
#include "../SW_Times.h"
#include "../SW_Files.h"
#include "../SW_Carbon.h"
#include "../SW_Site.h"
#include "../SW_VegProd.h"
#include "../SW_VegEstab.h"
#include "../SW_Model.h"
#include "../SW_SoilWater.h"
#include "../SW_Weather.h"
#include "../SW_Markov.h"
#include "../SW_Sky.h"

#include "../SW_Flow_lib.h"
#include "../SW_Flow_lib.c"


#include "sw_testhelpers.h"

extern SW_MODEL SW_Model;
extern SW_VEGPROD SW_VegProd;

extern ST_RGR_VALUES stValues;

namespace {

  // Test the function 'surface_temperature_under_snow'
  TEST(SWFlowTempTest, SurfaceTemperatureUnderSnow) {

    // declare inputs and output
    double snow, airTempAvg;
    double tSoilAvg;

    // test when snow is 0 and airTempAvg > 0
    snow = 0, airTempAvg = 10;

    tSoilAvg = surface_temperature_under_snow(airTempAvg, snow);

    EXPECT_EQ(0, tSoilAvg); // When there is snow, the return is 0


    // test when snow is > 0 and airTempAvg is >= 0
    snow = 1, airTempAvg = 0;

    tSoilAvg = surface_temperature_under_snow(airTempAvg, snow);

    EXPECT_EQ(-2.0, tSoilAvg); // When there is snow and airTemp >= 0, the return is -2.0

    // test when snow > 0 and airTempAvg < 0
    snow = 1, airTempAvg = -10;

    tSoilAvg = surface_temperature_under_snow(airTempAvg, snow);

    EXPECT_EQ(-4.55, tSoilAvg); // When there snow == 1 airTempAvg = -10

    //
    snow = 6.7, airTempAvg = 0;

    tSoilAvg = surface_temperature_under_snow(airTempAvg, snow);

    EXPECT_EQ(-2.0, tSoilAvg); // When there is snow > 6.665 and airTemp >= 0, the return is -2.0

    // Reset to previous global state
    Reset_SOILWAT2_after_UnitTest();
  }

  // Test the soil temperature initialization function 'soil_temperature_init'
  TEST(SWFlowTempTest, SoilTemperatureInit) {

    // declare inputs and output
    double deltaX = 15.0, theMaxDepth = 990.0, sTconst = 4.15;
    unsigned int nlyrs, nRgr = 65;
    Bool ptr_stError = swFALSE;

    // *****  Test when nlyrs = 1  ***** //
    unsigned int i =0.;
    nlyrs = 1;
    double width[] = {20}, oldsTemp[] = {1};
    double bDensity[] = {RandNorm(1.,0.5)}, fc[] = {RandNorm(1.5, 0.5)};
    double wp[1];
    wp[0]= fc[0] - 0.6; // wp will always be less than fc

    /// test standard conditions
    soil_temperature_init(bDensity, width, oldsTemp, sTconst, nlyrs,
      fc, wp, deltaX, theMaxDepth, nRgr, &ptr_stError);

    //Structure Tests
    EXPECT_EQ(sizeof(stValues.tlyrs_by_slyrs), 21008.);//Is the structure the expected size? - This value is static.

    for(unsigned int i = ceil(stValues.depths[nlyrs - 1]/deltaX); i < nRgr + 1; i++){
        EXPECT_EQ(stValues.tlyrs_by_slyrs[i][nlyrs], -deltaX);
        //Values should be equal to -deltaX when i > the depth of the soil profile/deltaX and j is == nlyrs
    }

    double acc = 0.0;
    for (i = 0; i < nRgr + 1; i++) {
      for (unsigned int j = 0; j < MAX_LAYERS + 1; j++) {
        if (stValues.tlyrs_by_slyrs[i][j] >=0 ) {
          acc += stValues.tlyrs_by_slyrs[i][j];
        }
      }
    }
    EXPECT_EQ(acc, stValues.depths[nlyrs - 1]); //The sum of all positive values should equal the maximum depth

    // Other init test
    EXPECT_EQ(stValues.depths[nlyrs - 1], 20); // sum of inputs width = maximum depth; in my example 295
    EXPECT_EQ((stValues.depthsR[nRgr]/deltaX) - 1, nRgr); // nRgr = (MaxDepth/deltaX) - 1

    // Reset to previous global state
    Reset_SOILWAT2_after_UnitTest();


    // *****  Test when nlyrs = MAX_LAYERS (SW_Defines.h)  ***** //
    /// generate inputs using a for loop
    nlyrs = MAX_LAYERS;
    double width2[] = {5, 5, 5, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 20};
    double oldsTemp2[] = {1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4};
    double bDensity2[nlyrs], fc2[nlyrs], wp2[nlyrs];

    for (i = 0; i < nlyrs; i++) {
      bDensity2[i] = RandNorm(1.,0.5);
      fc2[i] = RandNorm(1.5, 0.5);
      wp2[i] = fc2[i] - 0.6; // wp will always be less than fc
    }

    soil_temperature_init(bDensity2, width2, oldsTemp2, sTconst, nlyrs,
      fc2, wp2, deltaX, theMaxDepth, nRgr, &ptr_stError);

    //Structure Tests
    EXPECT_EQ(sizeof(stValues.tlyrs_by_slyrs), 21008.);//Is the structure the expected size? - This value is static.

    for(unsigned int i = ceil(stValues.depths[nlyrs - 1]/deltaX); i < nRgr + 1; i++){
        EXPECT_EQ(stValues.tlyrs_by_slyrs[i][nlyrs], -deltaX);
        //Values should be equal to -deltaX when i > the depth of the soil profile/deltaX and j is == nlyrs
    }

    acc = 0.0;
    for (unsigned int i = 0; i < nRgr + 1; i++) {
      for (unsigned int j = 0; j < MAX_LAYERS + 1; j++) {
        if (stValues.tlyrs_by_slyrs[i][j] >=0 ) {
          acc += stValues.tlyrs_by_slyrs[i][j];
        }
      }
    }
    EXPECT_EQ(acc, stValues.depths[nlyrs - 1]); //The sum of all positive values should equal the maximum depth

    // Other init test
    EXPECT_EQ(stValues.depths[nlyrs - 1], 295); // sum of inputs width = maximum depth; in my example 295
    EXPECT_EQ((stValues.depthsR[nRgr]/deltaX) - 1, nRgr); // nRgr = (MaxDepth/deltaX) - 1

    // Reset to previous global state
    Reset_SOILWAT2_after_UnitTest();

    /// test when theMaxDepth is less than soil layer depth - function should fail
    double theMaxDepth2 = 70.0;

    EXPECT_DEATH(soil_temperature_init(bDensity2, width2, oldsTemp2, sTconst, nlyrs,
        fc2, wp2, deltaX, theMaxDepth2, nRgr, &ptr_stError),"@ generic.c LogError"); // We expect death when max depth < last layer

    // Reset to previous global state
    Reset_SOILWAT2_after_UnitTest();
  }


  // Test lyrSoil_to_lyrTemp, lyrSoil_to_lyrTemp_temperature via
  // soil_temperature_init function
  TEST(SWFlowTempTest, SoilLayerInterpolationFunctions) {

    // declare inputs and output
    double deltaX = 15.0, theMaxDepth = 990.0, sTconst = 4.15;
    unsigned int nlyrs, nRgr = 65;
    Bool ptr_stError = swFALSE;

    // *****  Test when nlyrs = 1  ***** //
    unsigned int i = 0.;
    nlyrs = 1;
    double width[] = {20}, oldsTemp[] = {1};
    double bDensity[] = {fmaxf(RandNorm(1.5,0.5), 0.1)}, fc[] = {fmaxf(RandNorm(1.5, 0.5), 0.1)};
    double wp[1];
    wp[0]= fmax(fc[0] - 0.6, .1); // wp will always be less than fc

    soil_temperature_init(bDensity, width, oldsTemp, sTconst, nlyrs,
      fc, wp, deltaX, theMaxDepth, nRgr, &ptr_stError);

    // lyrSoil_to_lyrTemp tests: This function is used in soil_temperature_init
    // to transfer the soil layer values of bdensity, fc, and wp, to the "temperature layer"
    // which are contained in bdensityR, fcR, and wpR. Thus we check these values.
    for (i = 0; i < nRgr + 1; i++) {  // all Values should be greater than 0
      EXPECT_GT(stValues.bDensityR[i], 0);
      EXPECT_GT(stValues.fcR[i], 0);
      EXPECT_GT(stValues.wpR[i], 0);
    }

    for (i = ceil(stValues.depths[nlyrs - 1]/deltaX) + 1; i < nRgr + 1; i++) {
      //The TempLayer values that are at depths greater than the max SoilLayer depth should be uniform
      EXPECT_EQ(stValues.bDensityR[i], stValues.bDensityR[i - 1]);
      EXPECT_EQ(stValues.fcR[i], stValues.fcR[i - 1]);
      EXPECT_EQ(stValues.wpR[i], stValues.wpR[i - 1]);
      }

    // lyrSoil_to_lyrTemp_temperature tests
      double maxvalR = 0.;
      for (i = 0; i < nRgr + 1; i++) {
        EXPECT_GT(stValues.oldsTempR[i], -100); //Values interpolated into oldsTempR should be realistic
        EXPECT_LT(stValues.oldsTempR[i], 100); //Values interpolated into oldsTempR should be realistic
        if(GT(stValues.oldsTempR[i], maxvalR)) {
          maxvalR = stValues.oldsTempR[i];
        }
      }
      EXPECT_LE(maxvalR, sTconst);//Maximum interpolated oldsTempR value should be less than or equal to maximum in oldsTemp2 (sTconst = last layer)
      EXPECT_EQ(stValues.oldsTempR[nRgr + 1], sTconst); //Temperature in last interpolated layer should equal sTconst

      // *****  Test when nlyrs = MAX_LAYERS (SW_Defines.h)  ***** //
      /// generate inputs using a for loop
      nlyrs = MAX_LAYERS;
      double width2[] = {5, 5, 5, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 20};
      double oldsTemp2[] = {1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4};
      double bDensity2[nlyrs], fc2[nlyrs], wp2[nlyrs];

      for (i = 0; i < nlyrs; i++) {
        bDensity2[i] = fmaxf(RandNorm(1.,0.5), 0.1);
        fc2[i] = fmaxf(RandNorm(1.5, 0.5), 0.1);
        wp2[i] = fmaxf(fc2[i] - 0.6, 0.1); // wp will always be less than fc
        EXPECT_GT(bDensity2[i], 0);
        EXPECT_GT(fc2[i], 0);
        EXPECT_GT(wp2[i], 0);
      }

      soil_temperature_init(bDensity2, width2, oldsTemp2, sTconst, nlyrs,
        fc2, wp2, deltaX, theMaxDepth, nRgr, &ptr_stError);

      // lyrSoil_to_lyrTemp tests
      for (i = 0; i < nRgr + 1; i++) {  // all Values should be greater than 0
        EXPECT_GT(stValues.bDensityR[i], 0);
        EXPECT_GT(stValues.fcR[i], 0);
        EXPECT_GT(stValues.wpR[i], 0);
      }

      for (i = ceil(stValues.depths[nlyrs - 1]/deltaX) + 1; i < nRgr + 1; i++) {
        //The TempLayer values that are at depths greater than the max SoilLayer depth should be uniform
        EXPECT_EQ(stValues.bDensityR[i], stValues.bDensityR[i - 1]);
        EXPECT_EQ(stValues.fcR[i], stValues.fcR[i - 1]);
        EXPECT_EQ(stValues.wpR[i], stValues.wpR[i - 1]);
        }

      // lyrSoil_to_lyrTemp_temperature tests
        maxvalR = 0.;
        for (i = 0; i < nRgr + 1; i++) {
          EXPECT_GT(stValues.oldsTempR[i], -200); //Values interpolated into oldsTempR should be realistic
          EXPECT_LT(stValues.oldsTempR[i], 200); //Values interpolated into oldsTempR should be realistic
          if(GT(stValues.oldsTempR[i], maxvalR)) {
            maxvalR = stValues.oldsTempR[i];
          }
        }
        EXPECT_LE(maxvalR, sTconst);//Maximum interpolated oldsTempR value should be less than or equal to maximum in oldsTemp2 (sTconst = last layer)
        EXPECT_EQ(stValues.oldsTempR[nRgr + 1], sTconst); //Temperature in last interpolated layer should equal sTconst

        //Reset to global state
        Reset_SOILWAT2_after_UnitTest();

  }

  // Test set layer to frozen or unfrozen 'set_frozen_unfrozen'
  TEST(SWFlowTempTest, SetFrozenUnfrozen){

    // declare inputs and output
    // *****  Test when nlyrs = 1  ***** //
    /// ***** Test that soil freezes ***** ///
    unsigned int nlyrs = 1;
    double sTemp[] = {-5}, swc[] = {1.5}, swc_sat[] = {1.8}, width[] = {5};

    set_frozen_unfrozen(nlyrs, sTemp, swc, swc_sat, width);

    EXPECT_EQ(1,stValues.lyrFrozen[0]); // Soil should freeze when sTemp is <= -1
    // AND swc is > swc_sat - width * .13

    /// ***** Test that soil does not freeze ***** ///
    double sTemp2[] = {0};

    set_frozen_unfrozen(nlyrs, sTemp2, swc, swc_sat, width);

    EXPECT_EQ(0,stValues.lyrFrozen[0]); // Soil should NOT freeze when sTemp is > -1

    // Reset to previous global state
    Reset_SOILWAT2_after_UnitTest();

    // *****  Test when nlyrs = MAX_LAYERS (SW_Defines.h)  ***** //
    nlyrs = MAX_LAYERS;
    double width2[] = {5, 5, 5, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 20};
    double sTemp3[nlyrs], sTemp4[nlyrs],swc2[nlyrs], swc_sat2[nlyrs];

    unsigned int i = 0.;
    for (i = 0; i < nlyrs; i++) {
    sTemp3[i] = -5;
    sTemp4[i] = 0;
    swc2[i] = 5; // set swc to a high value so will be > swc_sat - width * .13
    swc_sat2[i] = 1;
    // run
    set_frozen_unfrozen(nlyrs, sTemp3, swc2, swc_sat2, width2);
    // Test
    EXPECT_EQ(1,stValues.lyrFrozen[i]);
    // run
    set_frozen_unfrozen(nlyrs, sTemp4, swc2, swc_sat2, width2);
    // Test
    EXPECT_EQ(0,stValues.lyrFrozen[i]);

    // Reset to previous global state
    Reset_SOILWAT2_after_UnitTest();
    }

  }

  // Test soil temperature today function 'soil_temperature_today'
  TEST(SWFlowTempTest, SoilTemperatureTodayFunction) {

    // declare inputs and output
    double delta_time = 86400., deltaX = 15.0, T1 = 20.0, sTconst = 4.16, csParam1 = 0.00070,
    csParam2 = 0.000030, shParam = 0.18;
    int nRgr =65;
    Bool ptr_stError = swFALSE;

    // declare input in for loop for non-error causing conditions;
    /// don't use RandNorm for fcR, wpR, vwcR, and bDensityR because will trigger
    /// error causing condtions

    double sTempR[nRgr + 1], oldsTempR[nRgr + 1], wpR[nRgr + 1], fcR[nRgr + 1],
    vwcR[nRgr + 1], bDensityR[nRgr + 1];
    int i = 0.;
    for (i = 0; i < nRgr + 1; i++) {
      sTempR[i] = RandNorm(1.5, 1);
      oldsTempR[i] = RandNorm(1.5, 1);
      fcR[i] = 2.1;
      wpR[i] = 1.5; // wp will always be less than fc
      vwcR[i] = 1.6;
      bDensityR[i] = 1.5;
    }

    soil_temperature_today(&delta_time, deltaX, T1, sTconst, nRgr, sTempR, oldsTempR,
    	vwcR, wpR, fcR, bDensityR, csParam1, csParam2, shParam, &ptr_stError);

    // Check that values that are set, are set right.
    EXPECT_EQ(sTempR[0], T1);
    EXPECT_EQ(sTempR[nRgr + 1], sTconst);

    //Check that ptr_stError is FALSE
    EXPECT_EQ(ptr_stError, 0);

    //Check that sTempR values are realisitic and pass check in code (between -100 and 100)
    for (i = 0; i < nRgr + 1; i++) {
    EXPECT_LT(sTempR[i], 100);
    EXPECT_GT(sTempR[i], -100);
    }

    // test that the ptr_stError is FALSE when it is supposed to
    double sTempR2[nRgr + 1], oldsTempR3[nRgr + 1];

    for (i = 0; i < nRgr + 1; i++) {
    sTempR2[i] = RandNorm(150, 1);
    oldsTempR3[i] = RandNorm(150, 1);
    }

    soil_temperature_today(&delta_time, deltaX, T1, sTconst, nRgr, sTempR2, oldsTempR3,
    vwcR, wpR, fcR, bDensityR, csParam1, csParam2, shParam, &ptr_stError);

    //Check that ptr_stError is TRUE
    EXPECT_EQ(ptr_stError, 1);

    // Reset to previous global state
    Reset_SOILWAT2_after_UnitTest();
  }

  // Test main soil temperature function 'soil_temperature'
  // AND lyrTemp_to_lyrSoil_temperature as this function
  // is only called in the soil_temperature function
  TEST(SWFlowTempTest, MainSoilTemperatureFunction) {

    // *****  Test when nlyrs = 1  ***** //
    unsigned int nlyrs = 1, nRgr = 65;
    double airTemp = 25.0, pet = 5.0, aet = 4.0, biomass = 100., surfaceTemp[] = {20.0, 15. ,14.},
    bmLimiter = 300., t1Param1 = 15., t1Param2 = -4., t1Param3 = 600., csParam1 =0.00070,
    csParam2 = 0.00030, shParam = 0.18, snowdepth = 5, sTconst = 4.15, deltaX = 15,
    theMaxDepth = 990., snow = 1;
    Bool ptr_stError = swFALSE;

    double swc[] = {1.0}, swc_sat[] = {1.5}, bDensity[] = {1.8}, width[] = {20},
    oldsTemp[] = {5.0}, sTemp[] = {4.0}, fc[] = {2.6}, wp[] = {1.0};

    // copy initital oldsTempR values so we can check they have been updated
    double OLDTEMPS[nRgr + 1];
    memcpy(OLDTEMPS, stValues.oldsTempR, sizeof(OLDTEMPS));

    soil_temperature(airTemp, pet, aet, biomass, swc, swc_sat, bDensity, width,
    oldsTemp, sTemp, surfaceTemp, nlyrs, fc, wp, bmLimiter, t1Param1, t1Param2,
    t1Param3, csParam1, csParam2, shParam, snowdepth, sTconst, deltaX, theMaxDepth,
    nRgr, snow, &ptr_stError);

    // Test surface temp equals surface_temperature_under_snow() because snow > 0
    EXPECT_EQ(surfaceTemp[Today], surface_temperature_under_snow(airTemp, snow));
    EXPECT_NE(surfaceTemp[Today], airTemp + ((t1Param2 * (biomass - bmLimiter)) / t1Param3));
    EXPECT_NE(surfaceTemp[Today], airTemp + (t1Param1 * pet * (1. - (aet / pet)) * (1. - (biomass / bmLimiter))));

    //Test surface temp equals equatin when biomass < blimititer & snow = 0
    snowdepth = 0;

    soil_temperature(airTemp, pet, aet, biomass, swc, swc_sat, bDensity, width,
    oldsTemp, sTemp, surfaceTemp, nlyrs, fc, wp, bmLimiter, t1Param1, t1Param2,
    t1Param3, csParam1, csParam2, shParam, snowdepth, sTconst, deltaX, theMaxDepth,
    nRgr, snow, &ptr_stError);

    EXPECT_EQ(surfaceTemp[Today], airTemp + (t1Param1 * pet * (1. - (aet / pet)) * (1. - (biomass / bmLimiter))));
    EXPECT_NE(surfaceTemp[Today], airTemp + ((t1Param2 * (biomass - bmLimiter)) / t1Param3));
    EXPECT_NE(surfaceTemp[Today], surface_temperature_under_snow(airTemp, snow));

    //Test surface temp equals equation when biomass < blimititer & snow = 0
    biomass = 305;

    soil_temperature(airTemp, pet, aet, biomass, swc, swc_sat, bDensity, width,
    oldsTemp, sTemp, surfaceTemp, nlyrs, fc, wp, bmLimiter, t1Param1, t1Param2,
    t1Param3, csParam1, csParam2, shParam, snowdepth, sTconst, deltaX, theMaxDepth,
    nRgr, snow, &ptr_stError);

    EXPECT_EQ(surfaceTemp[Today], airTemp + ((t1Param2 * (biomass - bmLimiter)) / t1Param3));
    EXPECT_NE(surfaceTemp[Today], airTemp + (t1Param1 * pet * (1. - (aet / pet)) * (1. - (biomass / bmLimiter))));
    EXPECT_NE(surfaceTemp[Today], surface_temperature_under_snow(airTemp, snow));

    // checks for  lyrTemp_to_lyrSoil_temperature
    int resultValue = sizeof(sTemp) / sizeof(sTemp[0]);
    EXPECT_EQ(1, resultValue); // when the number of soil layers is 1, sTemp should be 1
    EXPECT_GT(sTemp[0], -100); // Sense check
    EXPECT_LT(sTemp[0], 100); // Sense check
    EXPECT_EQ(0, ptr_stError); // ptr_stError should be 1

    // Test that oldsTempR is updated to sTempR for the next day
    unsigned int k = 0;
    for(k = 0; k < nRgr +1; k++) {
    //  printf("\n k %u sTemp %f , newoldtemp %f,OLDSTEMPS %f", k, sTemp[k], stValues.oldsTempR[k],OLDTEMPS[k] );
    EXPECT_NE(stValues.oldsTempR[k], OLDTEMPS[k]);
    }

    //Reset to global state
    Reset_SOILWAT2_after_UnitTest();
    unsigned int i = 0;

    // *****  Test when nlyrs = MAX_LAYERS  ***** //

    // intialize values
    unsigned int nlyrs2 = MAX_LAYERS;
    double width2[] = {5, 5, 5, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 20};
    double oldsTemp3[] = {1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4};
    double sTemp3[] = {1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4};
    ptr_stError = swFALSE;
    double swc2[nlyrs2], swc_sat2[nlyrs2], bDensity2[nlyrs2], fc2[nlyrs2], wp2[nlyrs2];

    for (i = 0; i < nlyrs2; i++) {
        swc2[i] =  fmaxf(RandNorm(1.,0.5), 0.1);;
        swc_sat2[i] = swc2[i] + 0.5;
        bDensity2[i] = fmaxf(RandNorm(1.,0.5), 0.1);
        fc2[i] = fmaxf(RandNorm(1.5, 0.5), 0.1);
        wp2[i] = fmaxf(fc2[i] - 0.6, 0.1); // wp will always be less than fc
      }

    // copy initital oldsTempR values so we can check they have been updated
    double OLDTEMPS2[nRgr + 1];
    memcpy(OLDTEMPS2, stValues.oldsTempR, sizeof(OLDTEMPS2));

    // Test surface temp equals surface_temperature_under_snow() because snow > 0
    snowdepth = 5;

    soil_temperature(airTemp, pet, aet, biomass, swc2, swc_sat2, bDensity2, width2,
    oldsTemp3, sTemp3, surfaceTemp, nlyrs2, fc2, wp2, bmLimiter, t1Param1, t1Param2,
    t1Param3, csParam1, csParam2, shParam, snowdepth, sTconst, deltaX, theMaxDepth,
     nRgr, snow, &ptr_stError);

    EXPECT_EQ(surfaceTemp[Today], surface_temperature_under_snow(airTemp, snow));
    EXPECT_NE(surfaceTemp[Today], airTemp + ((t1Param2 * (biomass - bmLimiter)) / t1Param3));
    EXPECT_NE(surfaceTemp[Today], airTemp + (t1Param1 * pet * (1. - (aet / pet)) * (1. - (biomass / bmLimiter))));

    //Test surface temp equals equatin when biomass < blimititer & snow = 0
    snowdepth = 0;
    biomass = 100;

    soil_temperature(airTemp, pet, aet, biomass, swc2, swc_sat2, bDensity2, width2,
    oldsTemp3, sTemp3, surfaceTemp, nlyrs2, fc2, wp2, bmLimiter, t1Param1, t1Param2,
    t1Param3, csParam1, csParam2, shParam, snowdepth, sTconst, deltaX, theMaxDepth,
    nRgr, snow, &ptr_stError);

    EXPECT_EQ(surfaceTemp[Today], airTemp + (t1Param1 * pet * (1. - (aet / pet)) * (1. - (biomass / bmLimiter))));
    EXPECT_NE(surfaceTemp[Today], airTemp + ((t1Param2 * (biomass - bmLimiter)) / t1Param3));
    EXPECT_NE(surfaceTemp[Today], surface_temperature_under_snow(airTemp, snow));

    //Test surface temp equals equation when biomass < blimititer & snow = 0
    biomass = 305;

    soil_temperature(airTemp, pet, aet, biomass, swc2, swc_sat2, bDensity2, width2,
    oldsTemp3, sTemp3, surfaceTemp, nlyrs2, fc2, wp2, bmLimiter, t1Param1, t1Param2,
    t1Param3, csParam1, csParam2, shParam, snowdepth, sTconst, deltaX, theMaxDepth,
    nRgr, snow, &ptr_stError);

    EXPECT_EQ(surfaceTemp[Today], airTemp + ((t1Param2 * (biomass - bmLimiter)) / t1Param3));
    EXPECT_NE(surfaceTemp[Today], airTemp + (t1Param1 * pet * (1. - (aet / pet)) * (1. - (biomass / bmLimiter))));
    EXPECT_NE(surfaceTemp[Today], surface_temperature_under_snow(airTemp, snow));

     // checks for  lyrTemp_to_lyrSoil_temperature
     int resultValue2 = sizeof(sTemp3) / sizeof(sTemp3[0]);
     EXPECT_EQ(MAX_LAYERS, resultValue2); // when the number of soil layers is MAX_LAYERS, sTemp should be MAX_LAYERS
     EXPECT_EQ(0, ptr_stError); // ptr_stError should be 0

     for(k = 1; k < nRgr +1; k++) {
       printf("\n k %u sTemp3 %f , newoldtemp %f,OLDSTEMPS2 %f", k, sTemp[k], stValues.oldsTempR[k], OLDTEMPS2[k] );
       EXPECT_GT(sTemp3[k], -100); // Sense check
       EXPECT_LT(sTemp3[k], 100); // Sense check
       // Test that oldsTempR is updated to sTempR for the next day
       EXPECT_NE(stValues.oldsTempR[k], OLDTEMPS2[k]);
     }


     //Reset to global state
     Reset_SOILWAT2_after_UnitTest();

     // Test that function stops or throws an error when it is supposed to.

     // Should fail when soil_temperature_init fails - i.e. when theMaxDepth < depth of nlyrs

     double theMaxDepth2 = 10;

     EXPECT_DEATH(soil_temperature(airTemp, pet, aet, biomass, swc, swc_sat, bDensity, width,
     oldsTemp, sTemp, surfaceTemp, nlyrs, fc, wp, bmLimiter, t1Param1, t1Param2,
     t1Param3, csParam1, csParam2, shParam, snowdepth, sTconst, deltaX, theMaxDepth2,
     nRgr, snow, &ptr_stError), "@ generic.c LogError");

     // ptr_stError should be set to TRUE if soil_temperature_today fails (i.e. unrealistic temp values)
     double sTemp2[nlyrs], oldsTemp2[nlyrs];
     for (i = 0; i < nlyrs; i++) {
     sTemp2[i] = RandNorm(150, 1);
     oldsTemp2[i] = RandNorm(150, 1);
     }

     soil_temperature(airTemp, pet, aet, biomass, swc, swc_sat, bDensity, width,
     oldsTemp2, sTemp2, surfaceTemp, nlyrs, fc, wp, bmLimiter, t1Param1, t1Param2,
     t1Param3, csParam1, csParam2, shParam, snowdepth, sTconst, deltaX, theMaxDepth,
      nRgr, snow, &ptr_stError);

      // Check that ptr_stError is TRUE
      EXPECT_EQ(ptr_stError, 1);

      //Reset to global state
      Reset_SOILWAT2_after_UnitTest();
   }
}