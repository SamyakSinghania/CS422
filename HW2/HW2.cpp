/*
 * Copyright (C) 2007-2023 Intel Corporation.
 * SPDX-License-Identifier: MIT
 */

/*! @file
 *  This is an example of the PIN tool that demonstrates some basic PIN APIs 
 *  and could serve as the starting point for developing your first PIN tool
 */

#include <iostream>
#include <fstream>
#include <set>
#include "pin.H"
#include <cstdlib>
#include <vector>
#include <cmath>
//#include <bits/stdc++.h>
using std::cerr;
using std::endl;
using std::string;

#define BILLION 1000000000

/* ================================================================== */
// Global variables
/* ================================================================== */

UINT64 insCount    = 0; //number of dynamically executed instructions
UINT64 bblCount    = 0; //number of dynamically executed basic blocks
UINT64 threadCount = 0; //total number of threads, including main thread

std::ostream* out = &cerr;

/* ===================================================================== */
// Command line switches
/* ===================================================================== */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "", "specify file name for HW2 output");
KNOB<UINT64> KnobFastForward(KNOB_MODE_WRITEONCE, "pintool", "f", "0", "number of instructions to fast forward in billions");
/* ===================================================================== */
// Utilities
/* ===================================================================== */
UINT64 icount = 0; //number of dynamically executed instructions

UINT64 fastForwardIns; // number of instruction to fast forward
UINT64 maxIns; // maximum number of instructions to simulate


//Variables to measure correctness of predictions
UINT64 static_correct = 0;
UINT64 static_forward_correct = 0;
UINT64 static_backward_correct = 0;

UINT64 bimodal_correct = 0;
UINT64 bimodal_forward_correct = 0;
UINT64 bimodal_backward_correct = 0;

UINT64 SAg_correct = 0;
UINT64 SAg_forward_correct = 0;
UINT64 SAg_backward_correct = 0;

UINT64 GAg_correct = 0;
UINT64 GAg_forward_correct = 0;
UINT64 GAg_backward_correct = 0;

UINT64 gshare_correct = 0;
UINT64 gshare_forward_correct = 0;
UINT64 gshare_backward_correct = 0;

UINT64 Meta_SAg_correct = 0;
UINT64 Meta_SAg_forward_correct = 0;
UINT64 Meta_SAg_backward_correct = 0;

UINT64 Meta_GAg_correct = 0;
UINT64 Meta_GAg_forward_correct = 0;
UINT64 Meta_GAg_backward_correct = 0;

UINT64 Meta_gshare_correct = 0;
UINT64 Meta_gshare_forward_correct = 0;
UINT64 Meta_gshare_backward_correct = 0;

UINT64 Hybrid_correct = 0;
UINT64 Hybrid_forward_correct = 0;
UINT64 Hybrid_backward_correct = 0;

UINT64 Hybrid21_correct = 0;
UINT64 Hybrid21_forward_correct = 0;
UINT64 Hybrid21_backward_correct = 0;

UINT64 Hybrid22_correct = 0;
UINT64 Hybrid22_forward_correct = 0;
UINT64 Hybrid22_backward_correct = 0;

UINT64 branch_count = 0;
UINT64 forward_branch_count = 0;
UINT64 backward_branch_count = 0;

UINT64 indirect_count = 0;
UINT64 misses_BTB1 = 0;
UINT64 BTB1_correct_count = 0;
UINT64 misses_BTB2 = 0;
UINT64 BTB2_correct_count = 0;
UINT64 time_BTB = 0;

//BTB entry for a particular way inside a set
struct cache_entry{
    UINT64 tag;
    UINT64 target;
    UINT64 LRU = 0;
    bool valid = 0;
};

//predictor sizes are defined here and can be modified at will
const int bimodal_PHT_rows = 512;
const int bimodal_PHT_width = 2;

const int SAg_PHT_rows = 512;
const int SAg_PHT_width = 2;
const int SAg_BHT_rows = 1024;
const int SAg_BHT_width = 9;

const int GHR_rows = 1;
const int GHR_width = 9;

const int GAg_PHT_rows = 512;
const int GAg_PHT_width = 3;

const int gshare_PHT_rows = 512;
const int gshare_PHT_width = 3;

const int Meta_SAg_PHT_rows = 512;
const int Meta_SAg_PHT_width = 2;
const int Meta_SAg_BHT_rows = 1024;
const int Meta_SAg_BHT_width = 9;

const int Meta_GAg_PHT_rows = 512;
const int Meta_GAg_PHT_width = 3;

const int Meta_gshare_PHT_rows = 512;
const int Meta_gshare_PHT_width = 3;

const int Meta_SAg_GAg_rows = 512;
const int Meta_SAg_GAg_width = 2;

const int Meta_SAg_gshare_rows = 512;
const int Meta_SAg_gshare_width = 2;

const int Meta_GAg_gshare_rows = 512;
const int Meta_GAg_gshare_width = 2;

const int BTB_sets = 128;
const int log_BTB_sets = static_cast<int>(log2(BTB_sets));
const int BTB_ways = 4;

//All the predictor tables
std::vector<std::vector<bool>> bimodal_PHT(bimodal_PHT_rows,std::vector<bool>(bimodal_PHT_width,0));
std::vector<std::vector<bool>> SAg_PHT(SAg_PHT_rows,std::vector<bool>(SAg_PHT_width,0));
std::vector<std::vector<bool>> SAg_BHT(SAg_BHT_rows,std::vector<bool>(SAg_BHT_width,0));
std::vector<std::vector<bool>> GHR(GHR_rows,std::vector<bool>(GHR_width,0));
std::vector<std::vector<bool>> GAg_PHT(GAg_PHT_rows,std::vector<bool>(GAg_PHT_width,0));
std::vector<std::vector<bool>> gshare_PHT(gshare_PHT_rows,std::vector<bool>(gshare_PHT_width,0));

std::vector<std::vector<bool>> Meta_SAg_PHT(Meta_SAg_PHT_rows,std::vector<bool>(Meta_SAg_PHT_width,0));
std::vector<std::vector<bool>> Meta_SAg_BHT(Meta_SAg_BHT_rows,std::vector<bool>(Meta_SAg_BHT_width,0));
std::vector<std::vector<bool>> Meta_GAg_PHT(Meta_GAg_PHT_rows,std::vector<bool>(Meta_GAg_PHT_width,0));
std::vector<std::vector<bool>> Meta_gshare_PHT(Meta_gshare_PHT_rows,std::vector<bool>(Meta_gshare_PHT_width,0));

std::vector<std::vector<bool>> Meta_SAg_GAg(Meta_SAg_GAg_rows,std::vector<bool>(Meta_SAg_GAg_width,0));
std::vector<std::vector<bool>> Meta_SAg_gshare(Meta_SAg_gshare_rows,std::vector<bool>(Meta_SAg_gshare_width,0));
std::vector<std::vector<bool>> Meta_GAg_gshare(Meta_GAg_gshare_rows,std::vector<bool>(Meta_GAg_gshare_width,0));

std::vector<std::vector<cache_entry>> BTBBuffer1(BTB_sets,std::vector<cache_entry>(BTB_ways));
std::vector<std::vector<cache_entry>> BTBBuffer2(BTB_sets,std::vector<cache_entry>(BTB_ways));

VOID increment(std::vector<std::vector<bool>> &vec,int idx){
    int sz=vec[idx].size();
    int first_zero=0;
    while(first_zero < sz && vec[idx][first_zero] == 1){
        first_zero++;
    }
    if(first_zero < sz){
        vec[idx][first_zero] = 1;
        for(int i = 0; i < first_zero; i++){
            vec[idx][i] = 0;
        }
    }
}

VOID decrement(std::vector<std::vector<bool>> &vec,int idx){
    int sz=vec[idx].size();
    int first_one=0;
    while(first_one < sz && vec[idx][first_one] == 0){
        first_one++;
    }
    if(first_one < sz){
        vec[idx][first_one] = 0;
        for(int i = 0; i < first_one; i++){
            vec[idx][i] = 1;
        }
    }
}

VOID update_history(std::vector<std::vector<bool>> &vec,int idx,int new_bit){
    int sz = vec[idx].size();
    for(int i = sz - 1; i > 0; i--){
        vec[idx][i] = vec[idx][i-1];
    }
    vec[idx][0] = new_bit;
}

INT get_idx(std::vector<bool> &vec){
    int val = 0;
    for(int i = 0; i < (int)vec.size(); i++){
        int value = 0;
        if(vec[i]){
            value = 1;
        }
        val += (value<<i);
    }
    return val;
}

VOID InsCount(void)
{
	icount++;
}

ADDRINT FastForward(void)
{
	return (icount >= fastForwardIns && icount < maxIns);
}

ADDRINT Terminate(void)
{
        return (icount >= maxIns);
}

VOID MyAnalysis_PartA(ADDRINT IP,ADDRINT TGT,int BT){
    branch_count++;
    int is_forward = 0;
    if(TGT >= IP){
        is_forward = 1;
        forward_branch_count++;
    }
    else{
        backward_branch_count++;
    }
    //static
    if(TGT >= IP && BT != 1){
        static_correct++;
        static_forward_correct++;
    }
    else if(TGT < IP && BT == 1){
        static_correct++;
        static_backward_correct++;
    }

    //bimodal
    int bimodal_idx = IP%bimodal_PHT_rows;
    if(bimodal_PHT[bimodal_idx][1] == BT){
        bimodal_correct++;
        if(is_forward){
            bimodal_forward_correct++;
        }
        else{
            bimodal_backward_correct++;
        }
    }
    if(BT == 1){
        increment(bimodal_PHT,bimodal_idx);
    }
    else{
        decrement(bimodal_PHT,bimodal_idx);
    }

    //SAg
    int SAg_bht_idx = IP%SAg_BHT_rows;
    int SAg_pht_idx = get_idx(SAg_BHT[SAg_bht_idx]);
    //int SAg_prediction = SAg_PHT[SAg_pht_idx][SAg_PHT_width-1];
    if(SAg_PHT[SAg_pht_idx][SAg_PHT_width-1] == BT){
        SAg_correct++;
        if(is_forward){
            SAg_forward_correct++;
        }
        else{
            SAg_backward_correct++;
        }
    }
    if(BT == 1){
        increment(SAg_PHT,SAg_pht_idx);
    }
    else{
        decrement(SAg_PHT,SAg_pht_idx);
    }
    //Updation at the end
    

    //GAg
    int GAg_pht_idx = get_idx(GHR[0]);
    //int GAg_prediction = GAg_PHT[GAg_pht_idx][GAg_PHT_width-1];
    if(GAg_PHT[GAg_pht_idx][GAg_PHT_width-1] == BT){
        GAg_correct++;
        if(is_forward){
            GAg_forward_correct++;
        }
        else{
            GAg_backward_correct++;
        }
    }
    if(BT == 1){
        increment(GAg_PHT,GAg_pht_idx);
    }
    else{
        decrement(GAg_PHT,GAg_pht_idx);
    }
    //update_GHR done later

    //gshare
    int temp = IP%gshare_PHT_rows;
    int gshare_pht_idx = (GAg_pht_idx^temp);
    //int gshare_prediction = gshare_PHT[gshare_pht_idx][gshare_PHT_width-1];
    if(gshare_PHT[gshare_pht_idx][gshare_PHT_width-1] == BT){
        gshare_correct++;
        if(is_forward){
            gshare_forward_correct++;
        }
        else{
            gshare_backward_correct++;
        }
    }
    if(BT == 1){
        increment(gshare_PHT,gshare_pht_idx);
    }
    else{
        decrement(gshare_PHT,gshare_pht_idx);
    }
    //update_GHR done later

    //Meta Predictors : Different Meta GAg SAg gshare are maintained but updated as above
    //Meta SAg
    int Meta_SAg_bht_idx = IP%Meta_SAg_BHT_rows;
    int Meta_SAg_pht_idx = get_idx(Meta_SAg_BHT[Meta_SAg_bht_idx]);
    int Meta_SAg_prediction = Meta_SAg_PHT[Meta_SAg_pht_idx][Meta_SAg_PHT_width-1];
    if(Meta_SAg_PHT[Meta_SAg_pht_idx][Meta_SAg_PHT_width-1] == BT){
        Meta_SAg_correct++;
        if(is_forward){
            Meta_SAg_forward_correct++;
        }
        else{
            Meta_SAg_backward_correct++;
        }
    }
    if(BT == 1){
        increment(Meta_SAg_PHT,Meta_SAg_pht_idx);
    }
    else{
        decrement(Meta_SAg_PHT,Meta_SAg_pht_idx);
    }
    //Updation at the end
    

    //Meta GAg
    int Meta_GAg_pht_idx = get_idx(GHR[0]);
    int Meta_GAg_prediction = Meta_GAg_PHT[Meta_GAg_pht_idx][Meta_GAg_PHT_width-1];
    if(Meta_GAg_PHT[Meta_GAg_pht_idx][Meta_GAg_PHT_width-1] == BT){
        Meta_GAg_correct++;
        if(is_forward){
            Meta_GAg_forward_correct++;
        }
        else{
            Meta_GAg_backward_correct++;
        }
    }
    if(BT == 1){
        increment(Meta_GAg_PHT,Meta_GAg_pht_idx);
    }
    else{
        decrement(Meta_GAg_PHT,Meta_GAg_pht_idx);
    }
    //update_GHR done later

    //Meta gshare
    int Meta_temp = IP%Meta_gshare_PHT_rows;
    int Meta_gshare_pht_idx = (Meta_GAg_pht_idx^Meta_temp);
    int Meta_gshare_prediction = Meta_gshare_PHT[Meta_gshare_pht_idx][Meta_gshare_PHT_width-1];
    if(Meta_gshare_PHT[Meta_gshare_pht_idx][Meta_gshare_PHT_width-1] == BT){
        Meta_gshare_correct++;
        if(is_forward){
            Meta_gshare_forward_correct++;
        }
        else{
            Meta_gshare_backward_correct++;
        }
    }
    if(BT == 1){
        increment(Meta_gshare_PHT,Meta_gshare_pht_idx);
    }
    else{
        decrement(Meta_gshare_PHT,Meta_gshare_pht_idx);
    }


    //Hybrid SAg GAg
    int meta_idx = get_idx(GHR[0]);
    int Hybrid_prediction = 0;
    int winner1 = 0;
    if(Meta_SAg_GAg[meta_idx][Meta_SAg_GAg_width-1] == 1){
        Hybrid_prediction = Meta_SAg_prediction;
        winner1 = 1;
    }
    else{
        Hybrid_prediction = Meta_GAg_prediction;
        winner1 = 2;
    }
    if(Hybrid_prediction == BT){
        Hybrid_correct++;
        if(is_forward){
            Hybrid_forward_correct++;
        }
        else{
            Hybrid_backward_correct++;
        }
    }

    if(BT == Meta_SAg_prediction && BT != Meta_GAg_prediction){
        increment(Meta_SAg_GAg,meta_idx);
    }
    else if(BT != Meta_SAg_prediction && BT == Meta_GAg_prediction){
        decrement(Meta_SAg_GAg,meta_idx);
    }

    //Hybrid21 majority predictor
    int Hybrid21_prediction = Meta_SAg_prediction + Meta_GAg_prediction + Meta_gshare_prediction;
    if(Hybrid21_prediction > 1){
        Hybrid21_prediction = 1;
    }
    else{
        Hybrid21_prediction = 0;
    }
    if(Hybrid21_prediction == BT){
        Hybrid21_correct++;
        if(is_forward){
            Hybrid21_forward_correct++;
        }
        else{
            Hybrid21_backward_correct++;
        }
    }

    //Hybrid22 tournament predictor
    int meta_idx2 = get_idx(GHR[0]);
    int winner2 = 0;
    if(winner1 == 1){
        if(Meta_SAg_gshare[meta_idx2][Meta_SAg_gshare_width-1] == 1){
            winner2 = 1;
        }
        else{
            winner2 = 3;
        }
    }
    else{
        if(Meta_GAg_gshare[meta_idx2][Meta_GAg_gshare_width-1] == 1){
            winner2 = 2;
        }
        else{
            winner2 = 3;
        }
    }
    int Hybrid22_prediction;
    if(winner2 == 1){
        Hybrid22_prediction = Meta_SAg_prediction;
    }
    else if(winner2 == 2){
        Hybrid22_prediction = Meta_GAg_prediction;
    }
    else{
        Hybrid22_prediction = Meta_gshare_prediction;
    }
    if(Hybrid22_prediction == BT){
        Hybrid22_correct++;
        if(is_forward){
            Hybrid22_forward_correct++;
        }
        else{
            Hybrid22_backward_correct++;
        }
    }

    if(BT == Meta_SAg_prediction && BT != Meta_gshare_prediction){
        increment(Meta_SAg_gshare,meta_idx2);
    }
    else if(BT != Meta_SAg_prediction && BT == Meta_gshare_prediction){
        decrement(Meta_SAg_gshare,meta_idx2);
    }

    if(BT == Meta_GAg_prediction && BT != Meta_gshare_prediction){
        increment(Meta_GAg_gshare,meta_idx2);
    }
    else if(BT != Meta_GAg_prediction && BT == Meta_gshare_prediction){
        decrement(Meta_GAg_gshare,meta_idx2);
    }

    update_history(SAg_BHT,SAg_bht_idx,BT);
    update_history(Meta_SAg_BHT,Meta_SAg_bht_idx,BT);
    update_history(GHR,0,BT);

}

VOID MyAnalysis_PartB(ADDRINT IP,ADDRINT TGT,int BT){
    indirect_count++;
    time_BTB++;
    //Part1
    
    UINT64 BTBidx1 = IP%(BTB_sets);
    long long predicted_tgt1 = -1;
    int found1 = 0;
    for(auto way : BTBBuffer1[BTBidx1]){
        if(way.tag == IP && way.valid == 1){
            predicted_tgt1 = (long long)way.target;
            found1 = 1;
            break;
        }
    }
    if(!found1){
        misses_BTB1++;
    }
    if((predicted_tgt1 == (long long)TGT && BT == 1) || (BT == 0 && predicted_tgt1 == -1)){
        BTB1_correct_count++;
    }
    else{
        
        if(found1){
            for(auto &way : BTBBuffer1[BTBidx1]){
                if(way.tag == IP && way.valid == 1){
                    if(BT == 1){
                        way.target = TGT;
                        way.LRU = time_BTB;
                    }
                    else{
                        way.valid = 0;
                    }
                    break;
                }
            }
        }
        else if(BT == 1){
            long long replace_idx = -1, mn = 1e18;
            int idx = 0;
            for(auto &way : BTBBuffer1[BTBidx1]){
                if(way.valid == 0){
                    replace_idx = idx;
                    break;
                }
                idx++;
            }
            if(replace_idx == -1){
                idx = 0;
                for(auto &way : BTBBuffer1[BTBidx1]){
                    if(way.LRU < (UINT64)mn){
                        replace_idx = idx;
                        mn = way.LRU;
                    }
                    idx++;
                }
            }
            BTBBuffer1[BTBidx1][replace_idx].tag = IP;
            BTBBuffer1[BTBidx1][replace_idx].valid = 1;
            BTBBuffer1[BTBidx1][replace_idx].LRU = time_BTB;
            BTBBuffer1[BTBidx1][replace_idx].target = TGT;
        }
    }

    //Part2
    UINT64 BTBidx2 = IP%(BTB_sets);
    UINT64 mask = 0;
    for(int i = 0; i < log_BTB_sets; i++){
        long long bitmask = (1LL<<i);
        if(GHR[0][i] == 1){
            mask += bitmask;
        }
    }
    BTBidx2 = (BTBidx2^mask);
    long long predicted_tgt2 = -1;
    int found2 = 0;
    for(auto way : BTBBuffer2[BTBidx2]){
        if(way.tag == IP && way.valid == 1){
            predicted_tgt2 = (long long)way.target;
            found2 = 1;
            break;
        }
    }
    if(!found2){
        misses_BTB2++;
    }
    if((predicted_tgt2 == (long long)TGT && BT == 1) || (BT == 0 && predicted_tgt2 == -1)){
        BTB2_correct_count++;
    }
    else{
        if(found2){
            for(auto &way : BTBBuffer2[BTBidx2]){
                if(way.tag == IP && way.valid == 1){
                    if(BT == 1){
                        way.target = TGT;
                        way.LRU = time_BTB;
                    }
                    else{
                        way.valid = 0;
                    }
                    break;
                }
            }
        }
        else if(BT == 1){
            long long replace_idx = -1, mn = 1e18;
            int idx = 0;
            for(auto &way : BTBBuffer2[BTBidx2]){
                if(way.valid == 0){
                    replace_idx = idx;
                    break;
                }
                idx++;
            }
            if(replace_idx == -1){
                idx = 0;
                for(auto &way : BTBBuffer2[BTBidx2]){
                    if(way.LRU < (UINT64)mn){
                        replace_idx = idx;
                        mn = way.LRU;
                    }
                    idx++;
                }
            }
            BTBBuffer2[BTBidx2][replace_idx].tag = IP;
            BTBBuffer2[BTBidx2][replace_idx].valid = 1;
            BTBBuffer2[BTBidx2][replace_idx].LRU = time_BTB;
            BTBBuffer2[BTBidx2][replace_idx].target = TGT;
            
        }
    }
}

VOID StatDump(){
    // Static Predictor
    *out << "===============================================" << endl;
    *out << "Direction Predictors" << endl;
    *out << "Static : Accesses " << branch_count << ", Mispredictions " << (branch_count - static_correct) 
         << " (" << static_cast<double>(branch_count - static_correct) / branch_count << "), "
         << "Forward branches " << forward_branch_count << ", Forward mispredictions " 
         << (forward_branch_count - static_forward_correct) << " (" 
         << static_cast<double>(forward_branch_count - static_forward_correct) / forward_branch_count << "), "
         << "Backward branches " << backward_branch_count << ", Backward mispredictions " 
         << (backward_branch_count - static_backward_correct) << " (" 
         << static_cast<double>(backward_branch_count - static_backward_correct) / backward_branch_count << ")" << endl;

    // Bimodal Predictor
    *out << "Bimodal : Accesses " << branch_count << ", Mispredictions " << (branch_count - bimodal_correct) 
         << " (" << static_cast<double>(branch_count - bimodal_correct) / branch_count << "), "
         << "Forward branches " << forward_branch_count << ", Forward mispredictions " 
         << (forward_branch_count - bimodal_forward_correct) << " (" 
         << static_cast<double>(forward_branch_count - bimodal_forward_correct) / forward_branch_count << "), "
         << "Backward branches " << backward_branch_count << ", Backward mispredictions " 
         << (backward_branch_count - bimodal_backward_correct) << " (" 
         << static_cast<double>(backward_branch_count - bimodal_backward_correct) / backward_branch_count << ")" << endl;

    // SAg Predictor
    *out << "SAg : Accesses " << branch_count << ", Mispredictions " << (branch_count - SAg_correct) 
         << " (" << static_cast<double>(branch_count - SAg_correct) / branch_count << "), "
         << "Forward branches " << forward_branch_count << ", Forward mispredictions " 
         << (forward_branch_count - SAg_forward_correct) << " (" 
         << static_cast<double>(forward_branch_count - SAg_forward_correct) / forward_branch_count << "), "
         << "Backward branches " << backward_branch_count << ", Backward mispredictions " 
         << (backward_branch_count - SAg_backward_correct) << " (" 
         << static_cast<double>(backward_branch_count - SAg_backward_correct) / backward_branch_count << ")" << endl;

    // GAg Predictor
    *out << "GAg : Accesses " << branch_count << ", Mispredictions " << (branch_count - GAg_correct) 
         << " (" << static_cast<double>(branch_count - GAg_correct) / branch_count << "), "
         << "Forward branches " << forward_branch_count << ", Forward mispredictions " 
         << (forward_branch_count - GAg_forward_correct) << " (" 
         << static_cast<double>(forward_branch_count - GAg_forward_correct) / forward_branch_count << "), "
         << "Backward branches " << backward_branch_count << ", Backward mispredictions " 
         << (backward_branch_count - GAg_backward_correct) << " (" 
         << static_cast<double>(backward_branch_count - GAg_backward_correct) / backward_branch_count << ")" << endl;

    // Gshare Predictor
    *out << "gshare : Accesses " << branch_count << ", Mispredictions " << (branch_count - gshare_correct) 
         << " (" << static_cast<double>(branch_count - gshare_correct) / branch_count << "), "
         << "Forward branches " << forward_branch_count << ", Forward mispredictions " 
         << (forward_branch_count - gshare_forward_correct) << " (" 
         << static_cast<double>(forward_branch_count - gshare_forward_correct) / forward_branch_count << "), "
         << "Backward branches " << backward_branch_count << ", Backward mispredictions " 
         << (backward_branch_count - gshare_backward_correct) << " (" 
         << static_cast<double>(backward_branch_count - gshare_backward_correct) / backward_branch_count << ")" << endl;

    // Hybrid SAg-GAg Predictor
    *out << "Combined2 : Accesses " << branch_count << ", Mispredictions " << (branch_count - Hybrid_correct) 
         << " (" << static_cast<double>(branch_count - Hybrid_correct) / branch_count << "), "
         << "Forward branches " << forward_branch_count << ", Forward mispredictions " 
         << (forward_branch_count - Hybrid_forward_correct) << " (" 
         << static_cast<double>(forward_branch_count - Hybrid_forward_correct) / forward_branch_count << "), "
         << "Backward branches " << backward_branch_count << ", Backward mispredictions " 
         << (backward_branch_count - Hybrid_backward_correct) << " (" 
         << static_cast<double>(backward_branch_count - Hybrid_backward_correct) / backward_branch_count << ")" << endl;

    // Hybrid21 (Majority) Predictor
    *out << "Combined3Majority : Accesses " << branch_count << ", Mispredictions " << (branch_count - Hybrid21_correct) 
         << " (" << static_cast<double>(branch_count - Hybrid21_correct) / branch_count << "), "
         << "Forward branches " << forward_branch_count << ", Forward mispredictions " 
         << (forward_branch_count - Hybrid21_forward_correct) << " (" 
         << static_cast<double>(forward_branch_count - Hybrid21_forward_correct) / forward_branch_count << "), "
         << "Backward branches " << backward_branch_count << ", Backward mispredictions " 
         << (backward_branch_count - Hybrid21_backward_correct) << " (" 
         << static_cast<double>(backward_branch_count - Hybrid21_backward_correct) / backward_branch_count << ")" << endl;

    // Hybrid22 (Tournament) Predictor
    *out << "Combined3 : Accesses " << branch_count << ", Mispredictions " << (branch_count - Hybrid22_correct) 
         << " (" << static_cast<double>(branch_count - Hybrid22_correct) / branch_count << "), "
         << "Forward branches " << forward_branch_count << ", Forward mispredictions " 
         << (forward_branch_count - Hybrid22_forward_correct) << " (" 
         << static_cast<double>(forward_branch_count - Hybrid22_forward_correct) / forward_branch_count << "), "
         << "Backward branches " << backward_branch_count << ", Backward mispredictions " 
         << (backward_branch_count - Hybrid22_backward_correct) << " (" 
         << static_cast<double>(backward_branch_count - Hybrid22_backward_correct) / backward_branch_count << ")" << endl;

    *out << endl << "Branch Target Predictors" << endl;

    // Part B, BTB1
    *out << "BTB1 : Accesses " << indirect_count << ", Misses " << misses_BTB1 
         << " (" << static_cast<double>(misses_BTB1) / indirect_count << "), "
         << "Mispredictions " << (indirect_count - BTB1_correct_count) 
         << " (" << static_cast<double>(indirect_count - BTB1_correct_count) / indirect_count << ")" << endl;

    // Part B, BTB2
    *out << "BTB2 : Accesses " << indirect_count << ", Misses " << misses_BTB2 
         << " (" << static_cast<double>(misses_BTB2) / indirect_count << "), "
         << "Mispredictions " << (indirect_count - BTB2_correct_count) 
         << " (" << static_cast<double>(indirect_count - BTB2_correct_count) / indirect_count << ")" << endl;

    *out << "===============================================" << endl;

    exit(0);
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */


/* ===================================================================== */
// Instrumentation callbacks
/* ===================================================================== */


VOID Instruction(INS ins, VOID *v)
{
	
	INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) Terminate, IARG_END);
    INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR) StatDump, IARG_END);

    //IARG_BRANCH_TAKEN and IARG_BRANCH_TARGET_ADDR

    if(INS_Category(ins) == XED_CATEGORY_COND_BR){
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
        INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR) MyAnalysis_PartA, IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
    }

    if(INS_IsIndirectControlFlow(ins)){
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
        INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR) MyAnalysis_PartB, IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
    }
	
	/* Called for each instruction */
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) InsCount, IARG_END);
}


VOID Fini(INT32 code, VOID* v)
{
    StatDump();
    *out << "===============================================" << endl;
    *out << "MyPinTool analysis results: " << endl;
    *out << "===============================================" << endl;
}



INT32 Usage()
{
	cerr << "CS422 Homework 1" << endl;
	cerr << KNOB_BASE::StringKnobSummary() << endl;
	return -1;
}

int main(int argc, char* argv[])
{
    // Initialize PIN library. Print help message if -h(elp) is specified
	// in the command line or the command line is invalid 
	if (PIN_Init(argc, argv))
		return Usage();

	/* Set number of instructions to fast forward and simulate */
	fastForwardIns = KnobFastForward.Value() * BILLION;
	maxIns = fastForwardIns + BILLION;

	string fileName = KnobOutputFile.Value();

	if (!fileName.empty())
		out = new std::ofstream(fileName.c_str());

	
	// Register function to be called to instrument instructions
	INS_AddInstrumentFunction(Instruction, 0);

	// Register function to be called when the application exits
	PIN_AddFiniFunction(Fini, 0);

	cerr << "===============================================" << endl;
	cerr << "This application is instrumented by HW2" << endl;
	if (!KnobOutputFile.Value().empty())
		cerr << "See file " << KnobOutputFile.Value() << " for analysis results" << endl;
	cerr << "===============================================" << endl;

	// Start the program, never returns
	PIN_StartProgram();

	return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
