#ifndef ARM_XCFI_BACKWARDCFI
#define ARM_XCFI_BACKWARDCFI

#include "ARMXCFIInstrumentor.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/Pass.h"

namespace llvm {
	struct ARMXCFIBackwardCFI: public MachineFunctionPass, ARMXCFIInstrumentor {
		static char ID;

		ARMXCFIBackwardCFI();

		virtual bool runOnMachineFunction(MachineFunction &MF) override;
	};

	FunctionPass *createARMXCFIBackwardCFI(void);
}

#endif
