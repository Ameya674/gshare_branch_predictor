#ifndef __CPU_PRED_CUSTOM_BP_HH__
#define __CPU_PRED_CUSTOM_BP_HH__

#include "base/sat_counter.hh"
#include "cpu/pred/bpred_unit.hh"
#include "params/CustomBP.hh"

class CustomBP : public BPredUnit {

  public:
    // constructor for CustomBP.
    CustomBP(const CustomBPParams &params);
    // function to deal with unconditional branches.
    void uncondBranch(ThreadID tid, Addr pc, void * &bp_history);
    // function to squash speculatively fetched instructions and restoring previous state of global history.
    void squash(ThreadID tid, void *bp_history);
    // function to lookup the counter table and make a prediction based on corresponding counter value.
    bool lookup(ThreadID tid, Addr branch_addr, void * &bp_history);
    // function to update the global history in case of a miss.
    void btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history);
    // function to update the counter table and global history with the actual path taken.
    void update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history, 
        bool squashed, const StaticInstPtr & inst, Addr corrTarget);

  private:
    struct BPHistory {
      unsigned globalHistory;
    };
    // Vector that stores global history of each thread. It is indexed by the thread ID.
    std::vector<unsigned> globalHistory;
    // No. of lower bits to take from the global history and branch address, globalHistoryBits = 6
    unsigned globalHistoryBits;
    // No. of entries in the predictor. 2^globalHistoryBits. 64 entries in this case.
    unsigned globalPredictorSize;
    // Size of the counter. 2 bit counter in this case.
    unsigned globalCtrBits;
    // initializing the global counter table.
    std::vector<SatCounter8> globalCtrs;
    // Mask to ensure only "globalHistoryBits" bits are used for indexing the predictor.
    unsigned historyRegisterMask;
    // Max value that the counter can hold.
    unsigned globalThreshold;
};

#endif // __CPU_PRED_CUSTOM_BP_HH__

