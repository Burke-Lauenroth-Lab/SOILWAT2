/********************************************************/
/********************************************************/
/*  Source file: SW_Site.h
	Type: header
	Application: SOILWAT - soilwater dynamics simulator
	Purpose: Define a structure to hold parameters that
	         are read in Site::read() and passed to
	         Layers::init(*).  There are a couple of
	         parameters that belong at the site level
	         but it also makes sense to keep the layer
	         parms in the same site input file.
	History:
	   (8/28/01) -- INITIAL CODING - cwb
	   1-Oct-03  (cwb) Removed variables sum_evap_coeff and
	             *sum_transp_coeff.  sum_evap_coeff doesn't
	             do anything since it's always 1.0 (due
	             to normalization) and sum_transp_coeff[]
	             can easily be calculated in the only
	             function where it applies: transp_weighted_avg().
	(10/12/2009) - (drs) added altitude
	11/02/2010	(drs) added snow parameters to SW_SITE to be read in from siteparam.in
	10/19/2010	(drs) added Bool HydraulicRedistribution flag and RealD maxCondroot, swp50, and shapeCond parameters to the structure SW_SITE
	07/20/2011	(drs) added RealD impermeability to struct SW_LAYER_INFO: fraction of how impermeable a layer is (0=permeable, 1=impermeable)
 					added RealD swc_saturated to struct SW_LAYER_INFO: saturated soil water content (cm) * width
	09/08/2011	(drs) moved all hydraulic redistribution parameters to SW_VegProd.h struct VegType
	09/09/2011	(drs) added RealD transp_coeff_xx for each vegetation type (tree, shrub, grass) to struct SW_LAYER_INFO
	09/09/2011	(drs) added RealD my_transp_rgn_xx for each vegetation type (tree, shrub, grass) to struct SW_LAYER_INFO
						added RealD n_transp_lyrs_xx for each vegetation type (tree, shrub, grass) to struct SW_SITE
	09/15/2011	(drs)	deleted RealD albedo in struct SW_SITE and moved it to SW_VegProd.h to make input vegetation type dependent
	02/04/2012	(drs)	added Reald swc_atSWPcrit_xx for each vegetation type (tree, shrub, grass) to struct SW_LAYER_INFO: swc at the critical soil water potential
	05/24/2012  (DLM) added variables for Soil Temperature constants to SW_SITE struct
	05/25/2012  (DLM) added variable sTemp to SW_LAYER_INFO struct to keep track of the soil temperature for each soil layer
	05/30/2012  (DLM) added stDeltaX variable for soil_temperature function to SW_SITE struct
	05/31/2012  (DLM) added use_soil_temp, stMaxDepth, stNRGR variables to SW_SITE struct
	11/06/2012	(clk)	added slope and aspect to SW_SITE struct
	11/30/2010	(clk)	added the variable 'percentRunoff' which is the percent of surface water that is lost do to surface runoff
 	04/16/2013	(clk)	added the variables soilMatric_density and fractionVolBulk_gravel
 						soilMatric_density is the soil density excluding the gravel component
						fractionVolBulk_gravel is the gravel content as a fraction of the bulk soil
						renamed many of the of the variables in SW_LAYER_INFO to better reflect BULK and MATRIC values
 						due to the renaming, also had to update the use of these variables in the other files.
	07/09/2013	(clk)	added the variables transp_coeff[SW_FORBS], swcBulk_atSWPcrit[SW_FORBS], and my_transp_rgn[SW_FORBS] to SW_LAYER_INFO
	07/09/2013	(clk)	added the variable n_transp_lyrs_forb to SW_SITE
*/
/********************************************************/
/********************************************************/
#ifndef SW_SITE_H
#define SW_SITE_H

#include "SW_Defines.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef unsigned int LyrIndex;

typedef struct {
	/* bulk = relating to the whole soil, i.e., matric + rock/gravel/coarse fragments */
	/* matric = relating to the < 2 mm fraction of the soil, i.e., sand, clay, and silt */

	RealD
		/* Inputs */
		width, /* width of the soil layer (cm) */
		soilMatric_density, /* matric soil density of the < 2 mm fraction, i.e., gravel component excluded, (g/cm3) */
		evap_coeff, /* prop. of total soil evap from this layer */
		transp_coeff[NVEGTYPES], /* prop. of total transp from this layer    */
		fractionVolBulk_gravel, /* gravel content (> 2 mm) as volume-fraction of bulk soil (g/cm3) */
		fractionWeightMatric_sand, /* sand content (< 2 mm & > . mm) as weight-fraction of matric soil (g/g) */
		fractionWeightMatric_clay, /* clay content (< . mm & > . mm) as weight-fraction of matric soil (g/g) */
		impermeability, /* fraction of how impermeable a layer is (0=permeable, 1=impermeable)    */
		sTemp, /* initial soil temperature for each soil layer */

		/* Derived soil characteristics */
		soilBulk_density, /* bulk soil density of the whole soil, i.e., including rock/gravel component, (g/cm3) */
		swcBulk_fieldcap, /* Soil water content (SWC) corresponding to field capacity (SWP = -0.033 MPa) [cm] */
		swcBulk_wiltpt, /* SWC corresponding to wilting point (SWP = -1.5 MPa) [cm] */
		swcBulk_min, /* Minimal SWC [cm] */
		swcBulk_wet, /* SWC considered "wet" [cm] */
		swcBulk_init, /* Initial SWC for first day of simulation [cm] */
		swcBulk_atSWPcrit[NVEGTYPES], /* SWC corresponding to critical SWP for transpiration */

		/* Saxton et al. 2006 */
		swcBulk_saturated, /* saturated bulk SWC [cm] */

		/* Cosby et al. (1984): SOILWAT2's soil water retention curve */
		thetasMatric, /* saturated matric SWC [cm] */
		psisMatric, /* saturated matric SWP [cm] */
		bMatric, /* slope of the logarithmic retention curve */
		binverseMatric; /* inverse of bMatric */

	LyrIndex my_transp_rgn[NVEGTYPES]; /* which transp zones from Site am I in? */
} SW_LAYER_INFO;


typedef struct {

	Bool reset_yr, /* 1: reset values at start of each year */
		deepdrain, /* 1: allow drainage into deepest layer  */
		use_soil_temp; /* whether or not to do soil_temperature calculations */

	LyrIndex n_layers, /* total number of soil layers */
		n_transp_rgn, /* soil layers are grouped into n transp. regions */
		n_evap_lyrs, /* number of layers in which evap is possible */
		n_transp_lyrs[NVEGTYPES], /* layer index of deepest transp. region       */
		deep_lyr; /* index of deep drainage layer if deepdrain, 0 otherwise */

	RealD slow_drain_coeff, /* low soil water drainage coefficient   */
		pet_scale,	/* changes relative effect of PET calculation */
		longitude,	/* longitude of the site (radians)        */
		latitude,	/* latitude of the site (radians)        */
		altitude,	/* altitude a.s.l (m) of the site */
		slope,		/* slope of the site (radians): between 0 (horizontal) and pi / 2 (vertical) */
		aspect,		/* aspect of the site (radians): A value of \ref SW_MISSING indicates no data, ie., treat it as if slope = 0; South facing slope: aspect = 0, East = -pi / 2, West = pi / 2, North = ±pi */
					/* SWAT2K model parameters : Neitsch S, Arnold J, Kiniry J, Williams J. 2005. Soil and water assessment tool (SWAT) theoretical documentation. version 2005. Blackland Research Center, Texas Agricultural Experiment Station: Temple, TX. */
		TminAccu2,	/* Avg. air temp below which ppt is snow ( C) */
		TmaxCrit,	/* Snow temperature at which snow melt starts ( C) */
		lambdasnow,	/* Relative contribution of avg. air temperature to todays snow temperture vs. yesterday's snow temperature (0-1) */
		RmeltMin,	/* Minimum snow melt rate on winter solstice (cm/day/C) */
		RmeltMax,	/* Maximum snow melt rate on summer solstice (cm/day/C) */
		t1Param1,	/* Soil temperature constants */
		t1Param2,	/* t1Params are the parameters for the avg daily temperature at the top of the soil (T1) equation */
		t1Param3,
		csParam1,	/* csParams are the parameters for the soil thermal conductivity (cs) equation */
		csParam2,
		shParam,	/* shParam is the parameter for the specific heat capacity equation */
		bmLimiter,	/* bmLimiter is the biomass limiter constant, for use in the T1 equation */
		Tsoil_constant, 	/* soil temperature at a depth where soil temperature is (mostly) constant in time; for instance, approximated as the mean air temperature */
		stDeltaX,		/* for the soil_temperature function, deltaX is the distance between profile points (default: 15) */
		stMaxDepth,		/* for the soil_temperature function, the maxDepth of the interpolation function */
		percentRunoff,	/* the percentage of surface water lost daily */
		percentRunon;	/* the percentage of water that is added to surface gained daily */

	unsigned int stNRGR; /* number of interpolations, for the soil_temperature function */
	/* params for tanfunc rate calculations for evap and transp. */
	/* tanfunc() creates a logistic-type graph if shift is positive,
	 * the graph has a negative slope, if shift is 0, slope is positive.
	 */
	tanfunc_t evap, transp;

	SW_LAYER_INFO **lyr; 	/* one struct per soil layer pointed to by   */
							/* a dynamically allocated block of pointers */

} SW_SITE;


void water_eqn(RealD fractionGravel, RealD sand, RealD clay, LyrIndex n);
RealD calculate_soilBulkDensity(RealD matricDensity, RealD fractionGravel);
LyrIndex nlayers_bsevap(void);
void nlayers_vegroots(LyrIndex n_transp_lyrs[]);

void SW_SIT_construct(void);
void SW_SIT_deconstruct(void);
void SW_SIT_init_counts(void);
void SW_SIT_read(void);
void SW_SIT_init_run(void);
void _echo_inputs(void);

/* these used to be in Layers */
void SW_SIT_clear_layers(void);
LyrIndex _newlayer(void);
void add_deepdrain_layer(void);

void set_soillayers(LyrIndex nlyrs, RealF *dmax, RealF *matricd, RealF *f_gravel,
  RealF *evco, RealF *trco_grass, RealF *trco_shrub, RealF *trco_tree,
  RealF *trco_forb, RealF *psand, RealF *pclay, RealF *imperm, RealF *soiltemp,
  int nRegions, RealD *regionLowerBounds);
void derive_soilRegions(int nRegions, RealD *regionLowerBounds);

#ifdef DEBUG_MEM
	void SW_SIT_SetMemoryRefs(void);
#endif


#ifdef __cplusplus
}
#endif

#endif
