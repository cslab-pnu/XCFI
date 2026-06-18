#ifndef ARM_XCFI_FORWARDCFI
#define ARM_XCFI_FORWARDCFI

#include "ARMXCFIInstrumentor.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/Pass.h"

namespace llvm {
	struct ARMXCFIForwardCFI: public MachineFunctionPass, ARMXCFIInstrumentor {
		static char ID;

		ARMXCFIForwardCFI();

		virtual bool runOnMachineFunction(MachineFunction &MF) override;
	};

	FunctionPass *createARMXCFIForwardCFI(void);
}

#endif
