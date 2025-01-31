/*
 *
 *  ENVIRONMENTAL MODELLING SUITE (EMS)
 *  
 *  File: model/lib/ecology/process_library/dinoflagellate_diel_mortality_sed.c
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
 *  $Id: dinoflagellate_diel_mortality_sed.c 5846 2018-06-29 04:14:26Z riz008 $
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
#include "dinoflagellate_diel_mortality_sed.h"

typedef struct {
    int do_mb;                  /* flag */

    /*
     * parameters
     */
    double mL_t0;

    /*
     * tracers
     */
    int Phy_N_i;
    int Phy_NR_i;
    int Phy_I_i;
    int DIC_i;
    int DIP_i;
    int NH4_i;
    int NH4_pr_i;
    int DetPL_N_i;
    int TN_i;
    int TP_i;
    int TC_i;

    /*
     * common cell variables
     */
    int Tfactor_i;
    int mL_i;
} workspace;

void dinoflagellate_diel_mortality_sed_init(eprocess* p)
{
    ecology* e = p->ecology;
    stringtable* tracers = e->tracers;
    workspace* ws = malloc(sizeof(workspace));

    p->workspace = ws;

    /*
     * parameters
     */
    ws->mL_t0 = get_parameter_value(e, "PD_mL");

    /*
     * tracers
     */
    ws->Phy_N_i = e->find_index(tracers, "PhyD_N", e);
    ws->Phy_NR_i = e->find_index(tracers, "PhyD_NR", e);
    ws->Phy_I_i = e->find_index(tracers, "PhyD_I", e);
    ws->NH4_i = e->find_index(tracers, "NH4", e);
    ws->NH4_pr_i = e->find_index(tracers, "NH4_pr", e);
    ws->DIC_i = e->find_index(tracers, "DIC", e);
    ws->DIP_i = e->find_index(tracers, "DIP", e);
    ws->DetPL_N_i = e->find_index(tracers, "DetPL_N", e);
    ws->TN_i = e->find_index(tracers, "TN", e);
    ws->TP_i = e->find_index(tracers, "TP", e);
    ws->TC_i = e->find_index(tracers, "TC", e);

    /*
     * common cell variables
     */
    ws->Tfactor_i = try_index(e->cv_cell, "Tfactor", e);
    ws->mL_i = find_index_or_add(e->cv_cell, "PD_mL", e);
}

void dinoflagellate_diel_mortality_sed_postinit(eprocess* p)
{
    ecology* e = p->ecology;
    workspace* ws = (workspace*) p->workspace;

    ws->do_mb = (try_index(e->cv_model, "massbalance_sed", e) >= 0) ? 1 : 0;
}

void dinoflagellate_diel_mortality_sed_destroy(eprocess* p)
{
    free(p->workspace);
}

void dinoflagellate_diel_mortality_sed_precalc(eprocess* p, void* pp)
{
    workspace* ws = p->workspace;
    cell* c = (cell*) pp;
    double* cv = c->cv;
    double* y = c->y;

    double Tfactor = (ws->Tfactor_i >= 0) ? cv[ws->Tfactor_i] : 1.0;

    cv[ws->mL_i] = ws->mL_t0 * Tfactor;

    if (ws->do_mb) {
        double Phy_N = y[ws->Phy_N_i];
        double Phy_NR = y[ws->Phy_NR_i];

        y[ws->TN_i] += Phy_N + Phy_NR;
        y[ws->TP_i] += (Phy_N + Phy_NR) * red_W_P;
        y[ws->TC_i] += Phy_N * red_W_C;
    }
}

void dinoflagellate_diel_mortality_sed_calc(eprocess* p, void* pp)
{
    workspace* ws = p->workspace;
    intargs* ia = (intargs*) pp;
    cell* c = (cell*) ia->media;
    double* cv = c->cv;
    double* y = ia->y;
    double* y1 = ia->y1;

    double Phy_N = y[ws->Phy_N_i];
    double Phy_I = y[ws->Phy_I_i];
    double Phy_NR = y[ws->Phy_NR_i];
    double mL = cv[ws->mL_i];
    double mortality1 = Phy_N * mL;
    double mortality2 = Phy_NR * mL;
    double mortality3 = Phy_I * mL;
    double porosity = c->porosity;

 /* Phy_N mortality at Redfield but Phy_NR is excess nutrients so they must go into individual pools */
    y1[ws->Phy_N_i] -= mortality1;
    y1[ws->Phy_NR_i] -= mortality2;
    y1[ws->Phy_I_i] -= mortality3;
    y1[ws->DetPL_N_i] += mortality1 ;
    y1[ws->NH4_i] += mortality2 / porosity ;
    y1[ws->NH4_pr_i] += mortality2 * SEC_PER_DAY * c->dz_sed * porosity;
    y1[ws->DIP_i] += mortality2 * red_W_P / porosity;

 /*   y1[ws->Phy_N_i] -= mortality1;
    y1[ws->DetPL_N_i] += mortality1;
    y1[ws->DIC_i] += (Phy_C - Phy_N * red_W_C) * mL / porosity;
    y1[ws->Phy_C_i] -= Phy_C * mL; */
}

void dinoflagellate_diel_mortality_sed_postcalc(eprocess* p, void* pp)
{
    cell* c = ((cell*) pp);
    workspace* ws = p->workspace;
    double* y = c->y;

    double Phy_N = y[ws->Phy_N_i];
    double Phy_NR = y[ws->Phy_NR_i];

    y[ws->TN_i] += Phy_N + Phy_NR;
    y[ws->TP_i] += (Phy_N + Phy_NR) * red_W_P;
    y[ws->TC_i] += Phy_N * red_W_C;
}
