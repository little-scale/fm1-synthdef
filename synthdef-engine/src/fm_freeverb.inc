/* SPDX-License-Identifier: GPL-3.0-or-later
 * Adapted from SuperCollider 3.14.1 ReverbUGens.cpp.
 * Copyright (c) 2005 blackrain <blackrain.sc@gmail.com>, GPL-2.0-or-later.
 * Generated with tools/generate-freeverb.py; arithmetic/order retained.
 * All rings, counters and recurrence state live in the caller's float arena.
 */
static __attribute__((noinline)) void render_freeverb(const fm_graph *g,fm_scratch *b,const fm_node *n,float *mem,int count) {
    float* input0 = b->signal[g->node[g->ref[n->input+0].node].output+g->ref[n->input+0].output];
    float* output0 = b->signal[n->output+0];


    float ftemp0 = input(g,b,n,1,0); // mix
    if (ftemp0 > 1.0f)
        ftemp0 = 1.0f;
    if (ftemp0 < 0.0f)
        ftemp0 = 0.0f;
    float ftemp1 = (1 - ftemp0);

    float room = input(g,b,n,2,0); // room
    if (room > 1.0f)
        room = 1.0f;
    if (room < 0.0f)
        room = 0.0f;
    float ftemp5 = (0.700000f + (0.280000f * room));

    float damp = input(g,b,n,3,0); // damp
    if (damp > 1.0f)
        damp = 1.0f;
    if (damp < 0.0f)
        damp = 0.0f;
    float ftemp6 = (0.400000f * damp);
    float ftemp7 = (1 - ftemp6);

    int iota0 = mem[0];
    int iota1 = mem[1];
    int iota2 = mem[2];
    int iota3 = mem[3];
    int iota4 = mem[4];
    int iota5 = mem[5];
    int iota6 = mem[6];
    int iota7 = mem[7];
    int iota8 = mem[8];
    int iota9 = mem[9];
    int iota10 = mem[10];
    int iota11 = mem[11];

    float R0_1 = mem[12];
    float R1_1 = mem[13];
    float R2_1 = mem[14];
    float R3_1 = mem[15];

    float R0_0 = mem[16];
    float R1_0 = mem[17];
    float R2_0 = mem[18];
    float R3_0 = mem[19];
    float R4_0 = mem[20];
    float R5_0 = mem[21];
    float R6_0 = mem[22];
    float R7_0 = mem[23];
    float R8_0 = mem[24];
    float R9_0 = mem[25];
    float R10_0 = mem[26];
    float R11_0 = mem[27];
    float R12_0 = mem[28];
    float R13_0 = mem[29];
    float R14_0 = mem[30];
    float R15_0 = mem[31];
    float R16_0 = mem[32];
    float R17_0 = mem[33];
    float R18_0 = mem[34];
    float R19_0 = mem[35];

    float* dline0 = mem+36;
    float* dline1 = mem+261;
    float* dline2 = mem+602;
    float* dline3 = mem+1043;
    float* dline4 = mem+1599;
    float* dline5 = mem+3216;
    float* dline6 = mem+4773;
    float* dline7 = mem+6264;
    float* dline8 = mem+7686;
    float* dline9 = mem+8963;
    float* dline10 = mem+10079;
    float* dline11 = mem+11267;

    for (int i = 0; i < count; i++) {
        float ftemp2 = input0[i];
        float ftemp4 = (1.500000e-02f * ftemp2);


        if (++iota0 == 225)
            iota0 = 0;
        float T0 = dline0[iota0];

        if (++iota1 == 341)
            iota1 = 0;
        float T1 = dline1[iota1];

        if (++iota2 == 441)
            iota2 = 0;
        float T2 = dline2[iota2];

        if (++iota3 == 556)
            iota3 = 0;
        float T3 = dline3[iota3];


        if (++iota4 == 1617)
            iota4 = 0;
        float T4 = dline4[iota4];
        R5_0 = ((ftemp7 * R4_0) + (ftemp6 * R5_0));
        dline4[iota4] = (ftemp4 + (ftemp5 * R5_0));
        R4_0 = T4;

        if (++iota5 == 1557)
            iota5 = 0;
        float T5 = dline5[iota5];
        R7_0 = ((ftemp7 * R6_0) + (ftemp6 * R7_0));
        dline5[iota5] = (ftemp4 + (ftemp5 * R7_0));
        R6_0 = T5;

        if (++iota6 == 1491)
            iota6 = 0;
        float T6 = dline6[iota6];
        R9_0 = ((ftemp7 * R8_0) + (ftemp6 * R9_0));
        dline6[iota6] = (ftemp4 + (ftemp5 * R9_0));
        R8_0 = T6;

        if (++iota7 == 1422)
            iota7 = 0;
        float T7 = dline7[iota7];
        R11_0 = ((ftemp7 * R10_0) + (ftemp6 * R11_0));
        dline7[iota7] = (ftemp4 + (ftemp5 * R11_0));
        R10_0 = T7;

        if (++iota8 == 1277)
            iota8 = 0;
        float T8 = dline8[iota8];
        R13_0 = ((ftemp7 * R12_0) + (ftemp6 * R13_0));
        dline8[iota8] = (ftemp4 + (ftemp5 * R13_0));
        R12_0 = T8;

        if (++iota9 == 1116)
            iota9 = 0;
        float T9 = dline9[iota9];
        R15_0 = ((ftemp7 * R14_0) + (ftemp6 * R15_0));
        dline9[iota9] = (ftemp4 + (ftemp5 * R15_0));
        R14_0 = T9;

        if (++iota10 == 1188)
            iota10 = 0;
        float T10 = dline10[iota10];
        R17_0 = ((ftemp7 * R16_0) + (ftemp6 * R17_0));
        dline10[iota10] = (ftemp4 + (ftemp5 * R17_0));
        R16_0 = T10;

        if (++iota11 == 1356)
            iota11 = 0;
        float T11 = dline11[iota11];
        R19_0 = ((ftemp7 * R18_0) + (ftemp6 * R19_0));
        dline11[iota11] = (ftemp4 + (ftemp5 * R19_0));
        R18_0 = T11;

        float ftemp8 = (R16_0 + R18_0);

        dline3[iota3] = ((((0.500000f * R3_0) + R4_0) + (R6_0 + R8_0)) + ((R10_0 + R12_0) + (R14_0 + ftemp8)));
        R3_0 = T3;

        R3_1 = (R3_0 - (((R4_0 + R6_0) + (R8_0 + R10_0)) + ((R12_0 + R14_0) + ftemp8)));
        dline2[iota2] = ((0.500000f * R2_0) + R3_1);
        R2_0 = T2;

        R2_1 = (R2_0 - R3_1);
        dline1[iota1] = ((0.500000f * R1_0) + R2_1);
        R1_0 = T1;

        R1_1 = (R1_0 - R2_1);
        dline0[iota0] = ((0.500000f * R0_0) + R1_1);
        R0_0 = T0;

        R0_1 = (R0_0 - R1_1);
        output0[i] = ((ftemp1 * ftemp2) + (ftemp0 * R0_1));
    }

    mem[0] = iota0;
    mem[1] = iota1;
    mem[2] = iota2;
    mem[3] = iota3;
    mem[4] = iota4;
    mem[5] = iota5;
    mem[6] = iota6;
    mem[7] = iota7;
    mem[8] = iota8;
    mem[9] = iota9;
    mem[10] = iota10;
    mem[11] = iota11;

    mem[12] = R0_1;
    mem[13] = R1_1;
    mem[14] = R2_1;
    mem[15] = R3_1;

    mem[16] = R0_0;
    mem[17] = R1_0;
    mem[18] = R2_0;
    mem[19] = R3_0;
    mem[20] = R4_0;
    mem[21] = R5_0;
    mem[22] = R6_0;
    mem[23] = R7_0;
    mem[24] = R8_0;
    mem[25] = R9_0;
    mem[26] = R10_0;
    mem[27] = R11_0;
    mem[28] = R12_0;
    mem[29] = R13_0;
    mem[30] = R14_0;
    mem[31] = R15_0;
    mem[32] = R16_0;
    mem[33] = R17_0;
    mem[34] = R18_0;
    mem[35] = R19_0;

}
static __attribute__((noinline)) void render_freeverb2(const fm_graph *g,fm_scratch *b,const fm_node *n,float *mem,int count) {
    float* input0 = b->signal[g->node[g->ref[n->input+0].node].output+g->ref[n->input+0].output];
    float* input1 = b->signal[g->node[g->ref[n->input+1].node].output+g->ref[n->input+1].output];
    float* output0 = b->signal[n->output+0];
    float* output1 = b->signal[n->output+1];

    float ftemp0 = input(g,b,n,2,0); // mix
    if (ftemp0 > 1.0f)
        ftemp0 = 1.0f;
    if (ftemp0 < 0.0f)
        ftemp0 = 0.0f;
    float ftemp1 = (1 - ftemp0);

    float room = input(g,b,n,3,0); // room
    if (room > 1.0f)
        room = 1.0f;
    if (room < 0.0f)
        room = 0.0f;
    float ftemp5 = (0.700000f + (0.280000f * room));

    float damp = input(g,b,n,4,0); // damp
    if (damp > 1.0f)
        damp = 1.0f;
    if (damp < 0.0f)
        damp = 0.0f;
    float ftemp6 = (0.400000f * damp);
    float ftemp7 = (1 - ftemp6);

    float R0_0 = mem[28];
    float R1_0 = mem[29];
    float R2_0 = mem[30];
    float R3_0 = mem[31];
    float R4_0 = mem[32];
    float R5_0 = mem[33];
    float R6_0 = mem[34];
    float R7_0 = mem[35];
    float R8_0 = mem[36];
    float R9_0 = mem[37];
    float R10_0 = mem[38];
    float R11_0 = mem[39];
    float R12_0 = mem[40];
    float R13_0 = mem[41];
    float R14_0 = mem[42];
    float R15_0 = mem[43];
    float R16_0 = mem[44];
    float R17_0 = mem[45];
    float R18_0 = mem[46];
    float R19_0 = mem[47];
    float R20_0 = mem[48];
    float R21_0 = mem[49];
    float R22_0 = mem[50];
    float R23_0 = mem[51];
    float R24_0 = mem[52];
    float R25_0 = mem[53];
    float R26_0 = mem[54];
    float R27_0 = mem[55];
    float R28_0 = mem[56];
    float R29_0 = mem[57];
    float R30_0 = mem[58];
    float R31_0 = mem[59];
    float R32_0 = mem[60];
    float R33_0 = mem[61];
    float R34_0 = mem[62];
    float R35_0 = mem[63];
    float R36_0 = mem[64];
    float R37_0 = mem[65];
    float R38_0 = mem[66];
    float R39_0 = mem[67];

    float R0_1 = mem[24];
    float R1_1 = mem[25];
    float R2_1 = mem[26];
    float R3_1 = mem[27];

    float R23_1 = mem[71];
    float R22_1 = mem[70];
    float R21_1 = mem[69];
    float R20_1 = mem[68];

    int iota0 = mem[0];
    int iota1 = mem[1];
    int iota2 = mem[2];
    int iota3 = mem[3];
    int iota4 = mem[4];
    int iota5 = mem[5];
    int iota6 = mem[6];
    int iota7 = mem[7];
    int iota8 = mem[8];
    int iota9 = mem[9];
    int iota10 = mem[10];
    int iota11 = mem[11];
    int iota12 = mem[12];
    int iota13 = mem[13];
    int iota14 = mem[14];
    int iota15 = mem[15];
    int iota16 = mem[16];
    int iota17 = mem[17];
    int iota18 = mem[18];
    int iota19 = mem[19];
    int iota20 = mem[20];
    int iota21 = mem[21];
    int iota22 = mem[22];
    int iota23 = mem[23];

    float* dline0 = mem+72;
    float* dline1 = mem+297;
    float* dline2 = mem+638;
    float* dline3 = mem+1079;
    float* dline4 = mem+1635;
    float* dline5 = mem+3252;
    float* dline6 = mem+4809;
    float* dline7 = mem+6300;
    float* dline8 = mem+7722;
    float* dline9 = mem+8999;
    float* dline10 = mem+10115;
    float* dline11 = mem+11303;
    float* dline12 = mem+12659;
    float* dline13 = mem+12907;
    float* dline14 = mem+13271;
    float* dline15 = mem+13735;
    float* dline16 = mem+14314;
    float* dline17 = mem+15954;
    float* dline18 = mem+17534;
    float* dline19 = mem+19048;
    float* dline20 = mem+20493;
    float* dline21 = mem+21793;
    float* dline22 = mem+22932;
    float* dline23 = mem+24143;

    for (int i = 0; i < count; i++) {
        float ftemp2 = input0[i];
        if (++iota0 == 225)
            iota0 = 0;
        float T0 = dline0[iota0];
        if (++iota1 == 341)
            iota1 = 0;
        float T1 = dline1[iota1];
        if (++iota2 == 441)
            iota2 = 0;
        float T2 = dline2[iota2];
        if (++iota3 == 556)
            iota3 = 0;
        float T3 = dline3[iota3];
        if (++iota4 == 1617)
            iota4 = 0;
        float T4 = dline4[iota4];
        float ftemp3 = input1[i];
        float ftemp4 = (1.500000e-02f * (ftemp2 + ftemp3));
        R5_0 = ((ftemp7 * R4_0) + (ftemp6 * R5_0));
        dline4[iota4] = (ftemp4 + (ftemp5 * R5_0));
        R4_0 = T4;
        if (++iota5 == 1557)
            iota5 = 0;
        float T5 = dline5[iota5];
        R7_0 = ((ftemp7 * R6_0) + (ftemp6 * R7_0));
        dline5[iota5] = (ftemp4 + (ftemp5 * R7_0));
        R6_0 = T5;
        if (++iota6 == 1491)
            iota6 = 0;
        float T6 = dline6[iota6];
        R9_0 = ((ftemp7 * R8_0) + (ftemp6 * R9_0));
        dline6[iota6] = (ftemp4 + (ftemp5 * R9_0));
        R8_0 = T6;
        if (++iota7 == 1422)
            iota7 = 0;
        float T7 = dline7[iota7];
        R11_0 = ((ftemp7 * R10_0) + (ftemp6 * R11_0));
        dline7[iota7] = (ftemp4 + (ftemp5 * R11_0));
        R10_0 = T7;
        if (++iota8 == 1277)
            iota8 = 0;
        float T8 = dline8[iota8];
        R13_0 = ((ftemp7 * R12_0) + (ftemp6 * R13_0));
        dline8[iota8] = (ftemp4 + (ftemp5 * R13_0));
        R12_0 = T8;
        if (++iota9 == 1116)
            iota9 = 0;
        float T9 = dline9[iota9];
        R15_0 = ((ftemp7 * R14_0) + (ftemp6 * R15_0));
        dline9[iota9] = (ftemp4 + (ftemp5 * R15_0));
        R14_0 = T9;
        if (++iota10 == 1188)
            iota10 = 0;
        float T10 = dline10[iota10];
        R17_0 = ((ftemp7 * R16_0) + (ftemp6 * R17_0));
        dline10[iota10] = (ftemp4 + (ftemp5 * R17_0));
        R16_0 = T10;
        if (++iota11 == 1356)
            iota11 = 0;
        float T11 = dline11[iota11];
        R19_0 = ((ftemp7 * R18_0) + (ftemp6 * R19_0));
        dline11[iota11] = (ftemp4 + (ftemp5 * R19_0));
        R18_0 = T11;
        float ftemp8 = (R16_0 + R18_0);
        dline3[iota3] = ((((0.500000f * R3_0) + R4_0) + (R6_0 + R8_0)) + ((R10_0 + R12_0) + (R14_0 + ftemp8)));
        R3_0 = T3;
        R3_1 = (R3_0 - (((R4_0 + R6_0) + (R8_0 + R10_0)) + ((R12_0 + R14_0) + ftemp8)));
        dline2[iota2] = ((0.500000f * R2_0) + R3_1);
        R2_0 = T2;
        R2_1 = (R2_0 - R3_1);
        dline1[iota1] = ((0.500000f * R1_0) + R2_1);
        R1_0 = T1;
        R1_1 = (R1_0 - R2_1);
        dline0[iota0] = ((0.500000f * R0_0) + R1_1);
        R0_0 = T0;
        R0_1 = (R0_0 - R1_1);
        output0[i] = ((ftemp1 * ftemp2) + (ftemp0 * R0_1));

        // right chn
        if (++iota12 == 248)
            iota12 = 0;
        float T12 = dline12[iota12];
        if (++iota13 == 364)
            iota13 = 0;
        float T13 = dline13[iota13];
        if (++iota14 == 464)
            iota14 = 0;
        float T14 = dline14[iota14];
        if (++iota15 == 579)
            iota15 = 0;
        float T15 = dline15[iota15];
        if (++iota16 == 1640)
            iota16 = 0;
        float T16 = dline16[iota16];
        R25_0 = ((ftemp7 * R24_0) + (ftemp6 * R25_0));
        dline16[iota16] = (ftemp4 + (ftemp5 * R25_0));
        R24_0 = T16;
        if (++iota17 == 1580)
            iota17 = 0;
        float T17 = dline17[iota17];
        R27_0 = ((ftemp7 * R26_0) + (ftemp6 * R27_0));
        dline17[iota17] = (ftemp4 + (ftemp5 * R27_0));
        R26_0 = T17;
        if (++iota18 == 1514)
            iota18 = 0;
        float T18 = dline18[iota18];
        R29_0 = ((ftemp7 * R28_0) + (ftemp6 * R29_0));
        dline18[iota18] = (ftemp4 + (ftemp5 * R29_0));
        R28_0 = T18;
        if (++iota19 == 1445)
            iota19 = 0;
        float T19 = dline19[iota19];
        R31_0 = ((ftemp7 * R30_0) + (ftemp6 * R31_0));
        dline19[iota19] = (ftemp4 + (ftemp5 * R31_0));
        R30_0 = T19;
        if (++iota20 == 1300)
            iota20 = 0;
        float T20 = dline20[iota20];
        R33_0 = ((ftemp7 * R32_0) + (ftemp6 * R33_0));
        dline20[iota20] = (ftemp4 + (ftemp5 * R33_0));
        R32_0 = T20;
        if (++iota21 == 1139)
            iota21 = 0;
        float T21 = dline21[iota21];
        R35_0 = ((ftemp7 * R34_0) + (ftemp6 * R35_0));
        dline21[iota21] = (ftemp4 + (ftemp5 * R35_0));
        R34_0 = T21;
        if (++iota22 == 1211)
            iota22 = 0;
        float T22 = dline22[iota22];
        R37_0 = ((ftemp7 * R36_0) + (ftemp6 * R37_0));
        dline22[iota22] = (ftemp4 + (ftemp5 * R37_0));
        R36_0 = T22;
        if (++iota23 == 1379)
            iota23 = 0;
        float T23 = dline23[iota23];
        R39_0 = ((ftemp7 * R38_0) + (ftemp6 * R39_0));
        dline23[iota23] = (ftemp4 + (ftemp5 * R39_0));
        R38_0 = T23;
        float ftemp9 = (R36_0 + R38_0);
        dline15[iota15] = ((((0.500000f * R23_0) + R24_0) + (R26_0 + R28_0)) + ((R30_0 + R32_0) + (R34_0 + ftemp9)));
        R23_0 = T15;
        R23_1 = (R23_0 - (((R24_0 + R26_0) + (R28_0 + R30_0)) + ((R32_0 + R34_0) + ftemp9)));
        dline14[iota14] = ((0.500000f * R22_0) + R23_1);
        R22_0 = T14;
        R22_1 = (R22_0 - R23_1);
        dline13[iota13] = ((0.500000f * R21_0) + R22_1);
        R21_0 = T13;
        R21_1 = (R21_0 - R22_1);
        dline12[iota12] = ((0.500000f * R20_0) + R21_1);
        R20_0 = T12;
        R20_1 = (R20_0 - R21_1);
        output1[i] = ((ftemp1 * ftemp3) + (ftemp0 * R20_1));
    }

    mem[0] = iota0;
    mem[1] = iota1;
    mem[2] = iota2;
    mem[3] = iota3;
    mem[4] = iota4;
    mem[5] = iota5;
    mem[6] = iota6;
    mem[7] = iota7;
    mem[8] = iota8;
    mem[9] = iota9;
    mem[10] = iota10;
    mem[11] = iota11;
    mem[12] = iota12;
    mem[13] = iota13;
    mem[14] = iota14;
    mem[15] = iota15;
    mem[16] = iota16;
    mem[17] = iota17;
    mem[18] = iota18;
    mem[19] = iota19;
    mem[20] = iota20;
    mem[21] = iota21;
    mem[22] = iota22;
    mem[23] = iota23;

    mem[24] = R0_1;
    mem[25] = R1_1;
    mem[26] = R2_1;
    mem[27] = R3_1;

    mem[68] = R20_1;
    mem[69] = R21_1;
    mem[70] = R22_1;
    mem[71] = R23_1;

    mem[28] = R0_0;
    mem[29] = R1_0;
    mem[30] = R2_0;
    mem[31] = R3_0;
    mem[32] = R4_0;
    mem[33] = R5_0;
    mem[34] = R6_0;
    mem[35] = R7_0;
    mem[36] = R8_0;
    mem[37] = R9_0;
    mem[38] = R10_0;
    mem[39] = R11_0;
    mem[40] = R12_0;
    mem[41] = R13_0;
    mem[42] = R14_0;
    mem[43] = R15_0;
    mem[44] = R16_0;
    mem[45] = R17_0;
    mem[46] = R18_0;
    mem[47] = R19_0;
    mem[48] = R20_0;
    mem[49] = R21_0;
    mem[50] = R22_0;
    mem[51] = R23_0;
    mem[52] = R24_0;
    mem[53] = R25_0;
    mem[54] = R26_0;
    mem[55] = R27_0;
    mem[56] = R28_0;
    mem[57] = R29_0;
    mem[58] = R30_0;
    mem[59] = R31_0;
    mem[60] = R32_0;
    mem[61] = R33_0;
    mem[62] = R34_0;
    mem[63] = R35_0;
    mem[64] = R36_0;
    mem[65] = R37_0;
    mem[66] = R38_0;
    mem[67] = R39_0;

}
