#include "ARMXCFINSC.h"
#include "ARMXCFIOptions.h"
#include "ARMBaseInstrInfo.h"
#include "MCTargetDesc/ARMMCTargetDesc.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"

using namespace llvm;
using namespace llvm::XCFI;

char ARMXCFINSC::ID = 0;
ARMXCFINSC::ARMXCFINSC(): MachineFunctionPass(ID) {}

bool ARMXCFINSC::runOnMachineFunction(MachineFunction &MF) {
    if (XCFIDisabled()) return true;
    if (XCFI::mode() == XCFI::Backward) return true;

    const Function &F = MF.getFunction();
    if (!F.hasFnAttribute("cmse_nonsecure_entry")) return true;

    errs() << "NSC veneer detected: " << F.getName() << "\n";
    MachineBasicBlock &MBB = MF.front();
    for (MachineInstr &MI: MBB) {
        if (MI.getOpcode() == ARM::t2SG) { 
            const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
            MachineInstr *Next = MI.getNextNode();
            if (Next) {
                DebugLoc DL = Next->getDebugLoc();
                BuildMI(MBB, Next, DL, TII->get(ARM::INLINEASM))
                    .addExternalSymbol(MF.createExternalSymbolName("ldr r10, [pc, #-12]"))
                    .addImm(1)
                    .addExternalSymbol("");
                BuildMI(MBB, Next, DL, TII->get(ARM::t2EORrr), ARM::R9)
                    .addReg(ARM::R9)
                    .addReg(ARM::R10)
                    .addImm(2)
                    .addImm(ARMCC::AL)
                    .addReg(0);
                BuildMI(MBB, Next, DL, TII->get(ARM::t2UXTH), ARM::R9).addReg(ARM::R9).addImm(0).add(predOps(ARMCC::AL, 0));
                BuildMI(MBB, Next, DL, TII->get(ARM::t2CMPri), ARM::R9).addImm(0).add(predOps(ARMCC::AL, 0));
                BuildMI(MBB, Next, DL, TII->get(ARM::INLINEASM))
                    .addExternalSymbol(MF.createExternalSymbolName("bne XCFI_fault"))
                    .addImm(1)
                    .addExternalSymbol("");
            }

            break;
        }
    }


    return true;
}

namespace llvm {
    FunctionPass *createARMXCFINSC(void) {
        return new ARMXCFINSC();
    }
}
