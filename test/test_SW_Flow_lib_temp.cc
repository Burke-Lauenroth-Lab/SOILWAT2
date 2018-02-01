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

  // Test the veg interception function 'surface_temperature_under_snow'
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

  // Test the veg interception function 'soil_temperature_init'
  TEST(SWFlowTempTest, SoilTemperatureInit) {

    // declare inputs and output
    double surfaceTemp = 2.0, meanAirTemp = 5.0, deltaX = 15.0, theMaxDepth = 990.0;
    unsigned int nlyrs, nRgr = 65;

    // *****  Test when nlyrs = 1  ***** //
    unsigned int i =0.;
    nlyrs = 1;
    double width[] = {20}, oldsTemp[] = {1};
    double bDensity[] = {RandNorm(1.,0.5)}, fc[] = {RandNorm(1.5, 0.5)};
    double wp[1];
    wp[0]= fc[0] - 0.6; // wp will always be less than fc

    /// test standard conditions
    soil_temperature_init(bDensity, width, surfaceTemp, oldsTemp, meanAirTemp, nlyrs, fc, wp, deltaX, theMaxDepth, nRgr);

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
    /* nlyrs = 8;
    double width[] = {5, 5, 5, 10, 10, 10, 20, 20};
    double oldsTemp[] = {1, 1, 1, 2, 2, 2, 3, 3};
    double bDensity[nlyrs], fc[nlyrs], wp[nlyrs]; */
    nlyrs = MAX_LAYERS;
    double width2[] = {5, 5, 5, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 20};
    double oldsTemp2[] = {1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4};
    double bDensity2[nlyrs], fc2[nlyrs], wp2[nlyrs];

    for (i = 0; i < nlyrs; i++) {
      bDensity2[i] = RandNorm(1.,0.5);
      fc2[i] = RandNorm(1.5, 0.5);
      wp2[i] = fc2[i] - 0.6; // wp will always be less than fc
    }

    soil_temperature_init(bDensity2, width2, surfaceTemp, oldsTemp2, meanAirTemp, nlyrs, fc2, wp2, deltaX, theMaxDepth, nRgr);

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


    /// test when theMaxDepth is less than soil layer depth
    theMaxDepth = 70.0;

    EXPECT_DEATH(soil_temperature_init(bDensity2, width2, surfaceTemp, oldsTemp2, meanAirTemp, nlyrs, fc2, wp2, deltaX, theMaxDepth, nRgr),
     "@ generic.c LogError");

    // Reset to previous global state
    Reset_SOILWAT2_after_UnitTest();
  }

}
