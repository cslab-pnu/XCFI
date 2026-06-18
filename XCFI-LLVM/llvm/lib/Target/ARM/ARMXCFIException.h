#ifndef ARM_XCFI_EXCEPTION
#define ARM_XCFI_EXCEPTION

#include "ARMXCFIInstrumentor.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/Pass.h"

namespace llvm {
	struct ARMXCFIException: public MachineFunctionPass, ARMXCFIInstrumentor {
		static char ID;

		ARMXCFIException();

		virtual bool runOnMachineFunction(MachineFunction &MF) override;
		
		void insertPrologueInstrumentation(MachineFunction &MF, MachineInstr &MI);
		void insertEpilogueInstrumentation(MachineFunction &MF, MachineInstr &MI);
	};

	FunctionPass *createARMXCFIException(void);
}

#endif
