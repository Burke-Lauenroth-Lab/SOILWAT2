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
#include "../pcg/pcg_basic.h"

#include "../SW_Flow_lib.h"

#include "sw_testhelpers.h"

extern SW_SITE SW_Site;
extern SW_MODEL SW_Model;
extern SW_VEGPROD SW_VegProd;
pcg32_random_t flow_rng;
SW_VEGPROD *v = &SW_VegProd;
int k;

namespace {

  // Test the veg interception function 'veg_intercepted_water'
  TEST(SWFlowTest, VegInterceptedWater) {

    ForEachVegType(k){
      // declare inputs
      double x;
      double ppt = 5.0, scale = 1.0;
      double pptleft = 5.0, wintveg = 0.0;
      double a = v -> veg[k].veg_intPPT_a, b = v -> veg[k].veg_intPPT_b,
      c = v -> veg[k].veg_intPPT_c, d = v -> veg[k].veg_intPPT_d;

      // Test expectation when x("vegcov") is zero
      x = 0.0;

      veg_intercepted_water(&pptleft, &wintveg, ppt, x, scale, a, b, c, d);

      EXPECT_EQ(0, wintveg); // When there is no veg, interception should be 0
      EXPECT_EQ(pptleft, ppt); /* When there is no interception, ppt before interception
      should equal ppt left after interception */

      // Test expectations when ppt is 0
      ppt = 0.0, x = 5.0;

      veg_intercepted_water(&pptleft, &wintveg, ppt, x, scale, a, b, c, d);

      EXPECT_EQ(0, wintveg);  // When there is no ppt, interception should be 0
      EXPECT_EQ(pptleft, ppt); /* When there is no interception, ppt before interception
      should equal ppt left after interception */


      // Test expectations when there is both veg cover and precipitation
      ppt = 5.0, x = 5.0;

      veg_intercepted_water(&pptleft, &wintveg, ppt, x, scale, a, b, c, d);

      EXPECT_GT(wintveg, 0); // interception by veg should be greater than 0
      EXPECT_LE(wintveg, MAX_WINTSTCR(x)); // interception by veg should be less than or equal to MAX_WINTSTCR (vegcov * .1)
      EXPECT_LE(wintveg, ppt); // interception by veg should be less than or equal to ppt
      EXPECT_GE(pptleft, 0); // The pptleft (for soil) should be greater than or equal to 0

      // Reset to previous global state
      Reset_SOILWAT2_after_UnitTest();
    }
  }

  // Test the litter interception function 'litter_intercepted_water'
  TEST(SWFlowTest, LitterInterceptedWater) {

    ForEachVegType(k){
      // declare inputs
      double scale, blitter,pptleft = 5.0;
      double wintlit;
      double a = v->veg[k].litt_intPPT_a, b = v->veg[k].litt_intPPT_b,
      c = v->veg[k].litt_intPPT_c, d = v->veg[k].litt_intPPT_d;

      // Test expectation when scale (cover) is zero
      pptleft = 5.0, scale = 0.0, blitter = 5.0;

      litter_intercepted_water(&pptleft, &wintlit, blitter, scale, a, b, c, d);

      EXPECT_EQ(0, wintlit); // When scale is 0, interception should be 0

      // Test expectations when blitter is 0
      pptleft = 5.0, scale = 0.5, blitter = 0.0;

      litter_intercepted_water(&pptleft, &wintlit, blitter, scale, a, b, c, d);

      EXPECT_EQ(0, wintlit); // When there is no blitter, interception should be 0


      // Test expectations when pptleft is 0
      pptleft = 0.0, scale = 0.5, blitter = 5.0;

      litter_intercepted_water(&pptleft, &wintlit, blitter, scale, a, b, c, d);

      EXPECT_EQ(0, pptleft); // When there is no ppt, pptleft should be 0
      EXPECT_EQ(0, wintlit); // When there is no blitter, interception should be 0

      // Test expectations when there pptleft, scale, and blitter are greater than 0
      pptleft = 5.0, scale = 0.5, blitter = 5.0;

      litter_intercepted_water(&pptleft, &wintlit, blitter, scale, a, b, c, d);

      EXPECT_GT(wintlit, 0); // interception by litter should be greater than 0
      EXPECT_LE(wintlit, pptleft); // interception by lit should be less than or equal to ppt
      EXPECT_LE(wintlit, MAX_WINTLIT); /* interception by lit should be less than
      or equal to MAX_WINTLIT (blitter * .2) */
      EXPECT_GE(pptleft, 0); // The pptleft (for soil) should be greater than or equal to 0

      // Reset to previous global state
      Reset_SOILWAT2_after_UnitTest();
    }
  }

  // Test infiltration under high water function, 'infiltrate_water_high'
  TEST(SWFlowTest, infiltrate_water_high){

    // declare inputs
    double pptleft = 5.0, standingWater, drainout;

    // ***** Tests when nlyrs = 1 ***** //
    ///  provide inputs
    int nlyrs = 1;
    double swc[1] = {0.8}, swcfc[1] = {1.1}, swcsat[1] = {1.6}, impermeability[1] = {0.}, drain[1] = {0.};

    infiltrate_water_high(swc, drain, &drainout, pptleft, nlyrs, swcfc, swcsat,
      impermeability, &standingWater);

    EXPECT_GE(drain[0], 0); // drainage should be greater than or equal to 0 when soil layers are 1 and ppt > 1
    EXPECT_LE(swc[0], swcsat[0]); // swc should be less than or equal to swcsat
    EXPECT_DOUBLE_EQ(drainout, drain[0]); // drainout and drain should be equal when we have one layer

    /* Test when pptleft and standingWater are 0 (No drainage) */
    pptleft = 0.0, standingWater = 0.0, drain[0] = 0., swc[0] = 0.8, swcfc[0] = 1.1, swcsat[0] = 1.6;

    infiltrate_water_high(swc, drain, &drainout, pptleft, nlyrs, swcfc, swcsat,
      impermeability, &standingWater);

    EXPECT_DOUBLE_EQ(0, drain[0]); // drainage should be 0


    /* Test when impermeability is greater than 0 and large precipitation */
    pptleft = 20.0;
    impermeability[0] = 1.;
    swc[0] = 0.8, drain[0] = 0.;

    infiltrate_water_high(swc, drain, &drainout, pptleft, nlyrs, swcfc, swcsat,
      impermeability, &standingWater);

    EXPECT_DOUBLE_EQ(0., drain[0]); //When impermeability is 1, drainage should be 0
    EXPECT_DOUBLE_EQ(standingWater, (pptleft + 0.8) - swcsat[0]); /* When impermeability is 1,
      standingWater should be equivalent to  pptLeft + swc[0] - swcsat[0]) */

    // Reset to previous global states
    Reset_SOILWAT2_after_UnitTest();

    // *****  Test when nlyrs = MAX_LAYERS (SW_Defines.h)  ***** //
    /// generate inputs using a for loop
    int i;
    nlyrs = MAX_LAYERS, pptleft = 5.0;
    double swc2[nlyrs], swcfc2[nlyrs], swcsat2[nlyrs], impermeability2[nlyrs], drain2[nlyrs];

    pcg32_random_t infiltrate_rng;
    RandSeed(0,&infiltrate_rng);

    for (i = 0; i < MAX_LAYERS; i++) {
      swc2[i] = RandNorm(1.,0.5,&infiltrate_rng);
      swcfc2[i] = RandNorm(1,.5,&infiltrate_rng);
      swcsat2[i] = swcfc2[i] + .1; // swcsat will always be greater than swcfc in each layer
      impermeability2[i] = 0.0;
    }

    infiltrate_water_high(swc2, drain2, &drainout, pptleft, nlyrs, swcfc2, swcsat2,
      impermeability2, &standingWater);

    EXPECT_EQ(drainout, drain2[MAX_LAYERS - 1]); /* drainout and drain should be
      equal in the last layer */

    for (i = 0; i < MAX_LAYERS; i++) {
      swc2[i] = swc2[i] - 1/10000.; // test below is failing because of small numerical differences.
      EXPECT_LE(swc2[i], swcsat2[i]); // swc should be less than or equal to swcsat
      EXPECT_GE(drain2[i], -1./100000000.); /*  drainage should be greater than or
      equal to 0 or a very small value like 0 */
    }

    /* Test when pptleft and standingWater are 0 (No drainage); swc < swcfc3  < swcsat */
    pptleft = 0.0, standingWater = 0.0;
    double swc3[nlyrs], swcfc3[nlyrs], swcsat3[nlyrs], drain3[nlyrs];

    for (i = 0; i < MAX_LAYERS; i++) {
      swc3[i] = RandNorm(1.,0.5,&infiltrate_rng);
      swcfc3[i] = swc3[i] + .2;
      swcsat3[i] = swcfc3[i] + .5;
      drain3[i] = 0.;// swcsat will always be greater than swcfc in each layer
    }

    infiltrate_water_high(swc3, drain3, &drainout, pptleft, nlyrs, swcfc3, swcsat3,
      impermeability2, &standingWater);

    for (i = 0; i < MAX_LAYERS; i++) {
      EXPECT_DOUBLE_EQ(0, drain3[i]); // drainage should be 0
    }

    /* Test when impermeability is greater than 0 and large precipitation */
    double impermeability4[nlyrs], drain4[nlyrs], swc4[nlyrs], swcfc4[nlyrs], swcsat4[nlyrs];
    pptleft = 20.0;

    for (i = 0; i < MAX_LAYERS; i++) {
      swc4[i] = RandNorm(1.,0.5,&infiltrate_rng);
      swcfc4[i] = swc4[i] + .2;
      swcsat4[i] = swcfc4[i] + .3; // swcsat will always be greater than swcfc in each layer
      impermeability4[i] = 1.0;
      drain4[i] = 0.0;
    }

    swc4[0] = 0.8; // Need to hard code this value because swc4 is altered by function

    infiltrate_water_high(swc4, drain4, &drainout, pptleft, nlyrs, swcfc4, swcsat4,
      impermeability4, &standingWater);

    EXPECT_DOUBLE_EQ(standingWater, (pptleft + 0.8) - swcsat4[0]); /* When impermeability is 1,
      standingWater should be equivalent to  pptLeft + swc[0] - swcsat[0]) */

    for (i = 0; i < MAX_LAYERS; i++) {
      EXPECT_DOUBLE_EQ(0, drain4[i]); //When impermeability is 1, drainage should be 0
    }

    /* Test "push", when swcsat > swc */
    double impermeability5[nlyrs], drain5[nlyrs], swc5[nlyrs], swcfc5[nlyrs], swcsat5[nlyrs];
    pptleft = 5.0;

    for (i = 0; i < MAX_LAYERS; i++) {
      swc5[i] = RandNorm(1.2,0.5,&infiltrate_rng);
      swcfc5[i] = swc5[i] - .4; // set up conditions for excess SWC
      swcsat5[i] = swcfc5[i] + .1; // swcsat will always be greater than swcfc in each layer
      impermeability5[i] = 1.0;
      drain5[i] = 0.0;
    }

    infiltrate_water_high(swc5, drain5, &drainout, pptleft, nlyrs, swcfc5, swcsat5,
      impermeability5, &standingWater);

    for (i = 0; i < MAX_LAYERS; i++) {
      EXPECT_NEAR(swc5[i], swcsat5[i], 0.0000001); // test that swc is now equal to or below swcsat in all layers but the top
    }

    EXPECT_GT(standingWater, 0); // standingWater should be above 0

    // Reset to previous global states
    Reset_SOILWAT2_after_UnitTest();
  }

  //Test svapor function by manipulating variable temp.
  TEST(SWFlowTest, svapor){

    //Declare INPUTS
    double temp[] = {30,35,40,45,50,55,60,65,70,75,20,-35,-12.667,-1,0}; // These are test temperatures, in degrees Celcius.
    double expOut[] = {32.171, 43.007, 56.963, 74.783, 97.353, 125.721, 161.113,
      204.958, 258.912, 324.881, 17.475, 0.243, 1.716, 4.191, 4.509}; // These are the expected outputs for the svapor function.

    //Declare OUTPUTS
    double vapor;

    //Begin TEST
    for (int i = 0; i <15; i++){
      vapor = svapor(temp[i]);

      EXPECT_NEAR(expOut[i], vapor, 0.001); // Testing input array temp[], expected output is expOut[].
    }
    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();
  }

  //Test petfunc by manipulating each input individually.
  TEST(SWFlowTest, petfunc){
    //Begin TEST for avgtemp input variable
    //Declare INPUTS
    unsigned int doy = 2; //For the second day in the month of January
    double rlat = 0.681, elev = 1000, slope = 0, aspect = -1, reflec = 0.15, humid = 61,
      windsp = 1.3, cloudcov = 71, transcoeff = 1;
    double temp, check;
    double avgtemps[] = {30,35,40,45,50,55,60,65,70,75,20,-35,-12.667,-1,0}; // These are test temperatures, in degrees Celcius.

    //Declare INPUTS for expected returns
    double expReturnTemp[] = {0.201, 0.245, 0.299, 0.364, 0.443, 0.539, 0.653, 0.788,
      0.948, 1.137, 0.136, 0.01, 0.032, 0.057, 0.060}; // These are the expected outcomes for the variable arads.

    for (int i = 0; i <15; i++){

      temp = avgtemps[i]; //Uses array of temperatures for testing for input into temp variable.
      check = petfunc(doy, temp, rlat, elev, slope, aspect, reflec, humid, windsp, cloudcov, transcoeff);

      EXPECT_NEAR(check, expReturnTemp[i], 0.001); //Tests the return of the petfunc against the expected return for the petfunc.
    }

    //Begin TEST for rlat input variable.  Inputs outside the range of [-1.169,1.169] produce the same output, 0.042.  Rlat values outside this range represent values near the poles.
    //INPUTS
    temp = 25, cloudcov = 71, humid = 61, windsp = 1.3;
    double rlats[] = {-1.570796, -0.7853982, 0, 0.7853982, 1.570796}; //Initialize array of potential latitudes, in radians, for the site.

   //Declare INPUTS for expected returns
    double expReturnLats[] = {0.042, 0.420, 0.346, 0.134, 0.042}; //These are the expected returns for petfunc while manipulating the rlats input variable.

    for (int i = 0; i < 5; i++){

      rlat = rlats[i]; //Uses array of latitudes initialized above.
      check = petfunc(doy, temp, rlat, elev, slope, aspect, reflec, humid, windsp, cloudcov, transcoeff);

      EXPECT_NEAR(check, expReturnLats[i], 0.001); //Tests the return of the petfunc against the expected return for the petfunc.
    }
    //Begin TEST for elev input variable, testing from -413 meters (Death Valley) to 8727 meters (~Everest).
    //INPUTS
    rlat = 0.681;
    double elevT[] = {-413,0,1000,4418,8727};

    //Declare INPUTS for expected returns
    double expReturnElev[] = {0.181,0.176,0.165,0.130,0.096};

    for(int i = 0; i < 5; i++){

      check = petfunc(doy, temp, rlat, elevT[i], slope, aspect, reflec, humid, windsp, cloudcov, transcoeff);
      //test = round(check* 1000 + 0.00001) / 1000; //Rounding is required for unit test.

      EXPECT_NEAR(check, expReturnElev[i], 0.001); //Tests the return of the petfunc against the expected return for the petfunc.

    }
    //Begin TEST for slope input variable
    //INPUTS
    elev = 1000;
    double slopeT[] = {0,15,34,57,90};

    //Declare INPUTS for expected returns
    double expReturnSlope[] = {0.165, 0.082, 0.01, 0.01, 0.01};
      //Expected returns of 0.01 occur when the petfunc returns a negative number.

    for (int i = 0; i < 5; i++){

      check = petfunc(doy, temp, rlat, elev, slopeT[i], aspect, reflec, humid, windsp, cloudcov, transcoeff);

      EXPECT_NEAR(check, expReturnSlope[i], 0.001); //Tests the return of the petfunc against the expected return for the petfunc.

    }

    //Begin TEST for aspect input variable
    //INPUTS
    slope = 5;
    double aspectT[] = {0, 46, 112, 253, 358};

    //Declare INPUTS for expected returns
    double expReturnAspect[] = {0.138, 0.146, 0.175, 0.172, 0.138};

    for(int i = 0; i < 5; i++){

      check = petfunc(doy, temp, rlat, elev, slope, aspectT[i], reflec, humid, windsp, cloudcov, transcoeff);

      EXPECT_NEAR(check, expReturnAspect[i], 0.001);

    }

    //Begin TEST for reflec input variable
    //INPUTS
    aspect = -1, slope = 0;
    double reflecT[] = {0.1, 0.22, 0.46, 0.55, 0.98};

    //Declare INPUTS for expected returnsdouble expReturnSwpAvg = 0.00000148917;
    double expReturnReflec[] = {0.172, 0.155, 0.120, 0.107, 0.045};

    for(int i = 0; i < 5; i++){

      check = petfunc(doy, temp, rlat, elev, slope, aspect, reflecT[i], humid, windsp, cloudcov, transcoeff);

      EXPECT_NEAR(check, expReturnReflec[i], 0.001);

    }

    //Begin TEST for humidity input varibable.
    //INPUTS
    reflec = 0.15;
    double humidT[] = {2, 34, 56, 79, 89};

    //Declare INPUTS for expected returns.
    double expReturnHumid[] = {0.242, 0.218, 0.176, 0.125, 0.102};

    for(int i = 0; i < 5; i++){

      check = petfunc(doy, temp, rlat, elev, slope, aspect, reflec, humidT[i], windsp, cloudcov, transcoeff);

      EXPECT_NEAR(check, expReturnHumid[i], 0.001);

    }

    //Begin TEST for windsp input variable.
    //INPUTS
    humid = 61, windsp = 0;

    //Declare INPUTS for expected returns.
    double expReturnWindsp[] = {0.112, 0.204, 0.297, 0.390, 0.483, 0.576, 0.669, 0.762, 0.855, 0.948,
      1.041, 1.134, 1.227, 1.320, 1.413, 1.506, 1.599, 1.692, 1.785, 1.878, 1.971, 2.064, 2.157};

    for(int i = 0; i < 23; i++){

      check = petfunc(doy, temp, rlat, elev, slope, aspect, reflec, humid, windsp, cloudcov, transcoeff);

      EXPECT_NEAR(check, expReturnWindsp[i], 0.001);

      windsp += 2.26; //Increments windsp input variable.
    }

    //Begin TEST for cloudcov input variable.
    //INPUTS
    windsp = 1.3;
    double cloudcovT[] = {1, 12, 36, 76, 99};

    //Declare INPUTS for expected returns.
    double expReturnCloudcov[] = {0.148, 0.151, 0.157, 0.166, 0.172};

    for(int i = 0; i < 5; i++){

      check = petfunc(doy, temp, rlat, elev, slope, aspect, reflec, humid, windsp, cloudcovT[i], transcoeff);

      EXPECT_NEAR(check, expReturnCloudcov[i], 0.001);

      cloudcov += 4.27; //Incrments cloudcov input variable.
    }

    //Begin TEST for cloudcov input variable.
    //INPUTS
    cloudcov = 71;
    double transcoeffT[] = {0.01, 0.11, 0.53, 0.67, 0.89};

    //Declare INPUTS for expected returns.
    double expReturnTranscoeff = 0.1650042;
    //The same value is returned for every tested transcoeff input.

    for(int i = 0; i < 5; i++){

      check = petfunc(doy, temp, rlat, elev, slope, aspect, reflec, humid, windsp, cloudcov, transcoeffT[i]);

      EXPECT_NEAR(check, expReturnTranscoeff, 0.001);

    }

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();
    }

  //Test transp_weighted_avg function.
  TEST(SWFlowTest, transp_weighted_avg){

    //INPUTS
    double swp_avg = 10;
    unsigned int n_tr_rgns = 1, n_layers = 1;
    unsigned int tr_regions[1] = {1}; // 1-4
    double tr_coeff[1] = {0.0496}; //trco_grass
    double swc[1] = {12};

    //INPUTS for expected outputs
    double swp_avgExpected1 = 1.100536e-06;
    //Begin TEST when n_layers is one
    transp_weighted_avg(&swp_avg, n_tr_rgns, n_layers, tr_regions, tr_coeff, swc);

    EXPECT_GE(swp_avg, 0); //Must always be non negative.
    EXPECT_NEAR(swp_avgExpected1, swp_avg, 0.000001); //swp_avg is expected to be 1.100536e-06

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Begin TEST when n_layers is at "max"
    //INPUTS
    swp_avg = 10, n_tr_rgns = 4, n_layers = 25;
    unsigned int tr_regions2[25] = {1,1,1,2,2,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4};
    double tr_coeff2[25] = {0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.0496, 0.0495,
        0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.1999}; //trco_tree
    double swc2[25] = {0.01, 0.02, 0.03, 0.04, 1.91, 1.92, 1.93, 3.81, 3.82, 3.83, 5.71, 5.72, 5.73, 7.61,
        7.62, 7.63, 9.51, 9.52, 9.53, 11.41, 11.42, 11.43, 13.31, 13.32, 13.33};

    //INPUTS for expected OUTPUTS
    double swp_avgExpectedM = 0.0009799683;

      //Pointer inputs for _set_layers.
			RealF dmax[25] = {5, 6, 10, 11, 12, 20, 21, 22, 25, 30, 40, 41, 42, 50, 51, 52, 53, 54, 55, 60, 70, 80, 90, 110, 150}; //Can't have multiple layers with same max depth.
			RealF matricd[25] = {1.430, 1.410, 1.390, 1.390, 1.380, 1.150, 1.130, 1.130, 1.430, 1.410, 1.390, 1.390, 1.380, 1.150, 1.130, 1.130,1.430, 1.410,
          1.390, 1.390, 1.380, 1.150, 1.130, 1.130, 1.4};
			RealF f_gravel[25] = {0.1, 0.1, 0.1, 0.1, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2};
      RealF evco[25] = {0.813, 0.153, 0.034, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      RealF trco_grass[25] = {0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997,
          0.1997, 0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.1999};
      RealF trco_shrub[25] = {0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.134,
          0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.1999};
      RealF trco_tree[25] = {0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997,
          0.1997, 0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.1999};
      RealF trco_forb[25] = {0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.134,
          0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.1999};
      RealF psand[25] = {0.51, 0.44, 0.35, 0.32, 0.31, 0.32, 0.57, 0.57, 0.51, 0.44, 0.35, 0.32, 0.31, 0.32, 0.57, 0.57, 0.51, 0.44, 0.35, 0.32, 0.31, 0.32, 0.57, 0.57, 0.58};
			RealF pclay[25] = {0.15, 0.26, 0.41, 0.45, 0.47, 0.47, 0.28, 0.28, 0.15, 0.26, 0.41, 0.45, 0.47, 0.47, 0.28, 0.28, 0.15, 0.26, 0.41, 0.45, 0.47, 0.47, 0.28, 0.28, 0.29};
			RealF imperm[25] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
			RealF soiltemp[25] = {-1, -1, -1, -1, 0, 0, 1, 1, -1, -1, -1, -1, 0, 0, 1, 1, -1, -1, -1, -1, 0, 0, 1, 1, 2};

      _set_layers(n_layers, dmax, matricd, f_gravel,
        evco, trco_grass, trco_shrub, trco_tree,
        trco_forb, psand, pclay, imperm, soiltemp);

		transp_weighted_avg(&swp_avg, n_tr_rgns, n_layers, tr_regions2, tr_coeff2, swc2);

		EXPECT_GE(swp_avg, 0); //Must always be non negative.


		EXPECT_NEAR(swp_avgExpectedM, swp_avg, 0.000001);

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();
  }


  //Test EsT_partitioning by manipulating fbse and fbst variables.
  TEST(SWFlowTest, EsT_partitioning){

    //INPUTS
    double fbse = 0, fbst = 0, blivelai = 0.002, lai_param = 2, tol6 = 0.0000001;

    //TEST when fbse > bsemax
    double fbseExpected = 0.995, fbstExpected = 0.005;
    EsT_partitioning(&fbse, &fbst, blivelai, lai_param);

    EXPECT_NEAR(fbse, fbseExpected, tol6); //fbse is expected to be 0.995
    EXPECT_NEAR(fbst, fbstExpected, tol6); //fbst = 1 - fbse; fbse = bsemax
    EXPECT_GE(fbse, 0); //fbse and fbst must be between zero and one
    EXPECT_GE(fbst, 0); //fbse and fbst must be between zero and one
    EXPECT_LT(fbse, 1); //fbse and fbst must be between zero and one
    EXPECT_LT(fbst, 1); //fbse and fbst must be between zero and one
    EXPECT_DOUBLE_EQ(fbst + fbse, 1); //Must add up to one.

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //TEST when fbse < bsemax
    blivelai = 0.0012, lai_param = 5, fbseExpected = 0.994018, fbstExpected = 0.005982036;
    EsT_partitioning(&fbse, &fbst, blivelai, lai_param);

    EXPECT_NEAR(fbse, fbseExpected, tol6); //fbse is expected to be 0.994018

    EXPECT_NEAR(fbst, fbstExpected, tol6); //fbst is expected to be 0.005982036
    EXPECT_GE(fbse, 0); //fbse and fbst must be between zero and one
    EXPECT_GE(fbst, 0); //fbse and fbst must be between zero and one
    EXPECT_LT(fbse, 1); //fbse and fbst must be between zero and one
    EXPECT_LT(fbst, 1); //fbse and fbst must be between zero and one
    EXPECT_DOUBLE_EQ(fbst + fbse, 1);  //Must add up to one.
    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();
  }

  //TEST pot_soil_evap for when nelyrs = 1 and nelyrs = MAX
  //Begin TEST with nelrs = 1
  TEST(SWFlowTest, pot_soil_evap){
    //INPUTS
    unsigned int nelyrs = 1;
    double ecoeff[1] = {0.01};
    double bserate = 0, totagb, fbse = 0.813, petday = 0.1, shift = 45, shape = 0.1,
      inflec = 0.25, range = 0.8, Es_param_limit = 1;
    double width[1] = {5};
    double swc[1] = {1};

    //INPUTS for expected outputs
    double bserateExpected = 0;

    //Begin TEST for if(totagb >= Es_param_limit)
    totagb = 17000;
    pot_soil_evap(&bserate, nelyrs, ecoeff, totagb, fbse, petday, shift, shape, inflec, range,
      width, swc, Es_param_limit);

    EXPECT_DOUBLE_EQ(bserate, bserateExpected); //Expected return of zero when totagb >= Es_param_limit

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Begin TEST for if(totagb < Es_param_limit)
    totagb = 0.5, bserateExpected = 0;
    bserateExpected = 0.02563937;

    pot_soil_evap(&bserate, nelyrs, ecoeff, totagb, fbse, petday, shift, shape, inflec, range,
      width, swc, Es_param_limit);

    EXPECT_NEAR(bserate, bserateExpected, 0.0001); //bserate is expected to be 0.02561575

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Begin TEST for when nelyrs = 25
    nelyrs = 25;
    double ecoeff8[25] = {0.01, 0.1, 0.25, 0.5, 0.01, 0.1, 0.25, 0.5, 0.01, 0.1, 0.25, 0.5,0.01, 0.1, 0.25, 0.5,
        0.01, 0.1, 0.25, 0.5,0.01, 0.1, 0.25, 0.5,0.51};
    double swc8[25] = {.01, 0.02, 0.03, 0.04, 1.91, 1.92, 1.93, 3.81, 3.82, 3.83, 5.71, 5.72, 5.73, 7.61, 7.62,
        7.63, 9.51, 9.52, 9.53, 11.41, 11.42, 11.43, 13.31, 13.32, 13.33};
    double width8[25] = {5,1,4,1,1,8,1,1,3,5,10,1,1,8,1,1,1,1,1,5,10,10,10,20,40};//Based on dmax in _set_layers


    //Begin TEST for if(totagb >= Es_param_limit)
    totagb = 17000;

    RealF dmax[25] = {5, 15, 25, 35, 55, 75, 95, 115, 135, 155, 175, 195, 215, 235, 255, 275, 295, 315, 335, 355, 375, 395, 415, 435, 455};
    RealF matricd[25] = {1.430, 1.410, 1.390, 1.390, 1.380, 1.150, 1.130, 1.130, 1.430, 1.410, 1.390, 1.390, 1.380, 1.150, 1.130, 1.130,1.430,
        1.410, 1.390, 1.390, 1.380, 1.150, 1.130, 1.130, 1.4};
    RealF f_gravel[25] = {0.1, 0.1, 0.1, 0.1, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2};
    RealF evco[25] = {0.813, 0.153, 0.034, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    RealF trco_grass[25] = {0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997,
        0.1997, 0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.1999};
    RealF trco_shrub[25] = {0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.134,
        0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.1999};
    RealF trco_tree[25] = {0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997,
        0.1997, 0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.1999};
    RealF trco_forb[25] = {0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.134,
        0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.1999};
    RealF psand[25] = {0.51, 0.44, 0.35, 0.32, 0.31, 0.32, 0.57, 0.57, 0.51, 0.44, 0.35, 0.32, 0.31, 0.32, 0.57, 0.57, 0.51, 0.44, 0.35, 0.32, 0.31, 0.32, 0.57, 0.57, 0.58};
    RealF pclay[25] = {0.15, 0.26, 0.41, 0.45, 0.47, 0.47, 0.28, 0.28, 0.15, 0.26, 0.41, 0.45, 0.47, 0.47, 0.28, 0.28, 0.15, 0.26, 0.41, 0.45, 0.47, 0.47, 0.28, 0.28, 0.29};
    RealF imperm[25] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    RealF soiltemp[25] = {-1, -1, -1, -1, 0, 0, 1, 1, -1, -1, -1, -1, 0, 0, 1, 1, -1, -1, -1, -1, 0, 0, 1, 1, 2};

    _set_layers(nelyrs, dmax, matricd, f_gravel,
      evco, trco_grass, trco_shrub, trco_tree,
      trco_forb, psand, pclay, imperm, soiltemp);

    pot_soil_evap(&bserate, nelyrs, ecoeff8, totagb, fbse, petday, shift, shape, inflec, range,
      width8, swc8, Es_param_limit);

    EXPECT_DOUBLE_EQ(bserate, 0); //Expected return of zero when totagb >= Es_param_limit

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();
    //Begin TEST for if(totagb < Es_param_limit)
    SW_VEGPROD *v = &SW_VegProd; //The float Es_param_limit is being affected by the veg pointer.
    totagb = 0.5, bserateExpected = 0.02563948;
//      printf("1totagb: %f Es_param_limit: %f\n", totagb, Es_param_limit);
	      _set_layers(nelyrs, dmax, matricd, f_gravel,
        evco, trco_grass, trco_shrub, trco_tree,
        trco_forb, psand, pclay, imperm, soiltemp);

    pot_soil_evap(&bserate, nelyrs, ecoeff8, totagb, fbse, petday, shift, shape, inflec, range,
      width8, swc8, Es_param_limit);
    //  printf("Es: %f\n", Es_param_limit);
    //  printf("togab: %f\n", totagb);
//      for(unsigned int k=0; k< nelyrs; k++){
//        printf("k: %x\n", k);
//      EXPECT_EQ(Es_param_limit, v->veg[k].Es_param_limit);  //Pointer is 0 when it should be 1
//    }
//      printf("2totagb: %f Es_param_limit: %f\n", totagb, Es_param_limit);
      if(v->veg[k].Es_param_limit > 600){
        EXPECT_GT(bserate, 0);
    EXPECT_NEAR(bserate, bserateExpected, 0.0001);  //bserate is expected to be 0.02561575
      }
    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();
  }



  //TEST pot_soil_evap_bs for when nelyrs = 1 and nelyrs = MAX
  TEST(SWFlowTest, pot_soil_evap_bs){
    //INPUTS
    unsigned int nelyrs = 1;
    double ecoeff[1] = {0.1};
    double bserate = 0, petday = 0.1, shift = 45, shape = 0.1, inflec = 0.25, range = 0.8;
    double width[1] = {5};
    double swc[1] = {1};

    //INPUTS for expected outputs
    double bserateExpected = 0.06306041;

    //Begin TEST for bserate when nelyrs = 1
    pot_soil_evap_bs(&bserate, nelyrs, ecoeff, petday, shift, shape, inflec, range, width, swc);

    EXPECT_NEAR(bserate, bserateExpected, 0.0001);  //bserate is expected to be 0.06306041

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Testing when nelyrs = MAX_LAYERS
    //INPUTS
    nelyrs = 25; //Only 8 inputs available for SW_SWCbulk2SWPmatric function of SW_SoilWater.c
    double ecoeff8[25] = {0.1, 0.1, 0.25, 0.5, 0.01, 0.1, 0.25, 0.5, 0.1, 0.1, 0.25, 0.5, 0.01, 0.1, 0.25, 0.5, 0.1, 0.1, 0.25, 0.5, 0.01, 0.1, 0.25, 0.5, 0.4};
    double width8[25] = {5,10,10,10,20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20};
    double swc8[25] = {0.01, 0.02, 0.03, 0.04, 1.91, 1.92, 1.93, 3.81, 3.82, 3.83, 5.71, 5.72, 5.73, 7.61, 7.62, 7.63, 9.51, 9.52, 9.53, 11.41, 11.42, 11.43, 13.31, 13.32, 13.33};

    RealF dmax[25] = {5, 15, 25, 35, 55, 75, 95, 115, 135, 155, 175, 195, 215, 235, 255, 275, 295, 315, 335, 355, 375, 395, 415, 435, 455};
    RealF matricd[25] = {1.430, 1.410, 1.390, 1.390, 1.380, 1.150, 1.130, 1.130, 1.430, 1.410, 1.390, 1.390, 1.380, 1.150, 1.130, 1.130,1.430, 1.410, 1.390,
        1.390, 1.380, 1.150, 1.130, 1.130, 1.4};
    RealF f_gravel[25] = {0.1, 0.1, 0.1, 0.1, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2};
    RealF evco[25] = {0.813, 0.153, 0.034, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    RealF trco_grass[25] = {0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997,
        0.1997, 0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.1999};
    RealF trco_shrub[25] = {0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.134,
        0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.1999};
    RealF trco_tree[25] = {0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997,
        0.1997, 0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.1999};
    RealF trco_forb[25] = {0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.134,
        0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.1999};
    RealF psand[25] = {0.51, 0.44, 0.35, 0.32, 0.31, 0.32, 0.57, 0.57, 0.51, 0.44, 0.35, 0.32, 0.31, 0.32, 0.57, 0.57, 0.51, 0.44, 0.35, 0.32, 0.31, 0.32, 0.57, 0.57, 0.58};
    RealF pclay[25] = {0.15, 0.26, 0.41, 0.45, 0.47, 0.47, 0.28, 0.28, 0.15, 0.26, 0.41, 0.45, 0.47, 0.47, 0.28, 0.28, 0.15, 0.26, 0.41, 0.45, 0.47, 0.47, 0.28, 0.28, 0.29};
    RealF imperm[25] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    RealF soiltemp[25] = {-1, -1, -1, -1, 0, 0, 1, 1, -1, -1, -1, -1, 0, 0, 1, 1, -1, -1, -1, -1, 0, 0, 1, 1, 2};

    _set_layers(nelyrs, dmax, matricd, f_gravel,
      evco, trco_grass, trco_shrub, trco_tree,
      trco_forb, psand, pclay, imperm, soiltemp);

    //INPUTS for expected outputs
    bserateExpected = 0;

    pot_soil_evap_bs(&bserate, nelyrs, ecoeff8, petday, shift, shape, inflec, range, width8, swc8);

    EXPECT_NEAR(bserate, bserateExpected, 0.0001); //bserate is expected to be 0

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();
  }


  //Test pot_transp by manipulating biolive and biodead input variables
  TEST(SWFlowTest, pot_transp){

    //INPUTS
    double bstrate = 0, swpavg = 0.8, biolive = -0.8, biodead = 0.2, fbst = 0.8, petday = 0.1,
      swp_shift = 45, swp_shape = 0.1, swp_inflec = 0.25, swp_range = 0.3, shade_scale = 1.1,
        shade_deadmax = 0.9, shade_xinflex = 0.4, shade_slope = 0.9, shade_yinflex = 0.3,
          shade_range = 0.8, co2_wue_multiplier = 2.1;

    //Begin TEST for if biolive < 0
    pot_transp(&bstrate, swpavg, biolive, biodead, fbst, petday, swp_shift, swp_shape,
      swp_inflec, swp_range, shade_scale, shade_deadmax, shade_xinflex, shade_slope,
        shade_yinflex, shade_range, co2_wue_multiplier);

    //INPUTS for expected outputs
    double bstrateExpected = 0.06596299;

    EXPECT_DOUBLE_EQ(bstrate, 0); //bstrate = 0 if biolove < 0

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Begin TEST for if biolive > 0
    biolive = 0.8;
    pot_transp(&bstrate, swpavg, biolive, biodead, fbst, petday, swp_shift, swp_shape,
      swp_inflec, swp_range, shade_scale, shade_deadmax, shade_xinflex, shade_slope,
        shade_yinflex, shade_range, co2_wue_multiplier);

    EXPECT_NEAR(bstrate, bstrateExpected, 0.0000001); //For this test local variable shadeaf = 1, affecting bstrate
                                   //bstrate is expected to be 0.06596299

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Begin TEST for if biodead > shade_deadmax
    biodead = 0.95, bstrateExpected = 0.06564905;
    pot_transp(&bstrate, swpavg, biolive, biodead, fbst, petday, swp_shift, swp_shape,
      swp_inflec, swp_range, shade_scale, shade_deadmax, shade_xinflex, shade_slope,
        shade_yinflex, shade_range, co2_wue_multiplier);

    //check = round(bstrate * 1000) / 1000;  //Rounding is required for unit test.
    EXPECT_NEAR(bstrate, bstrateExpected, 0.001); //bstrate is expected to be 0.06564905

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Begin TEST for if biodead < shade_deadmax
    biodead = 0.2, bstrateExpected = 0.06596299;
    pot_transp(&bstrate, swpavg, biolive, biodead, fbst, petday, swp_shift, swp_shape,
      swp_inflec, swp_range, shade_scale, shade_deadmax, shade_xinflex, shade_slope,
        shade_yinflex, shade_range, co2_wue_multiplier);

    EXPECT_NEAR(bstrate, bstrateExpected, 0.0000001); //For this test local variable shadeaf = 1, affecting bstrate
                                   //bstrate is expected to be 0.06596299

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();
  }

  //Test result for watrate by manipulating variable petday
  TEST(SWFlowTest, watrate){

    //INPUTS
    double swp = 0.8, petday = 0.1, shift = 45, shape = 0.1, inflec = 0.25, range = 0.8;

    //Begin TEST for if petday < .2
    double watExpected = 0.630365;
    double wat = watrate(swp, petday, shift, shape, inflec, range);

    EXPECT_NEAR(wat, watExpected, 0.0000001); //When petday = 0.1, watrate = 0.630365
    EXPECT_LE(wat, 1); //watrate must be between 0 & 1
    EXPECT_GE(wat, 0); //watrate must be between 0 & 1

    //Begin TEST for if 0.2 < petday < .4
    petday = 0.3, watExpected = 0.6298786;
    wat = watrate(swp, petday, shift, shape, inflec, range);

    EXPECT_NEAR(wat, watExpected, 0.0000001); //When petday = 0.3, watrate = 0.6298786
    EXPECT_LE(wat, 1); //watrate must be between 0 & 1
    EXPECT_GE(wat, 0); //watrate must be between 0 & 1

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Begin TEST for if 0.4 < petday < .6
    petday = 0.5, watExpected = 0.6285504;
    wat = watrate(swp, petday, shift, shape, inflec, range);

    EXPECT_NEAR(wat, watExpected, 0.0000001); //When petrate = 0.5, watrate = 0.6285504
    EXPECT_LE(wat, 1); //watrate must be between 0 & 1
    EXPECT_GE(wat, 0); //watrate must be between 0 & 1

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Begin TEST for if 0.6 < petday < 1
    petday = 0.8, watExpected = 0.627666;
    wat = watrate(swp, petday, shift, shape, inflec, range);

    EXPECT_NEAR(wat, watExpected, 0.0000001); //When petday = 0.8, watrate = 0.627666
    EXPECT_LE(wat, 1); //watrate must be between 0 & 1
    EXPECT_GE(wat, 0); //watrate must be between 0 & 1

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();
  }

  //Test evap_fromSurface by manipulating water_pool and evap_rate variables
  TEST(SWFlowTest, evap_fromSurface){

    //INPUTS
    double water_pool = 1, evap_rate = 0.33, aet = 0.53;

    //Begin TEST for when water_pool > evap_rate
    double aetExpected = 0.86, evapExpected = 0.33, waterExpected = 0.67;
    evap_fromSurface(&water_pool, &evap_rate, &aet);

    EXPECT_NEAR(aet, aetExpected, 0.0000001); //Variable aet is expected to be 0.86 with current inputs
    EXPECT_GE(aet, 0); //aet is never negative

    EXPECT_NEAR(evap_rate, evapExpected, 0.0000001); //Variable evap_rate is expected to be 0.33 with current inputs
    EXPECT_GE(evap_rate, 0); //evap_rate is never negative

    EXPECT_NEAR(water_pool, waterExpected, 0.0000001); //Variable water_pool is expected to be 0.67 with current inputs
    EXPECT_GE(water_pool, 0); //water_pool is never negative

    //Begin TEST for when water_pool < evap_rate
    water_pool = 0.1, evap_rate = 0.67, aet = 0.78, aetExpected = 0.88, evapExpected = 0.1;
    evap_fromSurface(&water_pool, &evap_rate, &aet);

    EXPECT_NEAR(aet, aetExpected, 0.0000001); //Variable aet is expected to be 0.88 with current inputs
    EXPECT_GE(aet, 0); //aet is never negative

    EXPECT_NEAR(evap_rate, evapExpected, 0.0000001); //Variable evap_rate is expected to be 0.1 with current inputs
    EXPECT_GE(evap_rate, 0); //evap_rate is never negative

    EXPECT_DOUBLE_EQ(water_pool, 0); //Variable water_pool is expected to be 0 when water_pool < evap_rate
    EXPECT_GE(water_pool, 0); //water_pool is never negative

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();
  }

  //Test remove_from_soil when nlyrs = 1 and when nlyrs = MAX
  TEST(SWFlowTest, remove_from_soil){

    //INPUTS
    double swc[25] =  {0.11, 0.12, 0.13, 0.14, 1.91, 1.92, 1.93, 3.81, 3.82, 3.83, 5.71, 5.72, 5.73, 7.61, 7.62,
        7.63, 9.51, 9.52, 9.53, 11.41, 11.42, 11.43, 13.31, 13.32, 13.33};
    double qty[25] = {0.01, 0.02, 0.03, 0.04, 0.05, 1.51, 3.51, 5.51, 7.51, 9.51, 11.51, 13.51, 0.01, 0.02, 0.03,
        0.04, 0.05, 1.51, 3.51, 5.51, 7.51, 9.51, 11.51, 13.51, 15};
    double aet = 0.33, rate = 0.62;
    unsigned int nlyrs = 25;
    double coeff[25] = {0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.0496, 0.0495, 0.1006,
        0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.1999};
    double coeffZero[25] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    double swcmin[25] = {0.01, 0.02, 0.03, 0.04, 1.71, 1.72, 1.73, 3.61, 3.62, 3.63, 5.51, 5.52, 5.53, 7.41, 7.42,
        7.43, 9.31, 9.32, 9.33, 11.11, 11.12, 11.13, 13.01, 13.02, 13.03};

    //Begin TEST when nlyrs = MAX LAYERS
    //TEST for if local variable sumswp = 0 (coeff[i] = 0)

    //INPUTS for expected outputs
    double swcExpected[25] =  {0.11, 0.12, 0.13, 0.14, 1.91, 1.92, 1.93, 3.81, 3.82, 3.83, 5.71, 5.72, 5.73,
        7.61, 7.62, 7.63, 9.51, 9.52, 9.53, 11.41, 11.42, 11.43, 13.31, 13.32, 13.33};
    double qtyExpected[25] = {0.01, 0.02, 0.03, 0.04, 0.05, 1.51, 3.51, 5.51, 7.51, 9.51, 11.51, 13.51, 0.01,
        0.02, 0.03, 0.04, 0.05, 1.51, 3.51, 5.51, 7.51, 9.51, 11.51, 13.51, 15};
    double aetExpected = 0.33;

    RealF dmax[25] = {5, 15, 25, 35, 55, 75, 95, 115, 135, 155, 175, 195, 215, 235, 255, 275, 295, 315, 335, 355, 375, 395, 415, 435, 455};
    RealF matricd[25] = {1.430, 1.410, 1.390, 1.390, 1.380, 1.150, 1.130, 1.130, 1.430, 1.410, 1.390, 1.390, 1.380,
        1.150, 1.130, 1.130,1.430, 1.410, 1.390, 1.390, 1.380, 1.150, 1.130, 1.130, 1.4};
    RealF f_gravel[25] = {0.1, 0.1, 0.1, 0.1, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2,
        0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2};
    RealF evco[25] = {0.813, 0.153, 0.034, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    RealF trco_grass[25] = {0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.0496, 0.0495, 0.1006,
        0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.1999};
    RealF trco_shrub[25] = {0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.134, 0.094, 0.176, 0.175, 0.110,
        0.109, 0.101, 0.101, 0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.1999};
    RealF trco_tree[25] = {0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.0496, 0.0495, 0.1006,
        0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.1999};
    RealF trco_forb[25] = {0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.134, 0.094, 0.176, 0.175, 0.110,
        0.109, 0.101, 0.101, 0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.1999};
    RealF psand[25] = {0.51, 0.44, 0.35, 0.32, 0.31, 0.32, 0.57, 0.57, 0.51, 0.44, 0.35, 0.32, 0.31, 0.32, 0.57,
        0.57, 0.51, 0.44, 0.35, 0.32, 0.31, 0.32, 0.57, 0.57, 0.58};
    RealF pclay[25] = {0.15, 0.26, 0.41, 0.45, 0.47, 0.47, 0.28, 0.28, 0.15, 0.26, 0.41, 0.45, 0.47, 0.47,
        0.28, 0.28, 0.15, 0.26, 0.41, 0.45, 0.47, 0.47, 0.28, 0.28, 0.29};
    RealF imperm[25] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    RealF soiltemp[25] = {-1, -1, -1, -1, 0, 0, 1, 1, -1, -1, -1, -1, 0, 0, 1, 1, -1, -1, -1, -1, 0, 0, 1, 1, 2};

    _set_layers(nlyrs, dmax, matricd, f_gravel,
      evco, trco_grass, trco_shrub, trco_tree,
      trco_forb, psand, pclay, imperm, soiltemp);

    remove_from_soil(swc, qty, &aet, nlyrs, coeffZero, rate, swcmin); //coeffZero used instead of coeff

    for(unsigned int i = 0; i < nlyrs; i++){
      EXPECT_DOUBLE_EQ(qty[i], qtyExpected[i]); //no change expected from original values
      EXPECT_DOUBLE_EQ(swc[i], swcExpected[i]); //no change expected from original values
      EXPECT_DOUBLE_EQ(aet, aetExpected); //no change expected from original values
    }

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    _set_layers(nlyrs, dmax, matricd, f_gravel,
      evco, trco_grass, trco_shrub, trco_tree,
      trco_forb, psand, pclay, imperm, soiltemp);

    //Begin TEST for if st->lyrFrozen[i]
    remove_from_soil(swc, qty, &aet, nlyrs, coeff, rate, swcmin); //Switch to coeff input

    //Begin TEST for if st->lyrFrozen[i] = false.
    double swcExpectedF[25] =  {0.110000, 0.120000, 0.130000, 0.140000, 1.910000, 1.920000,
      1.930000, 3.809984, 3.819993, 3.829998, 5.709966, 5.719973, 5.729975, 7.608994, 7.617507,
        7.627483, 9.509152, 9.518744, 9.525787, 11.382081, 11.387989, 11.361346, 13.161745, 13.170927, 13.148355};
    double qtyExpectedF[25] = {3.807585e-11, 2.946578e-15, 2.451138e-18, 5.229616e-19, 2.749129e-10,
      6.201412e-10, 1.066871e-7, 1.554821e-5, 6.735567e-6, 2.046469e-6, 3.360481e-5, 2.668769e-5,
        2.480712e-5, 1.005520e-3, 2.493016e-3, 2.517080e-3, 8.483740e-4, 1.255580e-3, 4.213218e-3,
          2.791898e-2, 3.201125e-02, 6.865433e-02, 1.482549e-01, 1.490728e-01, 1.816455e-01};
    aetExpected = 0.95;

    for(unsigned int i = 0; i < 8; i++){

      EXPECT_NEAR(qty[i], qtyExpectedF[i], 0.000001); //Values for qty[] are tested against array of expected Values
      EXPECT_NEAR(swc[i], swcExpectedF[i], 0.000001); //Values for swc[] are tested against array of expected values

    }
    EXPECT_DOUBLE_EQ(aet, aetExpected); //aet is expected to be 0.95

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Test when nlyrs = 1
    nlyrs = 1;
    double swc1[1] =  {0.11};
    double qty1[1] = {0.05};
    double coeff1[1] = {0.033};
    double swcmin1[1] = {0.01};
    aet = 0.33, aetExpected = 0.43;
    //Begin TEST for if local variable sumswp = 0 (coeff[i] = 0)
    double qtyExpected1[1] = {0.1};
    double swcExpected1[1] = {0.01};

    _set_layers(nlyrs, dmax, matricd, f_gravel,
      evco, trco_grass, trco_shrub, trco_tree,
      trco_forb, psand, pclay, imperm, soiltemp);

    remove_from_soil(swc1, qty1, &aet, nlyrs, coeff1, rate, swcmin1); //swcmin1 input
    for(unsigned int i = 0; i < nlyrs; i++){

      EXPECT_DOUBLE_EQ(qty1[i], qtyExpected1[i]);
      EXPECT_DOUBLE_EQ(swc1[i], swcExpected1[i]);
      EXPECT_DOUBLE_EQ(aet, aetExpected);
    }

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();
  }

  //Test when nlyrs = 1 and 25 for outputs; swc, drain, drainout, standing water
  TEST(SWFlowTest, infiltrate_water_low){

    //INPUTS
    double swcmin[25] = {0.02, 0.03, 0.04, 0.05, 1.92, 1.93, 1.94, 3.82, 3.83, 3.84, 5.72,
        5.73, 5.74, 7.62, 7.63, 7.64, 9.52, 9.53, 9.54, 11.42, 11.43, 11.44, 13.32, 13.33, 13.34};
    double drain[25] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
    double drainout = 0.1, sdrainpar = 0.6, sdraindpth = 6, standingWater = 0;
    unsigned int nlyrs = MAX_LAYERS;
    double swcfc[25] = {0.33, 0.36, 0.39, 0.41, 0.43, 0.46, 0.52, 0.63, 0.69, 0.72, 0.78,
        0.97, 1.02, 1.44, 1.78, 2.01, 2.50, 3.33, 4.04, 5.01, 5.02, 5.03, 5.05, 6.66, 7};
    double width[25] = {5,10,10,10,20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
        20, 20, 20, 20, 20, 20, 20, 20};
    double swc[25] = {0.01, 0.02, 0.03, 0.04, 1.91, 1.92, 1.93, 3.81, 3.82, 3.83, 5.71,
        5.72, 5.73, 7.61, 7.62, 7.63, 9.51, 9.52, 9.53, 11.41, 11.42, 11.43, 13.31, 13.32, 13.33};
    double swcsat[25] = {0, 2.5, 3.6, 4.8, 5.3, 6.2, 7.9, 8.2, 8.3, 9.99, 10.77, 13.61,
        5.01, 6.01, 6.2, 6.4, 7.03, 8.21, 9.31, 10.10, 12.01, 13.01, 14.01, 14.02, 15};
    double impermeability[25] = {0.025, 0.030, 0.035, 0.040, 0.060, 0.075, 0.080, 0.115, 0.125,
        0.145, 0.150, 0.155, 0.180, 0.190, 0.210, 0.215, 0.250, 0.255, 0.275, 0.375, 0.495, 0.500, 0.500, 0.750, 1.000};

    ST_RGR_VALUES stValues;
	  ST_RGR_VALUES *st = &stValues;

    //INPUTS for expected outputs
    double swcExpected[25] = {0, 0.02, 0.03, 0.04, 1.91, 1.92, 1.93, 3.81, 3.82, 3.83, 8.11,
        13.61, 5.01, 6.01, 6.20, 6.40, 7.03, 8.21, 9.31, 10.10, 11.42, 11.43, 13.31, 13.32, 13.33};
    double drainExpected[25] = {1,1,1,1,1,1,1,1,1,1,-1.40, -9.29, -8.57, -6.97, -5.55, -4.32, -1.84, -0.53, -0.31, 1,1,1,1,1,1};
    double drainoutExpected = 0.1;
    double standingWaterExpected = 0.01;

    //Begin TEST for if swc[i] <= swcmin[i]
    infiltrate_water_low(swc, drain, &drainout, nlyrs, sdrainpar, sdraindpth, swcfc, width, swcmin,
      swcsat, impermeability, &standingWater);

    EXPECT_NEAR(drainoutExpected, drainout, 0.0000001); //drainout expected to be 0.1
    EXPECT_NEAR(standingWaterExpected, standingWater, 0.0000001); //standingWater expected to be 0.01

    for(unsigned int i = 0; i < nlyrs; i++){

      EXPECT_NEAR(swcExpected[i], swc[i], 0.0000001); //swc is expected to match swcExpected
      EXPECT_NEAR(drainExpected[i], drain[i], 0.0000001); //drain is expected to match drainExpected
    }

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Begin TEST for if swc[i] > swcmin[i]
    double swcExpected2[25] = {0, 0.03000011, 0.04000101, 0.05000102, 1.92000405, 1.93000306, 1.94000103,
        3.82000707, 3.83000207, 3.84000405, 8.21004642, 13.61000000, 5.01000000, 6.01000000, 6.20000000,
          6.40000000, 7.03000000, 8.21000000, 9.31000000, 10.10000000, 11.43002411, 11.44000100,
            13.32000000, 13.33000000, 13.34000000};
    double drainExpected2[25] = {1.0000950, 1.0000949, 1.0000939, 1.0000929, 1.0000888, 1.0000858,
        1.0000847, 1.0000777, 1.0000756, 1.0000715, -1.4899749, -9.3699749, -8.6399749, -7.0299749,
          -5.5999749, -4.3599749, -1.8699749, -0.5499749, -0.3199749, 1.0000251, 1.0000010, 1.0000000,
            1.0000000, 0.9999500, 0.9999000};
    standingWaterExpected = 0.019905;

    //Reset altered INPUTS
    double swc1[25] = {0.02, 0.03, 0.04, 0.05, 1.92, 1.93, 1.94, 3.82, 3.83, 3.84, 5.72, 5.73, 5.74,
        7.62, 7.63, 7.64, 9.52, 9.53, 9.54, 11.42, 11.43, 11.44, 13.32, 13.33, 13.34};
    double drain1[25] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
    drainout = 0.1, standingWater = 0;

    infiltrate_water_low(swcmin, drain1, &drainout, nlyrs, sdrainpar, sdraindpth, swcfc, width, swc1,
      swcsat, impermeability, &standingWater); //switched swc and swcmin arrays to avoid creating new arrays, replaced swc with swc1

    EXPECT_DOUBLE_EQ(drainoutExpected, drainout); //drainout is expected to be 0.1

    EXPECT_NEAR(standingWater, standingWaterExpected, 0.01); //standingWater is expected to be 0.02;  small precision is needed

    for(unsigned int i = 0; i < nlyrs; i++){

      EXPECT_NEAR(swcmin[i], swcExpected2[i], 0.001); //swc is expected to match swcExpected, swcmin has replaced swc
      EXPECT_NEAR(drain[i], drainExpected2[i], 0.1); //drain is expected to match drainExpected. ASK about negative values
    }

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Begin TEST for if swc[j] > swcsat[j]
    //INPUTS
    double swc3[25] = {0.1, 2.5, 3.6, 4.8, 5.3, 6.2, 7.9, 8.2, 8.3, 9.99, 10.77, 13.61, 6.01, 8.01,
        8.2, 8.4, 10.03, 10.21, 10.31, 12.10, 12.01, 13.01, 14.01, 14.02, 15};
    double swcsat2[25] = {0.01, 0.03, 0.04, 0.05, 1.92, 1.93, 1.94, 3.82, 3.83, 3.84, 5.72, 5.73, 5.74,
        7.62, 7.63, 7.64, 9.52, 9.53, 9.54, 11.42, 11.43, 11.44, 13.32, 13.33, 13.34};
    double drain3[25] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
    standingWater = 0;

    //INPUTS for expected outputs
    double swcExpected4[25] = {0.01, 0.03, 0.04, 0.05, 1.92, 1.93, 1.94, 3.82, 3.83, 3.84, 5.72, 5.73, 5.74, 7.62,
        7.63, 7.64, 9.52, 9.53, 9.54, 11.42, 11.43, 11.44, 13.32, 13.33, 13.34};
    double drainExpected4[25] = {-61.1385, -58.6685, -55.1085, -50.3585, -46.9785, -42.7085, -36.7485, -32.3685,
        -27.8985, -21.7485, -16.6985, -8.8185, -8.5485, -8.1585, -7.5885, -6.8285, -6.3185, -5.6385, -4.8685,
          -4.1885, -3.6085, -2.0385, -1.3485, -0.6585, 1};
    standingWaterExpected = 62.2185;

    infiltrate_water_low(swc3, drain3, &drainout, nlyrs, sdrainpar, sdraindpth, swcfc, width, swc1,
      swcsat2, impermeability, &standingWater); //swc1 array used instead of swcmin

    EXPECT_DOUBLE_EQ(drainoutExpected, drainout); //drainout is expected to be 0.1
    EXPECT_NEAR(standingWater, standingWaterExpected, 0.1); //standingWater is expected to be 62.2185

    for(unsigned int i = 0; i < nlyrs; i++){

      EXPECT_NEAR(swc3[i], swcExpected4[i], 0.0000001); //swc is expected to match swcExpected
      EXPECT_NEAR(drain3[i], drainExpected4[i], 0.01); //drain is expected to match drainExpected
    }

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Begin TEST for if swc[j] <= swcsat[j]
    //INPUTS
    double swcsat3[25] = {0.1, 2.5, 3.6, 4.8, 5.3, 6.2, 7.9, 8.2, 8.3, 9.99, 10.77, 13.61, 6.01, 8.01,
        8.2, 8.4, 10.03, 10.21, 10.31, 12.10, 12.01, 13.01, 14.01, 14.02, 15};
    double swc4[25] = {0.01, 0.03, 0.04, 0.05, 1.92, 1.93, 1.94, 3.82, 3.83, 3.84, 5.72, 5.73, 5.74,
        7.62, 7.63, 7.64, 9.52, 9.53, 9.54, 11.42, 11.43, 11.44, 13.32, 13.33, 13.34};
    double drain4[25] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
    double swcmin2[25] = {0.01, 0.02, 0.03, 0.04, 1.91, 1.92, 1.93, 3.81, 3.82, 3.83, 5.71, 5.72, 5.73,
        7.61, 7.62, 7.63, 9.51, 9.52, 9.53, 11.41, 11.42, 11.43, 13.31, 13.32, 13.33};
    standingWater = 0;

    //INPUTS for expected outputs
    double swcExpected5[25] = {0.01000000, 0.02990300, 0.03999956, 0.05000050, 1.92000202, 1.93000153,
        1.94000052, 3.82000354, 3.83000104, 3.84000203, 5.72000052, 5.73000051, 5.74000253, 7.62000103,
          7.63000202, 7.64000052, 9.52000353, 9.53000053, 9.54000202, 11.42001009, 11.43001214, 11.44000056, 13.32000000, 13.33002513, 13.34000000};
    double drainExpected5[25] = {1.000000, 1.000097, 1.000097, 1.000097, 1.000095, 1.000093, 1.000093,
        1.000089, 1.000088, 1.000086, 1.000086, 1.000085, 1.000083, 1.000082, 1.000080, 1.000079, 1.000076,
          1.000075, 1.000073, 1.000063, 1.000051, 1.000050, 1.000050, 1.000025, 1.000000};
    standingWaterExpected = 0;

    infiltrate_water_low(swc4, drain4, &drainout, nlyrs, sdrainpar, sdraindpth, swcfc, width, swcmin2,
      swcsat3, impermeability, &standingWater);

    EXPECT_DOUBLE_EQ(drainoutExpected, drainout);
    EXPECT_NEAR(standingWater, standingWaterExpected, 0.0000001); //standingWater is expected to be 0

    for(unsigned int i = 0; i < nlyrs; i++){

      EXPECT_NEAR(swc4[i], swcExpected5[i], 0.01); //swc is expected to match swcExpected
      EXPECT_NEAR(drain4[i], drainExpected5[i], 0.1); //drain is expected to match drainExpected(near 1)
    }

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Begin testing when nlyrs = 1
    nlyrs = 1;

    //INPUTS for one layer
    double swc_1[1] = {0.01};
    double drain_1[1] = {1};
    drainout = 0.1, sdrainpar = 0.6, sdraindpth = 6, standingWater = 0;
    double swcfc_1[1] = {0.33};
    double width_1[1] = {5};
    double swcmin_1[1] = {0.02};
    double swcsat_1[1] = {0};
    double impermeability_1[1] = {0.05};

    //INPUTS for expected outputs
    double swcExpected_1[1] = {0};
    double drainExpected_1[1] = {1};
    drainoutExpected = 0.1;
    standingWaterExpected = 0.01;

    //Begin TEST for if swc[i] <= swcmin[i]
    infiltrate_water_low(swc_1, drain_1, &drainout, nlyrs, sdrainpar, sdraindpth, swcfc_1, width_1, swcmin_1,
      swcsat_1, impermeability_1, &standingWater);

    EXPECT_DOUBLE_EQ(drainoutExpected, drainout); //drainout is expected to be 0.1
    EXPECT_DOUBLE_EQ(standingWaterExpected, standingWater); //standingWater is expected to be 0.01

    for(unsigned int i = 0; i < nlyrs; i++){

      EXPECT_DOUBLE_EQ(swcExpected_1[i], swc_1[i]); //swc is expected to match swcExpected
      EXPECT_DOUBLE_EQ(drainExpected_1[i], drain_1[i]); //drain is expected to match drainExpected

    }

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Begin TEST for if swc[i] > swcmin[i]
    double swcExpected2_1[1] = {0};
    double drainExpected2_1[1] = {1.019};
    standingWaterExpected = 0.011, drainoutExpected = 0.119;

    infiltrate_water_low(swcmin_1, drain_1, &drainout, nlyrs, sdrainpar, sdraindpth, swcfc_1, width_1, swc_1,
      swcsat_1, impermeability_1, &standingWater); //switch swc and swcmin arrays

    EXPECT_NEAR(drainout, drainoutExpected, 0.0000001);
    EXPECT_NEAR(standingWater, standingWaterExpected, 0.0000001); //standingWater is expected to be 0.0105

    for(unsigned int i = 0; i < nlyrs; i++){

      EXPECT_NEAR(swc_1[i], swcExpected2_1[i], 0.0000001); //swc is expected to match swcExpected
      EXPECT_NEAR(drain_1[i], drainExpected2_1[i], 0.0000001); //drain is expected to match drainExpected
    }

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Begin TEST for if lyrFrozen == true
    //INPUTS for expected outputs
    double swcExpected3_1[1] = {0};
    double drainoutExpectedf = 0.119;

    infiltrate_water_low(swcmin_1, drain_1, &drainout, nlyrs, sdrainpar, sdraindpth, swcfc_1, width_1, swc_1,
      swcsat_1, impermeability_1, &standingWater);

    for(unsigned int i = 0; i < nlyrs; i++){
      if (st->lyrFrozen[i] == swTRUE){

        EXPECT_NEAR(standingWater, standingWaterExpected, 0.0001); //standingWater is expected to be 0.011
        EXPECT_NEAR(drainoutExpectedf, drainout, 0.0001); //drainout is expected to be 0.1
        EXPECT_NEAR(swc_1[i], swcExpected3_1[i], 0.0001); //swc is expected to match swcExpected
        EXPECT_NEAR(drain_1[i], drainExpected2_1[i], 0.0001); //drain is expected to match drainExpected
      }
    //Begin TEST for if lyrFrozen == false
      else{
        //INPUTS for expected outputs
        standingWaterExpected = 0.011;

        EXPECT_NEAR(standingWater, standingWaterExpected, 0.0000001); //standingWater is expected to be 0.011
        EXPECT_NEAR(drainout, drainoutExpected, 0.0000001);//drainout is expected to be 0.1
        EXPECT_NEAR(swc_1[i], swcExpected2_1[i], 0.0000001); //swc is expected to match swcExpected
        EXPECT_NEAR(drain_1[i], drainExpected2_1[i], 0.0000001); //drain is expected to match drainExpected
      }
    }

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Begin TEST for if swc[j] > swcsat[j]
    //INPUTS
    double swc2_1[1] = {1.02};
    double swcsat2_1[1] = {1.01};

    //INPUTS for expected outputs
    double swcExpected4_1[1] = {0};
    double drainExpected4_1[1] = {1.589};
    standingWaterExpected = 0.011;
    drainoutExpected = 0.689;

    infiltrate_water_low(swc2_1, drain_1, &drainout, nlyrs, sdrainpar, sdraindpth, swcfc_1, width_1, swcmin_1,
      swcsat2_1, impermeability_1, &standingWater);

    EXPECT_NEAR(drainout, drainoutExpected, 0.0000001); //drainout is expected to be 0.67
    EXPECT_NEAR(standingWater, standingWaterExpected, 0.0000001); //No standingWater expected

    for(unsigned int i = 0; i < nlyrs; i++){

      EXPECT_NEAR(swc_1[i], swcExpected4_1[i], 0.0000001); //swc is expected to match swcExpected
      EXPECT_NEAR(drain_1[i], drainExpected4_1[i], 0.0000001); //drain is expected to match drainExpected
    }

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Begin TEST for if swc[j] <= swcsat[j]
    //INPUTS
    double swcsat3_1[1] = {5.0};

    //INPUTS for expected outputs
    double swcExpected5_1[1] = {0};
    double drainExpected5_1[1] = {1.589};
    standingWaterExpected = 0.011;

    infiltrate_water_low(swc_1, drain_1, &drainout, nlyrs, sdrainpar, sdraindpth, swcfc_1, width_1, swcmin_1,
      swcsat3_1, impermeability_1, &standingWater);

    EXPECT_NEAR(drainout, drainoutExpected, 0.0000001); //drainout is expected to be 0.67
    EXPECT_NEAR(standingWater, standingWaterExpected, 0.0000001); //No standingWater expected

    for(unsigned int i = 0; i < nlyrs; i++){

      EXPECT_NEAR(swc_1[i], swcExpected5_1[i], 0.0000001); //swc is expected to match swcExpected
      EXPECT_NEAR(drain_1[i], drainExpected5_1[i], 0.0000001); //drain is expected to match drainExpected
    }

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();
  }

  //TEST for hydraulic_redistribution when nlyrs = MAX_LAYERS and nlyrs = 1
  TEST(SWFlowTest, hydraulic_redistribution){

    //INPUTS
    double swc[25] = {5.01,10.005,10.02,10.03,20.01, 20.02, 20.03, 20.04, 20.01, 20.05, 20.06, 20.07,
        20.08, 20.09, 20.10, 20.11, 20.12, 20.13, 20.14, 20.15, 20.16, 20.17, 20.18, 20.19, 20.2};
    double swcwp[25] = {5,10,10,10,20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20};
    double lyrRootCo[25] = {.10,.11,.12,.13,.14,.15,.16,.17, .18,.19,.20,.21,.22,.23,.24,.25, .26,.27,.28,.29,.30,.31,.32,.33, .34};
    double hydred[25] = {1,2,3,4,5,6,7,8, 1,2,3,4,5,6,7,8, 1,2,3,4,5,6,7,8, 9}; //hydred[0] != 0 for test, otherwise these don't matter as they are reset to zero in function
    unsigned int nlyrs = MAX_LAYERS;
    double maxCondroot = -0.2328;
    double swp50 = 1.2e12;
    double shapeCond = 1;
    double scale = 0.3;

    RealF dmax[25] = {5, 15, 25, 35, 55, 75, 95, 115, 135, 155, 175, 195, 215, 235, 255, 275, 295, 315, 335, 355, 375, 395, 415, 435, 455};
    RealF matricd[25] = {1.430, 1.410, 1.390, 1.390, 1.380, 1.150, 1.130, 1.130, 1.430, 1.410, 1.390,
        1.390, 1.380, 1.150, 1.130, 1.130,1.430, 1.410, 1.390, 1.390, 1.380, 1.150, 1.130, 1.130, 1.4};
    RealF f_gravel[25] = {0.1, 0.1, 0.1, 0.1, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2};
    RealF evco[25] = {0.813, 0.153, 0.034, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    RealF trco_grass[25] = {0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.0496, 0.0495, 0.1006,
        0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.1999};
    RealF trco_shrub[25] = {0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.134, 0.094, 0.176, 0.175,
        0.110, 0.109, 0.101, 0.101, 0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.1999};
    RealF trco_tree[25] = {0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.0496, 0.0495, 0.1006,
        0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.0496, 0.0495, 0.1006, 0.1006, 0.1006, 0.1997, 0.1997, 0.1997, 0.1999};
    RealF trco_forb[25] = {0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.134, 0.094, 0.176, 0.175, 0.110,
        0.109, 0.101, 0.101, 0.134, 0.094, 0.176, 0.175, 0.110, 0.109, 0.101, 0.101, 0.1999};
    RealF psand[25] = {0.51, 0.44, 0.35, 0.32, 0.31, 0.32, 0.57, 0.57, 0.51, 0.44, 0.35, 0.32, 0.31, 0.32,
        0.57, 0.57, 0.51, 0.44, 0.35, 0.32, 0.31, 0.32, 0.57, 0.57, 0.58};
    RealF pclay[25] = {0.15, 0.26, 0.41, 0.45, 0.47, 0.47, 0.28, 0.28, 0.15, 0.26, 0.41, 0.45, 0.47, 0.47,
        0.28, 0.28, 0.15, 0.26, 0.41, 0.45, 0.47, 0.47, 0.28, 0.28, 0.29};
    RealF imperm[25] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    RealF soiltemp[25] = {-1, -1, -1, -1, 0, 0, 1, 1, -1, -1, -1, -1, 0, 0, 1, 1, -1, -1, -1, -1, 0, 0, 1, 1, 2};

    _set_layers(nlyrs, dmax, matricd, f_gravel,
      evco, trco_grass, trco_shrub, trco_tree,
      trco_forb, psand, pclay, imperm, soiltemp);

    //INPUTS for expected outputs
    double swcExpected[25] = {5.010, 10.005, 10.020, 10.030, 20.010, 20.020, 20.030, 20.040,
        20.010, 20.050, 20.060, 20.070, 20.080, 20.090, 20.100, 20.110, 20.120, 20.130, 20.140,
          20.150, 20.160, 20.170, 20.180, 20.190, 20.200};
    double hydredExpected[25] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    //Begin TESTing when nlyrs = MAX
    //Begin TEST for if swp[i] < swpwp[i] OR swp[j] < swpwp[j] AND lyrFrozen == false; j = i + 1
    hydraulic_redistribution(swcwp, swc, lyrRootCo, hydred, nlyrs, maxCondroot, swp50, shapeCond, scale);
      //inputs for swc and swcwp have been switched

    EXPECT_DOUBLE_EQ(hydred[0], 0); //no hydred in top layer
    for(unsigned int i = 0; i < nlyrs; i++){

      EXPECT_NEAR(swc[i], swcExpected[i], 0.0000001); //swc is expected to match swcExpected
      EXPECT_NEAR(hydred[i], hydredExpected[i], 0.0000001); //hydred is expected to match hydredExpected
    }

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Begin TEST for if else ^^
    //INPUTS for expected outputs.
    double swcExpected2[25] = {5.010, 10.005, 10.020, 10.030, 20.010, 20.020, 20.030, 20.040,
        20.010, 20.050, 20.060, 20.070, 20.080, 20.090, 20.100, 20.110, 20.120, 20.130, 20.140,
          20.150, 20.160, 20.170, 20.180, 20.190, 20.200};
    double hydredExpected2[25] = {0, 4.298116e-07, -8.801668e-08, -1.314532e-07,
        -1.885189e-07, -2.053540e-07, -1.214441e-07, -1.316728e-07, 2.016268e-06, 1.453133e-07,
          -2.634546e-07, -3.011598e-07, -3.274744e-07, -3.483882e-07, -2.135987e-07, -2.271032e-07,
            2.816679e-06, 1.642664e-07, -4.138196e-07, -4.647347e-07, -4.989383e-07, -5.252975e-07,
              -3.362445e-07, -3.541813e-07, -4.314838e-07};

    _set_layers(nlyrs, dmax, matricd, f_gravel,
      evco, trco_grass, trco_shrub, trco_tree,
      trco_forb, psand, pclay, imperm, soiltemp);

    hydraulic_redistribution(swc, swcwp, lyrRootCo, hydred, nlyrs, maxCondroot, swp50, shapeCond, scale);

    EXPECT_DOUBLE_EQ(hydred[0], 0); //no hydred in top layer
    for(unsigned int i = 0; i < nlyrs; i++){

      EXPECT_NEAR(swc[i], swcExpected2[i], 0.0001); //swc is expected to match swcExpected
      EXPECT_NEAR(hydred[i], hydredExpected2[i], 0.0001); //hydred is expected to match hydredExpected
    }

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Begin TESTing when nlyrs = 1
    //INPUTS
    double swc1[1] = {0.02};
    double swcwp1[1] = {0.01};
    double lyrRootCo1[1] = {0.1};
    double hydred1[1] = {1}; //hydred[0] != 0 for test
    nlyrs = 1;
    maxCondroot = -0.2328;
    swp50 = 10;
    shapeCond = 1;
    scale = 0.3;

    //INPUTS for expected outputs
    double swcExpected11[1] = {0.02};
    double hydredExpected11[1] = {0};

    //Begin TESTing when nlyrs = 1
    //Begin TEST for if swp[i] < swpwp[i] OR swp[j] < swpwp[j] AND lyrFrozen == false; j = i + 1
    hydraulic_redistribution(swcwp1, swc1, lyrRootCo1, hydred1, nlyrs, maxCondroot, swp50, shapeCond, scale);
      //inputs for swc and swcwp have been switched

    EXPECT_DOUBLE_EQ(hydred[0], 0); //no hydred in top layer
    for(unsigned int i = 0; i < nlyrs; i++){

      EXPECT_NEAR(swc1[i], swcExpected11[i], 0.0000001); //swc is expected to match swcExpected
      EXPECT_NEAR(hydred1[i], hydredExpected11[i], 0.0000001); //hydred is expected to match hydredExpected
    }

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();

    //Begin TEST for if else ^^
    //INPUTS for expected outputs.
    double swcExpected21[1] = {0.02};
    double hydredExpected21[1] = {0};

    hydraulic_redistribution(swc1, swcwp1, lyrRootCo1, hydred1, nlyrs, maxCondroot, swp50, shapeCond, scale);

    EXPECT_DOUBLE_EQ(hydred[0], 0); //no hydred in top layer

    for(unsigned int i = 0; i < nlyrs; i++){

      EXPECT_NEAR(swc1[i], swcExpected21[i], 0.0000001); //swc is expected to match swcExpected
      EXPECT_NEAR(hydred1[i], hydredExpected21[i], 0.0000001); //hydred is expected to match hydredExpected
    }

    //Reset to previous global states.
    Reset_SOILWAT2_after_UnitTest();
  }
}
