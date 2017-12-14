#include "ARM.h"
#include "ARMBaseInstrInfo.h"
#include "ARMMachineFunctionInfo.h"
#include "ARMSubtarget.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"

#include <random>

using namespace llvm;

static cl::opt<unsigned>
NOPChance("nopinsertion_chance",cl::Hidden, cl::init((unsigned)0),
			  cl::desc("NOP insertion chance"));
static cl::opt<unsigned>
NOPSeed("nopinsertion_seed",cl::Hidden, cl::init((unsigned)0),
			  cl::desc("Random NOP insertion seed"));

namespace {
  class ARMNopInsertionPass : public MachineFunctionPass {
    private:
      static std::uniform_int_distribution<unsigned> pct_distribution;
    public:
      static char ID;
      ARMNopInsertionPass() : MachineFunctionPass(ID) {}

      bool runOnMachineFunction(MachineFunction &MF) override;

      StringRef getPassName() const override {
        return "ARM NOP insertion pass";
      }
  };
  char ARMNopInsertionPass::ID = 0;
}

std::uniform_int_distribution<unsigned> ARMNopInsertionPass::pct_distribution(0,99);

bool ARMNopInsertionPass::runOnMachineFunction(MachineFunction &mf) {
  if (!NOPSeed)
    return false;

  const ARMSubtarget& STI = static_cast<const ARMSubtarget &>(mf.getSubtarget());
  const ARMBaseInstrInfo *TII = STI.getInstrInfo();

  // Create the seed and PRNG for the function. We create a function-specific seed from
  // the root seed by XORing the root with the hash value of the function name.
  std::mt19937 mf_rng(((unsigned)NOPSeed) ^ hash_value(mf.getName().str()));

  // Iterate over all basic blocks and instructions in the function
  for (MachineBasicBlock &MBB : mf) {
    // For every basic block we create a new RNG
    std::mt19937 mbb_rng(mf_rng());
    bool rng_used = false;
    for (MachineBasicBlock::iterator I = MBB.SkipPHIsLabelsAndDebug(MBB.begin()), E = MBB.end(); I != E; I = MBB.SkipPHIsLabelsAndDebug(++I)) {
      // If we're dealing with a normal-sized instruction, roll the dice to determine whether to insert a NOP instruction or not
      if (TII->getInstSizeInBytes(*I) == 4) {
        if (pct_distribution(mbb_rng) < NOPChance)
          // Insert a NOP instruction, preceding I, using I's debug location
          BuildMI(MBB, I, I->getDebugLoc(), TII->get(llvm::ARM::MOVr), ARM::R0).addReg(ARM::R0)
            .add(predOps(ARMCC::AL))
            .add(condCodeOp());

        rng_used = true;
      }
      // If we encounter an abnormally sized instruction, re-seed (if necessary).
      else {
        if (rng_used)
          mbb_rng.seed(mf_rng());
        rng_used = false;
      }
    }
  }

  return true;
}

//
/// createARMNopInsertionPass - returns an instance of the NOP insertion
/// pass.
FunctionPass *llvm::createARMNopInsertionPass() {
  return new ARMNopInsertionPass();
}
