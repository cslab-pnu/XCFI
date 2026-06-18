#ifndef ARM_XCFI_SECURE_EXCEPTION
#define ARM_XCFI_SECURE_EXCEPTION

#include "ARMXCFIInstrumentor.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/Pass.h"

namespace llvm {
	struct ARMXCFISecureException: public MachineFunctionPass, ARMXCFIInstrumentor {
		static char ID;

		ARMXCFISecureException();

		virtual bool runOnMachineFunction(MachineFunction &MF) override;
		
		void insertPrologueInstrumentation(MachineFunction &MF, MachineInstr &MI);
		void insertEpilogueInstrumentation(MachineFunction &MF, MachineInstr &MI);
	};

	FunctionPass *createARMXCFISecureException(void);
}

#endif
