//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <string.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Chen SHen";
const char *studentID   = "PID A59003243";
const char *email       = "EMAIL c3shen@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//
uint32_t globalHistory;
uint8_t* gShare_BHT;
uint8_t* local_BHT;
uint8_t* global_BHT;
uint32_t* PHT;
uint8_t* PTChoice;
int8_t** linearWeights;
int8_t* gat;
int MAX,MIN,THRESHOLD,ghb;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

void 
init_predictor_gShare(){
  globalHistory = 0;
  int size = (1 << ghistoryBits) * sizeof(uint8_t);
  gShare_BHT = malloc(size);
  memset(gShare_BHT, WN, size);
}

void 
init_predictor_TOURNAMENT(){
    
    globalHistory = 0;
    
    int localSize = 1 << lhistoryBits;
    int globalSize = 1 << ghistoryBits;
    int PHTSize = 1 << pcIndexBits;
    int choiceSize = 1 << ghistoryBits;
    
    local_BHT = malloc(localSize * sizeof(uint8_t));
    global_BHT = malloc(globalSize * sizeof(uint8_t));
    PHT = malloc(PHTSize * sizeof(uint32_t));
    PTChoice = malloc(choiceSize * sizeof(uint8_t));
    
    memset(local_BHT, WN, localSize * sizeof(uint8_t));
    memset(global_BHT, WN, globalSize * sizeof(uint8_t));
    memset(PHT, 0, PHTSize * sizeof(uint32_t));
    memset(PTChoice, WN, choiceSize * sizeof(uint8_t)); 
}

void
init_predictor_CUSTOM(){
    
    globalHistory = 0;
    MAX = 8;
    MIN = -8;
    THRESHOLD = 32;
    ghb = ghistoryBits <= 12 ? ghistoryBits : 12;

    int gShareSize = 1 << ghistoryBits;
    //int PHTSize = 1 << pcIndexBits;

    linearWeights = malloc(sizeof(int8_t *) * gShareSize);
    for (int i = 0; i < gShareSize; i++){
      linearWeights[i] = malloc(sizeof(int8_t) * (gShareSize + 1));
      memset(linearWeights[i], 0, sizeof(int8_t) * (gShareSize + 1));
    }

    
    gat = malloc(sizeof(int8_t) * ghb);
    memset(gat, 0, sizeof(int8_t) * ghb); // 0 NOTTAKEN, 1TAKEN

}

void
init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
  //
  switch(bpType){
    case STATIC:
      return;
    case GSHARE:
      init_predictor_gShare();
      break;
    case TOURNAMENT:
      init_predictor_TOURNAMENT();
      break;
    case CUSTOM:
      init_predictor_CUSTOM();
      break;
    default:
      break;
  }
}


// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t 
make_prediction_gShare(uint32_t pc){
  uint32_t mask = (1 << ghistoryBits) - 1;
  uint32_t gShareIdx = (globalHistory & mask) ^ (pc & mask);
  uint8_t gSharePred = gShare_BHT[gShareIdx];
  if (gSharePred == WT || gSharePred == ST){
    return TAKEN;
  }else{
    return NOTTAKEN;
  }
}

uint8_t
make_prediction_TOURNAMENT(uint32_t pc) {
    uint32_t pcMask= (1<<pcIndexBits) - 1;
    uint32_t globalMask = (1 << ghistoryBits) - 1;

    int PHTIdx = pc & pcMask;
    int localIdx = PHT[PHTIdx];
    int localPred = local_BHT[localIdx];

    int globalIdx = globalHistory & globalMask;
    int globalPred = global_BHT[globalIdx];
    
    int choice = PTChoice[globalIdx];
    
    if (choice == ST || choice == WT) {
        return (globalPred == ST || globalPred == WT) ? TAKEN: NOTTAKEN;
    }else {
        return (localPred == ST || localPred == WT ) ? TAKEN: NOTTAKEN;
    }
    
}

uint8_t
make_prediction_CUSTOM(uint32_t pc){
    //uint32_t pcMask= (1<<pcIndexBits) - 1;
    uint32_t gShareMask = (1 << ghistoryBits) - 1;

    int gShareIdx = (globalHistory & gShareMask) ^ (pc & gShareMask);
    int8_t* gShareVal = linearWeights[gShareIdx];

    int y = gShareVal[ghb];
    for (int i = 0; i < ghb; i++){
      if (gat[i] == 0){
        y -= gShareVal[i];
      }else{
        y += gShareVal[i];
      }
    }

    if (y >= 0){
      return TAKEN;
    }else{
      return NOTTAKEN;
    }
}

uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;

    case GSHARE:
      return make_prediction_gShare(pc);

    case TOURNAMENT:
      return make_prediction_TOURNAMENT(pc);
      
    case CUSTOM:
      return make_prediction_CUSTOM(pc);

    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

void train_predictor_gShare(uint32_t pc, uint8_t outcome){
  uint32_t mask = (1<<ghistoryBits) - 1;
  uint32_t gShareIdx = (globalHistory & mask) ^ (pc & mask);
  globalHistory = mask & (globalHistory << 1);
  
  if (outcome == TAKEN) {
    globalHistory += 1;
    switch (gShare_BHT[gShareIdx])
    {
    case SN:
      gShare_BHT[gShareIdx] = WN;
      break;
    case WN:
      gShare_BHT[gShareIdx] = WT;
      break;
    case WT:
      gShare_BHT[gShareIdx] = ST;
      break;
    default:
      break;
    }
  }else if (outcome == NOTTAKEN) {
    switch (gShare_BHT[gShareIdx])
    {
    case WN:
      gShare_BHT[gShareIdx] = SN;
      break;
    case WT:
      gShare_BHT[gShareIdx] = WN;
      break;
    case ST:
      gShare_BHT[gShareIdx] = WT;
      break;
    default:
      break;
    }
  }
}

void 
train_predictor_TOURNAMENT(uint32_t pc, uint8_t outcome) {  
  uint32_t pcMask = (1 << pcIndexBits) - 1;
  uint32_t localMask = (1 << lhistoryBits) - 1;
  uint32_t globalMask = (1 << ghistoryBits) - 1;
  
  int PHTIdx = pc & pcMask;
  int localIdx = PHT[PHTIdx];
  int localPred = local_BHT[localIdx];

  int globalIdx = globalHistory & globalMask;
  int globalPred = global_BHT[globalIdx];
  
  int choice = PTChoice[globalIdx];

  int localPredNT = (localPred == ST || localPred == WT ) ? TAKEN: NOTTAKEN;
  int globalPredNT = (globalPred == ST || globalPred == WT) ? TAKEN: NOTTAKEN;
  
  if (outcome == TAKEN){
      if (localPredNT != globalPredNT) {
        if (localPredNT == TAKEN && choice > SN){
          PTChoice[globalIdx]--;
        }else if (globalPredNT == TAKEN && choice < ST) {
          PTChoice[globalIdx]++;
        }
      }
      
      if (localPred < ST) local_BHT[localIdx]++;
      if (globalPredNT < ST) global_BHT[globalIdx]++;

      PHT[PHTIdx] = ((localIdx << 1) + 1) & localMask;
      globalHistory = ((globalHistory << 1) + 1) & globalIdx;
    }else{
      if (localPredNT != globalPredNT){
        if (localPredNT == NOTTAKEN && choice > SN){
          PTChoice[globalIdx]--;
        }else if (globalPredNT == NOTTAKEN && choice < ST) {
          PTChoice[globalIdx]++;
        }
      }
      
      if (localPred > SN ) local_BHT[localIdx]--;
      if (globalPredNT > SN) global_BHT[globalIdx]--;
  
      PHT[PHTIdx] = (localIdx << 1) & localMask;
      globalHistory = (globalHistory << 1) & globalIdx;
  }
}

int limit_weight(int w){
  if (w >= MAX){
    return MAX;
  }else if(w <= MIN){
    return MIN;
  }else{
    return w;
  }
}
void 
train_predictor_CUSTOM(uint32_t pc, uint8_t outcome) {
  //uint32_t pcMask = (1 << pcIndexBits) - 1;
  uint32_t gShareMask = (1 << ghistoryBits) - 1;
  
  int gShareIdx = (globalHistory & gShareMask) ^ (pc & gShareMask);
  int8_t* gShareVal = linearWeights[gShareIdx];
  
  int y = gShareVal[ghb];
  for (int i = 0; i < ghb; i++){
    if (gat[i] == 0){
      y -= gShareVal[i];
    }else{
      y += gShareVal[i];
    }
  }
  int pred = (y>=0) ? TAKEN:NOTTAKEN;

  if (outcome != pred || (y <= THRESHOLD && y>= -1*THRESHOLD)){
    linearWeights[gShareIdx][ghb] = limit_weight( linearWeights[gShareIdx][ghb] + (outcome == TAKEN ? 1:-1));
    for (int i = 0; i< ghb; i++){
      if (gat[i] == outcome){
        linearWeights[gShareIdx][i] = limit_weight(linearWeights[gShareIdx][i] + 1);
      }else{
        linearWeights[gShareIdx][i] = limit_weight(linearWeights[gShareIdx][i] - 1);
      }
    }
  }

  for (int i = ghb - 1; i > 0; i--){
    gat[i] = gat[i-1];
  }
  gat[0] = outcome;
  
  globalHistory = (globalHistory << 1) + outcome;

}

void
train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
  //
  switch (bpType) {
    case STATIC:
      break;

    case GSHARE:
      train_predictor_gShare(pc, outcome);
      break;

    case TOURNAMENT:
      train_predictor_TOURNAMENT(pc, outcome);
      break;

    case CUSTOM:
      train_predictor_CUSTOM(pc, outcome);
      break;

    default:
      break;
  }
}
