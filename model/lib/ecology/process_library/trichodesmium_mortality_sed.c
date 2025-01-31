/*
 *
 *  ENVIRONMENTAL MODELLING SUITE (EMS)
 *  
 *  File: model/lib/ecology/process_library/trichodesmium_mortality_sed.c
 *  
 *  Description:
 *  Process implementation
 *  
 *  Copyright:
 *  Copyright (c) 2018. Commonwealth Scientific and Industrial
 *  Research Organisation (CSIRO). ABN 41 687 119 230. All rights
 *  reserved. See the license file for disclaimer and full
 *  use/redistribution conditions.
 *  
 *  $Id: trichodesmium_mortality_sed.c 5935 2018-09-12 04:59:19Z bai155 $
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "ecology_internal.h"
#include "utils.h"
#include "ecofunct.h"
#include "constants.h"
#include "eprocess.h"
#include "cell.h"
#include "column.h"
#include "einterface.h"
#include "trichodesmium_mortality_sed.h"

typedef struct {
  int do_mb;                  /* flag */
  
  /*
   * parameters
   */
  double mL_t0;
  double KO_aer;
  
  /*
   * tracers
   */
  int Tricho_N_i;
  int Tricho_NR_i;
  int Tricho_I_i;
  int DIC_i;
  int DIP_i;
  int NH4_i;
  int NH4_pr_i;
  int DetPL_N_i;
  int Oxygen_i;
  int TN_i;
  int TP_i;
  int TC_i;
  int BOD_i;
  int COD_i;
  
  int Tricho_PR_i;
  int Tricho_Chl_i;
  
  /*
   * common cell variables
   */
  int Tfactor_i;
  int mL_i;
} workspace;

void trichodesmium_mortality_sed_init(eprocess* p)
{
  ecology* e = p->ecology;
  stringtable* tracers = e->tracers;
  workspace* ws = malloc(sizeof(workspace));
  
  p->workspace = ws;
  
  /*
   * parameters
   */

  ws->mL_t0 = try_parameter_value(e, "Tricho_mL_sed");
  if (isnan(ws->mL_t0)){
    ws->mL_t0 = get_parameter_value(e, "Tricho_mL");
    eco_write_setup(e,"Could not find Tricho_mL_sed, so used Tricho_mL = %e \n",ws->mL_t0);
  }
  ws->KO_aer = get_parameter_value(e, "KO_aer");
  
  /*
   * tracers
   */
  ws->Tricho_N_i = e->find_index(tracers, "Tricho_N", e);
  ws->Tricho_NR_i = e->find_index(tracers, "Tricho_NR", e);
  ws->Tricho_I_i = e->find_index(tracers, "Tricho_I", e);
  ws->NH4_i = e->find_index(tracers, "NH4", e);
  ws->DIC_i = e->find_index(tracers, "DIC", e);
  ws->DIP_i = e->find_index(tracers, "DIP", e);
  ws->DetPL_N_i = e->find_index(tracers, "DetPL_N", e);
  ws->Oxygen_i = e->find_index(tracers, "Oxygen", e);
  ws->TN_i = e->find_index(tracers, "TN", e);
  ws->TP_i = e->find_index(tracers, "TP", e);
  ws->TC_i = e->find_index(tracers, "TC", e);
  ws->BOD_i = e->find_index(tracers, "BOD", e);
  ws->COD_i = e->try_index(tracers, "COD", e);
  
  ws->Tricho_PR_i = e->find_index(tracers, "Tricho_PR", e);
  ws->Tricho_Chl_i = e->find_index(tracers, "Tricho_Chl", e);
  
  /* diagnostic variable */

  ws->NH4_pr_i = e->try_index(tracers, "NH4_pr", e);

  /*
   * common cell variables
   */

  ws->Tfactor_i = try_index(e->cv_cell, "Tfactor", e);
  ws->mL_i = find_index_or_add(e->cv_cell, "PD_mL", e);
}

void trichodesmium_mortality_sed_postinit(eprocess* p)
{
  ecology* e = p->ecology;
  workspace* ws = (workspace*) p->workspace;
  
  ws->do_mb = (try_index(e->cv_model, "massbalance_sed", e) >= 0) ? 1 : 0;
}

void trichodesmium_mortality_sed_destroy(eprocess* p)
{
  free(p->workspace);
}

void trichodesmium_mortality_sed_precalc(eprocess* p, void* pp)
{
  workspace* ws = p->workspace;
  cell* c = (cell*) pp;
  double* cv = c->cv;
  double* y = c->y;
  
  double Tfactor = (ws->Tfactor_i >= 0) ? cv[ws->Tfactor_i] : 1.0;
  
  cv[ws->mL_i] = ws->mL_t0 * Tfactor;
  
  if (ws->do_mb) {
    double Tricho_N = y[ws->Tricho_N_i];
    double Tricho_NR = y[ws->Tricho_NR_i];
    double Tricho_PR = y[ws->Tricho_PR_i];
    double Tricho_I = y[ws->Tricho_I_i];
 
    y[ws->TN_i] += Tricho_N + Tricho_NR;
    y[ws->TP_i] += Tricho_N * red_W_P + Tricho_PR;
    y[ws->TC_i] += Tricho_N * red_W_C + Tricho_I*106.0/1060.0*12.01;
    y[ws->BOD_i] += (Tricho_N * red_W_C + Tricho_I*106.0/1060.0*12.01)*C_O_W;
  }
}

void trichodesmium_mortality_sed_calc(eprocess* p, void* pp)
{
  workspace* ws = p->workspace;
  intargs* ia = (intargs*) pp;
  cell* c = (cell*) ia->media;
  double* cv = c->cv;
  double* y = ia->y;
  double* y1 = ia->y1;
  
  double porosity = c->porosity;
  double Tricho_N = y[ws->Tricho_N_i];
  double Tricho_NR = y[ws->Tricho_NR_i];
  double Tricho_PR = y[ws->Tricho_PR_i];
  double Tricho_I = y[ws->Tricho_I_i];
  double Tricho_Chl = y[ws->Tricho_Chl_i];
  double Oxygen = e_max(y[ws->Oxygen_i]);
  double mortality1 = Tricho_N * cv[ws->mL_i];
  double mortality2 = Tricho_NR * cv[ws->mL_i];
  double mortality3 = Tricho_I * cv[ws->mL_i];
  double mortality4 = Tricho_PR * cv[ws->mL_i];
  
  /* Structural material (Tricho_N) released at Redfield to DetPL, but all 
     reserves must go to individual pools (NR, PR), or lost (I, Chl)  */
  
  y1[ws->Tricho_N_i] -= mortality1;
  y1[ws->Tricho_NR_i] -= mortality2;
  y1[ws->Tricho_I_i] -= mortality3;
  y1[ws->Tricho_PR_i] -= mortality4;
  y1[ws->Tricho_Chl_i] -= Tricho_Chl * cv[ws->mL_i];
  
  y1[ws->DetPL_N_i] += mortality1 ;
  y1[ws->NH4_i] += mortality2 / porosity ;
  y1[ws->DIP_i] += mortality4 / porosity;
  y1[ws->DIC_i] += mortality3 * 106.0/1060.0*12.01 / porosity;

  double Oxygen2 = Oxygen * Oxygen; 
  double sigmoid = Oxygen2 / (ws->KO_aer * ws->KO_aer + Oxygen2 );

  y1[ws->Oxygen_i] -= mortality3 * 106.0/1060.0*12.01 * C_O_W * sigmoid / porosity;

  if (ws->COD_i > -1){
    y1[ws->COD_i] += mortality3 * 106.0/1060.0*12.01 * C_O_W * (1.0 - sigmoid) / porosity;
  }
  
  /* diagnostic tracer */

  if (ws->NH4_pr_i > -1)
    y1[ws->NH4_pr_i] += mortality2 * SEC_PER_DAY * c->dz_sed * porosity;
}

void trichodesmium_mortality_sed_postcalc(eprocess* p, void* pp)
{
  cell* c = ((cell*) pp);
  workspace* ws = p->workspace;
  double* y = c->y;
  
  double Tricho_N = y[ws->Tricho_N_i];
  double Tricho_NR = y[ws->Tricho_NR_i];
  double Tricho_PR = y[ws->Tricho_PR_i];
  double Tricho_I = y[ws->Tricho_I_i];
  
  y[ws->TN_i] += Tricho_N + Tricho_NR;
  y[ws->TP_i] += Tricho_N * red_W_P + Tricho_PR;
  y[ws->TC_i] += Tricho_N * red_W_C + Tricho_I*106.0/1060.0*12.01;
  y[ws->BOD_i] += (Tricho_N * red_W_C + Tricho_I*106.0/1060.0*12.01)*C_O_W;
}
