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

#include "sw_testhelpers.h"


extern SW_CARBON SW_Carbon;
extern SW_MODEL SW_Model;
extern SW_VEGPROD SW_VegProd;
extern SW_SITE SW_Site;
extern LyrIndex _TranspRgnBounds[];


namespace {
  // Test the water equation function 'water_eqn'
  TEST(SWSiteTest, WaterEquation) {

    //declare inputs
    RealD fractionGravel = 0.1, sand = .33, clay =.33;
    LyrIndex n = 1;

    water_eqn(fractionGravel, sand, clay, n);

    // Test swcBulk_saturated
    EXPECT_GT(SW_Site.lyr[n]->swcBulk_saturated, 0.); // The swcBulk_saturated should be greater than 0
    EXPECT_LT(SW_Site.lyr[n]->swcBulk_saturated, SW_Site.lyr[n]->width); // The swcBulk_saturated can't be greater than the width of the layer

    // Test thetasMatric
    EXPECT_GT(SW_Site.lyr[n]->thetasMatric, 36.3); /* Value should always be greater
    than 36.3 based upon complete consideration of potential range of sand and clay values */
    EXPECT_LT(SW_Site.lyr[n]->thetasMatric, 46.8); /* Value should always be less
    than 46.8 based upon complete consideration of potential range of sand and clay values */
    EXPECT_DOUBLE_EQ(SW_Site.lyr[n]->thetasMatric,  44.593); /* If sand is .33 and
    clay is .33, thetasMatric should be 44.593 */

    // Test psisMatric
    EXPECT_GT(SW_Site.lyr[n]->psisMatric, 3.890451); /* Value should always be greater
    than 3.890451 based upon complete consideration of potential range of sand and clay values */
    EXPECT_LT(SW_Site.lyr[n]->psisMatric,  34.67369); /* Value should always be less
    than 34.67369 based upon complete consideration of potential range of sand and clay values */
    EXPECT_DOUBLE_EQ(SW_Site.lyr[n]->psisMatric, 27.586715750763947); /* If sand is
    .33 and clay is .33, psisMatric should be 27.5867 */

    // Test bMatric
    EXPECT_GT(SW_Site.lyr[n]->bMatric, 2.8); /* Value should always be greater than
    2.8 based upon complete consideration of potential range of sand and clay values */
    EXPECT_LT(SW_Site.lyr[n]->bMatric, 18.8); /* Value should always be less
    than 18.8 based upon complete consideration of potential range of sand and clay values */
    EXPECT_DOUBLE_EQ(SW_Site.lyr[n]->bMatric, 8.182); /* If sand is .33 and clay is .33,
    thetasMatric should be 8.182 */

    // Reset to previous global states
    Reset_SOILWAT2_after_UnitTest();
  }

  // Test that water equation function 'water_eqn' fails
  TEST(SWSiteTest, WaterEquationDeathTest) {

    //declare inputs
    RealD fractionGravel = 0.1;
    LyrIndex n = 1;

    // Test that error will be logged when b_matric is 0
    RealD sand = 10. + 1./3.; // So that bmatric will equal 0, even though this is a very irrealistic value
    RealD clay = 0;

    EXPECT_DEATH_IF_SUPPORTED(water_eqn(fractionGravel, sand, clay, n), "@ generic.c LogError");

  }


  // Test that soil transpiration regions are derived well
  TEST(SWSiteTest, SoilTranspirationRegions) {
    /* Notes:
        - SW_Site.n_layers is base1
        - soil layer information in _TranspRgnBounds is base0
    */

    LyrIndex
      i, id,
      nRegions,
      prev_TranspRgnBounds[MAX_TRANSP_REGIONS] = {0};
    RealD
      soildepth;

    for (i = 0; i < MAX_TRANSP_REGIONS; ++i) {
      prev_TranspRgnBounds[i] = _TranspRgnBounds[i];
    }


    // Check that "default" values do not change region bounds
    nRegions = 3;
    RealD regionLowerBounds1[] = {20., 40., 100.};
    derive_soilRegions(nRegions, regionLowerBounds1);

    for (i = 0; i < nRegions; ++i) {
      // Quickly calculate soil depth for current region as output information
      soildepth = 0.;
      for (id = 0; id <= _TranspRgnBounds[i]; ++id) {
        soildepth += SW_Site.lyr[id]->width;
      }

      EXPECT_EQ(prev_TranspRgnBounds[i], _TranspRgnBounds[i]) <<
        "for transpiration region = " << i + 1 <<
        " at a soil depth of " << soildepth << " cm";
    }


    // Check that setting one region for all soil layers works
    nRegions = 1;
    RealD regionLowerBounds2[] = {100.};
    derive_soilRegions(nRegions, regionLowerBounds2);

    for (i = 0; i < nRegions; ++i) {
      EXPECT_EQ(SW_Site.n_layers - 1, _TranspRgnBounds[i]) <<
        "for a single transpiration region across all soil layers";
    }


    // Check that setting one region for one soil layer works
    nRegions = 1;
    RealD regionLowerBounds3[] = {SW_Site.lyr[0]->width};
    derive_soilRegions(nRegions, regionLowerBounds3);

    for (i = 0; i < nRegions; ++i) {
      EXPECT_EQ(0, _TranspRgnBounds[i]) <<
        "for a single transpiration region for the shallowest soil layer";
    }


    // Check that setting the maximal number of regions works
    nRegions = MAX_TRANSP_REGIONS;
    RealD *regionLowerBounds4 = new RealD[nRegions];
    // Example: one region each for the topmost soil layers
    soildepth = 0.;
    for (i = 0; i < nRegions; ++i) {
      soildepth += SW_Site.lyr[i]->width;
      regionLowerBounds4[i] = soildepth;
    }
    derive_soilRegions(nRegions, regionLowerBounds4);

    for (i = 0; i < nRegions; ++i) {
      EXPECT_EQ(i, _TranspRgnBounds[i]) <<
        "for transpiration region for the " << i + 1 << "-th soil layer";
    }

    delete[] regionLowerBounds4;

    // Reset to previous global states
    Reset_SOILWAT2_after_UnitTest();
  }

} // namespace
