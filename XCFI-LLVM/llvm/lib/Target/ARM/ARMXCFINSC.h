#ifndef ARM_XCFI_NSC
#define ARM_XCFI_NSC

#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/Pass.h"

namespace llvm {
	struct ARMXCFINSC: public MachineFunctionPass {
		static char ID;

		ARMXCFINSC();

		virtual bool runOnMachineFunction(MachineFunction &MF) override;
	};

	FunctionPass *createARMXCFINSC(void);
}

#endif
