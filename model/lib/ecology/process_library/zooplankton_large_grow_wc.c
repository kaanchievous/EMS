/*
 *
 *  ENVIRONMENTAL MODELLING SUITE (EMS)
 *  
 *  File: model/lib/ecology/process_library/zooplankton_large_grow_wc.c
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
 *  $Id: zooplankton_large_grow_wc.c 6703 2021-03-24 01:16:20Z wil00y $
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "ecology_internal.h"
#include "utils.h"
#include "ecofunct.h"
#include "constants.h"
#include "eprocess.h"
#include "cell.h"
#include "column.h"
#include "einterface.h"
#include "zooplankton_large_grow_wc.h"

typedef struct {
    int do_mb;                  /* flag */
    int with_df;                /* flag */

    /*
     * parameters
     */
    double umax_t0;
    double rad;
    char* meth;
    double swim_t0;
    double TKEeps;
    double MBrad;
    double PLrad;
    double DFrad;
    double m;
    double E;
    double FDG;
    double KO_aer;

    /*
     * tracers
     */
    int ZooL_N_i;
    int PhyL_N_i;
    int PhyD_N_i;
    int PhyD_C_i;
    int MPB_N_i;
    int NH4_i;
    int DetPL_N_i;
    int DIC_i;
    int Oxygen_i;
    int DIP_i;
    int temp_i;
    int salt_i;
    int ZooL_N_gr_i;
    int ZooL_N_rm_i;
    int NH4_pr_i;
    int Oxy_pr_i;
    int TN_i;
    int TP_i;
    int TC_i;

    /*
     * common cell variables
     */
    int Tfactor_i;
    int vis_i;
    int umax_i;
    int phi_MB_ZL_i;
    int phi_PL_ZL_i;
    int phi_DF_ZL_i;
} workspace;

void zooplankton_large_grow_wc_init(eprocess* p)
{
    ecology* e = p->ecology;
    stringtable* tracers = e->tracers;
    workspace* ws = malloc(sizeof(workspace));

    p->workspace = ws;

    /*
     * tracers
     */

    ws->ZooL_N_i = e->find_index(tracers, "ZooL_N", e);
    ws->PhyL_N_i = e->find_index(tracers, "PhyL_N", e);

    ws->PhyD_N_i = e->try_index(tracers, "PhyD_N", e);
    ws->PhyD_C_i = e->try_index(tracers, "PhyD_C", e);

    /*UR temp
    ws->PhyD_N_i = ecology_try_tracerindex(e,tracers,"PhyD_N");
    ws->PhyD_C_i = ecology_try_tracerindex(e,tracers,"PhyD_C");*/

    ws->MPB_N_i = e->find_index(tracers, "MPB_N", e);
    ws->NH4_i = e->find_index(tracers, "NH4", e);
    ws->DetPL_N_i = e->find_index(tracers, "DetPL_N", e);
    ws->DIC_i = e->find_index(tracers, "DIC", e);
    ws->Oxygen_i = e->find_index(tracers, "Oxygen", e);
    ws->DIP_i = e->find_index(tracers, "DIP", e);
    ws->temp_i = e->find_index(tracers, "temp", e);
    ws->salt_i = e->find_index(tracers, "salt", e);
    ws->ZooL_N_gr_i = e->find_index(tracers, "ZooL_N_gr", e);
    ws->ZooL_N_rm_i = e->find_index(tracers, "ZooL_N_rm", e);
    ws->Oxy_pr_i = e->find_index(tracers, "Oxy_pr", e);
    ws->NH4_pr_i = e->find_index(tracers, "NH4_pr", e);
    ws->TN_i = e->find_index(tracers, "TN", e);
    ws->TP_i = e->find_index(tracers, "TP", e);
    ws->TC_i = e->find_index(tracers, "TC", e);


    if (ws->PhyD_N_i >= 0 && ws->PhyD_C_i >= 0)
        ws->with_df = 1;
    else
        ws->with_df = 0;
    /*
     * parameters
     */
    ws->umax_t0 = get_parameter_value(e, "ZLumax");
    ws->rad = get_parameter_value(e, "ZLrad");
    ws->meth = get_parameter_stringvalue(e, "ZLmeth");
    ws->swim_t0 = get_parameter_value(e, "ZLswim");
    ws->TKEeps = get_parameter_value(e, "TKEeps");
    ws->PLrad = get_parameter_value(e, "PLrad");
    ws->MBrad = get_parameter_value(e, "MBrad");
    if (ws->with_df)
        ws->DFrad = get_parameter_value(e, "DFrad");
    else
        ws->DFrad = try_parameter_value(e, "DFrad");
    ws->m = try_parameter_value(e, "ZLm");
    if (isnan(ws->m))
        ws->m = ZooCellMass(ws->rad);
    ws->E = get_parameter_value(e, "ZL_E");
    ws->FDG = get_parameter_value(e, "ZL_FDG");
    ws->KO_aer = get_parameter_value(e, "KO_aer");

    /*
     * common cell variables
     */
    ws->Tfactor_i = try_index(e->cv_cell, "Tfactor", e);
    ws->vis_i = find_index(e->cv_cell, "viscosity", e);
    ws->umax_i = find_index_or_add(e->cv_cell, "ZLumax", e);
    ws->phi_MB_ZL_i = find_index_or_add(e->cv_cell, "phi_MB_ZL", e);
    ws->phi_PL_ZL_i = find_index_or_add(e->cv_cell, "phi_PL_ZL", e);
    ws->phi_DF_ZL_i = find_index_or_add(e->cv_cell, "phi_DF_ZL", e);
}

void zooplankton_large_grow_wc_postinit(eprocess* p)
{
    ecology* e = p->ecology;
    workspace* ws = p->workspace;

    ws->do_mb = (try_index(e->cv_model, "massbalance_wc", e) >= 0) ? 1 : 0;

    /*UR 10/6/05 since we have a dependancy of the PhyD_N and PhyD_C
     * we also have to make sure dinoflagellate growth is actually calculated
     *
     */
    if(ws->with_df)
      ws->with_df = process_present(e,PT_WC,"dinoflagellate_grow_wc");
    emstag(LINFO,"eco:zooplankton_large_grow_wc:postinit","%sCalculating consumption of Dinoflagellates",(ws->with_df?"":"NOT "));
}

void zooplankton_large_grow_wc_destroy(eprocess* p)
{
    free(p->workspace);
}

void zooplankton_large_grow_wc_precalc(eprocess* p, void* pp)
{
    workspace* ws = p->workspace;
    cell* c = (cell*) pp;
    double* y = c->y;
    double* cv = c->cv;
    double temp = y[ws->temp_i];
    double vis = cv[ws->vis_i];
    double swim = ws->swim_t0;

    double Tfactor = (ws->Tfactor_i >= 0) ? cv[ws->Tfactor_i] : 1.0;

    cv[ws->umax_i] = ws->umax_t0 * Tfactor;
    swim *= Tfactor;

    cv[ws->phi_PL_ZL_i] = phi(ws->meth, ws->PLrad, 0.004 * pow(ws->PLrad, 0.26), 0.0, ws->rad, swim, 0.0, ws->TKEeps, vis, 1000.0, temp);
    cv[ws->phi_MB_ZL_i] = phi(ws->meth, ws->MBrad, 0.004 * pow(ws->MBrad, 0.26), 0.0, ws->rad, swim, 0.0, ws->TKEeps, vis, 1000.0, temp);
    cv[ws->phi_DF_ZL_i] = (ws->with_df) ? phi(ws->meth, ws->DFrad, 0.004 * pow(ws->DFrad, 0.26), 0.0, ws->rad, swim, 0.0, ws->TKEeps, vis, 1000.0, temp) : 0.0;

    if (ws->do_mb) {
        double ZooL_N = y[ws->ZooL_N_i];

        y[ws->TN_i] += ZooL_N;
        y[ws->TP_i] += ZooL_N * red_W_P;
        y[ws->TC_i] += ZooL_N * red_W_C;
    }
}

void zooplankton_large_grow_wc_calc(eprocess* p, void* pp)
{
    workspace* ws = p->workspace;
    intargs* ia = (intargs*) pp;
    cell* c = ((cell*) ia->media);
    double* cv = c->cv;
    double* y = ia->y;
    double* y1 = ia->y1;

    double ZooL_N = y[ws->ZooL_N_i];
    double PhyL_N = y[ws->PhyL_N_i];
    double PhyD_N = (ws->with_df) ? y[ws->PhyD_N_i] : 0.0;
    double PhyD_C = (ws->with_df) ? y[ws->PhyD_C_i] : 0.0;
    double MPB_N = y[ws->MPB_N_i];
    double Oxygen = y[ws->Oxygen_i];
    double cells = ZooL_N * mgN2molN / ws->m / red_A_N;
    double phi_PL_ZL = cv[ws->phi_PL_ZL_i];
    double phi_MB_ZL = cv[ws->phi_MB_ZL_i];
    double phi_DF_ZL = cv[ws->phi_DF_ZL_i];
    double max_enc = PhyL_N * phi_PL_ZL + MPB_N * phi_MB_ZL + PhyD_N * phi_DF_ZL;
    double umax = cv[ws->umax_i];
    double max_ing = umax * ws->m * red_A_N / mgN2molN / ws->E;
    double graze = cells * e_min(max_ing, max_enc);
    /*UR Changed order, strict compiler wants declarations first
     */
    double NH4_release ;
    double growth ;
    double DF_graze ;
    double DIC_release ;
    double Oxy_pr;

    if (graze == 0.0)
        max_enc = 1.0;

    NH4_release = graze * (1.0 - ws->E) * (1.0 - ws->FDG);
    growth = graze * ws->E;
    DF_graze = (ws->with_df) ? graze * PhyD_N * phi_DF_ZL / max_enc : 0.0;
    DIC_release = (ws->with_df) ? NH4_release * red_W_C + DF_graze * (PhyD_C / e_max(PhyD_N) - red_W_C) : NH4_release * red_W_C;
    Oxy_pr = -DIC_release * red_W_O / red_W_C * Oxygen / (ws->KO_aer + e_max(Oxygen));

    /*
     * This happens all the time
     */
    /*
     * if (PhyD_C / PhyD_N - red_W_C < 0.0)
     * e->quitfn("ecology: error: PhyD_C / PhyD_N - red_W_C < 0\n", e->verbose);
     */

    y1[ws->ZooL_N_i] += growth;
    y1[ws->PhyL_N_i] -= graze * PhyL_N * phi_PL_ZL / max_enc;

    if (ws->with_df) {
        y1[ws->PhyD_N_i] -= DF_graze;
        y1[ws->PhyD_C_i] -= DF_graze * PhyD_C / e_max(PhyD_N);
    }

    y1[ws->MPB_N_i] -= graze * MPB_N * phi_MB_ZL / max_enc;
    y1[ws->NH4_i] += NH4_release;
    y1[ws->DIP_i] += NH4_release * red_W_P;
    y1[ws->DIC_i] += DIC_release;
    y1[ws->Oxygen_i] += Oxy_pr;
    y1[ws->Oxy_pr_i] += Oxy_pr * SEC_PER_DAY;
    y1[ws->DetPL_N_i] += graze * (1.0 - ws->E) * ws->FDG;
    y1[ws->NH4_pr_i] += NH4_release * SEC_PER_DAY;
    y1[ws->ZooL_N_rm_i] += graze * red_W_C * SEC_PER_DAY;
    y1[ws->ZooL_N_gr_i] += growth * SEC_PER_DAY;

}

void zooplankton_large_grow_wc_postcalc(eprocess* p, void* pp)
{
    cell* c = ((cell*) pp);
    workspace* ws = p->workspace;
    double* y = c->y;

    double ZooL_N = y[ws->ZooL_N_i];

    y[ws->TN_i] += ZooL_N;
    y[ws->TP_i] += ZooL_N * red_W_P;
    y[ws->TC_i] += ZooL_N * red_W_C;
}
