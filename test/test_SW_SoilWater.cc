#include "gtest/gtest.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../generic.h"
#include "../filefuncs.h"
#include "../myMemory.h"
#include "../SW_Defines.h"
#include "../SW_Files.h"
#include "../SW_Model.h"
#include "../SW_Site.h"
#include "../SW_SoilWater.h"
#include "../SW_VegProd.h"
#include "../SW_Site.h"
#include "../SW_Flow_lib.h"
#include "sw_testhelpers.h"

extern SW_MODEL SW_Model;
extern SW_SITE SW_Site;
extern SW_VEGPROD SW_VegProd;

namespace{
  // Test the 'SW_SoilWater' function 'SW_VWCBulkRes'
  TEST(SWSoilWaterTest, SWVWCBulkRes){
    //declare mock INPUTS
    RealD fractionGravel = .1;
    RealD clay = .7;
    RealD sand = .2;
    RealD porosity = 1;

    // test clay > .6
    RealD res = SW_VWCBulkRes(fractionGravel, sand, clay, porosity);
    EXPECT_DOUBLE_EQ(res, SW_MISSING);

    // Reset to previous global states
    Reset_SOILWAT2_after_UnitTest();

    clay = .5;
    sand = .04;
    // test sand < .05
    res = SW_VWCBulkRes(fractionGravel, sand, clay, porosity);
    EXPECT_DOUBLE_EQ(res, SW_MISSING);

    // Reset to previous global states
    Reset_SOILWAT2_after_UnitTest();
  }

  // Test the 'SW_SoilWater' function 'SW_SWC_adjust_snow'
  TEST(SWSoilWaterTest, SWSWCSdjustSnow){
    // setup mock variables
    SW_Site.TminAccu2 = 0;
    RealD temp_min = 0;
    RealD temp_max = 10;
    RealD ppt = 1;
    RealD rain = 1.5;
    RealD snow = 1.5;
    RealD snowmelt = 1.2;
    // test 1, since TminAccu2 is < temp_ave, we expect SnowAccu to be 0 and thus rain is ppt - SnowAccu
    SW_SWC_adjust_snow(temp_min, temp_max, ppt, &rain, &snow, &snowmelt);
    EXPECT_EQ(rain, 1);
    EXPECT_EQ(snow, 0);
    Reset_SOILWAT2_after_UnitTest();

  }

  // Test the 'SW_SoilWater' function 'SW_SWCbulk2SWPmatric'
  TEST(SWSoilWaterTest, SWSWCbulk2SWPmatric){
    // TODO, lacking info
    RealD fractionGravel = 0.2;
    RealD swcBulk = 0;
    LyrIndex n = 1;
    // test missing and 0 for swc
    double res = SW_SWCbulk2SWPmatric(fractionGravel, swcBulk, n);
    EXPECT_EQ(res, 0.0);
    Reset_SOILWAT2_after_UnitTest();
    swcBulk = 999;
    res = SW_SWCbulk2SWPmatric(fractionGravel, swcBulk, n);
    EXPECT_EQ(res, 0.0);
    Reset_SOILWAT2_after_UnitTest();

    // test swp val
    swcBulk = 4;
    SW_Site.lyr[n] -> width = 1;
    SW_Site.lyr[n] -> psisMatric = 1;
    SW_Site.lyr[n] -> thetasMatric = 1;
    SW_Site.lyr[n] -> bMatric = 1;
    res = SW_SWCbulk2SWPmatric(fractionGravel, swcBulk, n);
    double resExpect = .00013310902; // did math by hand to get this value
    double actualExpectDiff = fabs(res - resExpect);
    EXPECT_LT(actualExpectDiff, .0002);
  }

  // Test the 'SW_SoilWater' function 'SW_SWPmatric2VWCBulk'
  TEST(SWSoilWaterTest, SWSWPmatric2VWCBulk){
    // set up mock variables
    double fractionGravel = .1;
    double swpMatric = 15.0;
    double p;
    double tExpect;
    double t;
    double actualExpectDiff;
    double psisMatric = 18.608013;
    double binverseMatric = 0.188608;
    double thetaMatric = 41.37;
    double testNumber;
    LyrIndex n = 0;
    SW_Site.lyr[n]->thetasMatric = thetaMatric;
  	SW_Site.lyr[n]->psisMatric = psisMatric;
  	SW_Site.lyr[n]->bMatric = binverseMatric;

    // run tests for gravel fractions on the interval [.05, .8], step .05
    for(testNumber = 1; testNumber <= 16; testNumber++){
      fractionGravel = (testNumber / 20);
      p = powe(psisMatric / (swpMatric * BARCONV), binverseMatric);
      tExpect = thetaMatric * p * 0.01 * (1 - fractionGravel);
      t = SW_SWPmatric2VWCBulk(fractionGravel, swpMatric, n);
      actualExpectDiff = fabs(t - tExpect);

      // Tolerance for error since division with RealD introcuces some error
      EXPECT_LT(actualExpectDiff, 0.0000001);

    }
    Reset_SOILWAT2_after_UnitTest();
  }
}
