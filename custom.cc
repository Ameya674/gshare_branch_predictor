#include "cpu/pred/custom.hh"
#include "base/bitfield.hh"
#include "base/intmath.hh"
/*
This is the constructor of the Custom Branch Predictor. This initializes the 
global history, global history bits, global predictor size, global ctr bits, and 
the global counter table. It also checks the size of the predictor and throws an 
error if its not a power of 2. For this configuration, the global history bits and
branch address bits used to index are 6, the predictor size is 2^6 = 64, and the 
counter is a 2 bit counter.
*/
CustomBP::CustomBP(const CustomBPParams &params) 
    : BPredUnit(params),
      // initializing all thread global registers to 0.
      globalHistory(params.numThreads, 0),
      // initializing globalHistory bits to 6.
      globalHistoryBits(params.globalHistoryBits),
      // setting global predictor size as 64.
      globalPredictorSize(params.globalPredictorSize),
      // setting up 2 bit counters.
      globalCtrBits(params.globalCtrBits),
      // setting up the global counter table. 
      globalCtrs(globalPredictorSize, SatCounter8(globalCtrBits))

{   
    if(!isPowerOf2(globalPredictorSize)) 
        fatal("Invalid global predictor size.\n");
    // creating a mask of 6 bits--> 111111
    historyRegisterMask = mask(globalHistoryBits);
    // creating a global threshold of 1 above which we take the branch, below which we don't.
    globalThreshold = (ULL(1) << (globalCtrBits - 1)) - 1;
}

/*
The lookup function uses the lower 6 bits of the branch address and 
global history and NANDs them to obtain the index for the global counter 
table. It looks up the value in the counter and predicts whether we take the 
branch or not. It then updates the global history with its prediction. But it
first stores the previous state in the BPHistory structure.
*/
bool CustomBP::lookup(ThreadID tid, Addr branch_addr, void * &bp_history) {
    // shif the branch address and NAND it with the global history after masking them seperately.
    unsigned index = ~(((branch_addr << instShiftAmt) ) & (globalHistory[tid]));
    index = index & historyRegisterMask;
    // use the index to acces the right counter and store the prediction. True -> taken, False -> not taken. 
    bool prediction = globalThreshold < globalCtrs[index];
    // Record the current global history in the shared state corresponding to the branch.
    BPHistory *history = new BPHistory;
    history->globalHistory = globalHistory[tid] & historyRegisterMask;
    bp_history = static_cast<void *>(history);
    // based on the prediction, update the global history.
    if (prediction) {
        // prediction is taken, append 1 to the global history.
        globalHistory[tid] = (globalHistory[tid] << 1) | 1;
        // mask the history to limit it to 6 bits.
        globalHistory[tid] = globalHistory[tid] & historyRegisterMask;
        // return the value of prediction -> true.
        return true;
    }
    else {
        // prediction is taken, append 0 to the global history.
        globalHistory[tid] = (globalHistory[tid] << 1);
        // mask the history to limit it to 6 bits.
        globalHistory[tid] = globalHistory[tid] & historyRegisterMask;
        // return the value of prediction-> false.
        return false;
    }
}
/*
This function is invoked when an unconditional branch occurs. An unconditional 
branch is always taken. So this creates a BPHistory structure to store the old 
state of the global history and then appends 1 to the global history.
*/
void CustomBP::uncondBranch(ThreadID tid, Addr pc, void * &bp_history) {
    // Record the current global history in the shared state corresponding to the branch.
    BPHistory *history = new BPHistory;
    history->globalHistory = globalHistory[tid] & historyRegisterMask;
    bp_history = static_cast<void *>(history);
    // update the global history register.
    globalHistory[tid] = globalHistory[tid] << 1 | 1;
    globalHistory[tid] = globalHistory[tid] & historyRegisterMask;
}

/*
This function is invoked during a branch miss. In this case the branch doesn't know
where to jump and hence it is predicted as not taken. So this function clears the last 
bit of the global history in case of a branch miss.
*/
void CustomBP::btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history) {
    globalHistory[tid] &= (historyRegisterMask & ~1ULL);
}

/*
This function updates uses the global history with the actual path taken, only when it 
is not squashed, and it also increments the counter based on the value of taken. 
If taken is true, the counters are incremented and if taken is false, the counter is
decremented.
*/
void CustomBP::update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history, bool squashed, const StaticInstPtr & inst, Addr corrTarget) {
    // Check if this pointer is null and throw an error if it is.
    assert(bp_history);
    // Cast the shared state of the branch into the history pointer.
    BPHistory *history = static_cast<BPHistory *>(bp_history);
    // if squashed update the global history with the actual prediction.
    if (squashed) {
        globalHistory[tid] = (history->globalHistory << 1) | taken;
        globalHistory[tid] = globalHistory[tid] & historyRegisterMask;
        return;
    }
    // increment the counters if taken is true, else decrement if false.
    unsigned index = ~(((branch_addr << instShiftAmt) ) & (globalHistory[tid]));
    index = index & historyRegisterMask;
    if (taken) {
        globalCtrs[index]++;
    }
    else {
        globalCtrs[index]--;
    }
    // delete the BPHistory structure.
    delete history;
}

/*
During a branch miss all the speculatively fetched instructions are 
squashed and the global history is restored back to its previous state
by using the BPHistory structure of the branch. This structure is then deleted.
*/
void CustomBP::squash(ThreadID tid, void *bp_history) {
    // retreive the previous state in the history pointer.
    BPHistory *history = static_cast<BPHistory*>(bp_history);
    globalHistory[tid] = history->globalHistory;
    // delete the object storing the state after retreival.
    delete history;
} 