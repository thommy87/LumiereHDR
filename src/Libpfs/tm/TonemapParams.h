/*
 * This file is a part of LumiereHDR package.
 * ----------------------------------------------------------------------
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * ----------------------------------------------------------------------
 */

//! rief The numbers a tone mapping operator works with, in plain C++ so
//! the calculation core needs no Qt (TonemappingOptions adds the texts)

#ifndef PFS_TM_TONEMAPPARAMS_H
#define PFS_TM_TONEMAPPARAMS_H

//----------------- DO NOT CHANGE ENUMERATION ORDER -----------------------
// all is used by SavedParametersDialog to select comments from all operators
enum TMOperator : unsigned short {
    mantiuk06 = 0,
    mantiuk08 = 1,
    fattal = 2,
    ferradans = 3,
    drago = 4,
    durand = 5,
    reinhard02 = 6,
    reinhard05 = 7,
    ashikhmin = 8,
    pattanaik = 9,
    mai = 10,
    ferwerda = 11,
    kimkautz = 12,
    vanhateren = 13,
    lischinski = 14
};

struct TonemapParams {
    int origxsize;      // this parameter should be coming from the UI
    int xsize_percent;  // this parameter should be coming from the UI
    int xsize;          // this parameter should be coming from the frame
    int quality;
    float pregamma;
    float postgamma;
    float postsaturation;
    // applied to the result of every operator alike
    float postbrightness;  // exposure change in stops, 0 = none
    float postcontrast;    // -1 (flat) .. 0 (none) .. +1 (strong)
    float postwarmth;      // -1 (cool) .. 0 (none) .. +1 (warm)
    bool tonemapSelection;  // we should let do this thing to the tonemapping
                            // thread
    TMOperator tmoperator;
    struct {
        struct {
            bool simple;
            bool eq2;  // false means eq4
            float lct;
        } ashikhminoptions;
        struct {
            float bias;
        } dragooptions;
        struct {
            float spatial;
            float range;
            float base;
        } durandoptions;
        struct {
            float alpha;
            float beta;
            float color;
            float noiseredux;
            bool newfattal;
            bool fftsolver;
        } fattaloptions;
        struct {
            float rho;
            float inv_alpha;
        } ferradansoptions;
        struct {
            bool autolum;
            bool local;
            float cone;
            float rod;
            float multiplier;
        } pattanaikoptions;
        struct {
            bool scales;
            float key;
            float phi;
            int range;
            int lower;
            int upper;
        } reinhard02options;
        struct {
            float brightness;
            float chromaticAdaptation;
            float lightAdaptation;
        } reinhard05options;
        struct {
            float contrastfactor;
            float saturationfactor;
            float detailfactor;
            bool contrastequalization;
        } mantiuk06options;
        struct {
            float colorsaturation;
            float contrastenhancement;
            float luminancelevel;
            bool setluminance;
        } mantiuk08options;
        struct {
            float multiplier;
            float adaptationluminance;
        } ferwerdaoptions;
        struct {
            float c1;
            float c2;
        } kimkautzoptions;
        struct {
            float pupil_area;
        } vanhaterenoptions;
        struct {
            float alpha;
        } lischinskioptions;
    } operator_options;

    // Davide Anastasia <davideanastasia@users.sourceforge.net>
    // Adding the coordinates of the crop inside this structure will allow
    // TMOThread
    // to crop itself the region of interest, keeping the code tight and simple
    // and avoiding useless copy in memory of the frame to be processed.
    int selection_x_up_left;
    int selection_y_up_left;
    int selection_x_bottom_right;
    int selection_y_bottom_right;
};

#endif  // PFS_TM_TONEMAPPARAMS_H
