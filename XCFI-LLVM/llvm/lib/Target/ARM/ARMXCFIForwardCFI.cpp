#include "ARMXCFIForwardCFI.h"
#include "ARMXCFIOptions.h"
#include "ARMXCFIInstrumentor.h"
#include "ARMBaseInstrInfo.h"
#include "MCTargetDesc/ARMMCTargetDesc.h"
#include "MCTargetDesc/ARMAddressingModes.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/Pass.h"

using namespace llvm;
using namespace llvm::XCFI;

extern bool TrustZoneCFIEnabled;

char ARMXCFIForwardCFI::ID = 0;
ARMXCFIForwardCFI::ARMXCFIForwardCFI(): MachineFunctionPass(ID) {}

bool ARMXCFIForwardCFI::runOnMachineFunction(MachineFunction &MF) {
    if (!XCFIEnabled()) return true;
    if (XCFI::mode() == XCFI::Backward) return true;
    if (MF.getName() == "XCFI_fault") return true;

    const TargetInstrInfo* TII = MF.getSubtarget().getInstrInfo();
    const TargetRegisterInfo* TRI = MF.getSubtarget().getRegisterInfo();

    bool hasIndirectCall = false; 
    std::vector<MachineInstr*> IndirectCalls, IndirectJumps, BTIs, DirectCalls;
    for (MachineBasicBlock &MBB: MF) {
        for (MachineInstr &MI: MBB) {
            switch (MI.getOpcode()) {
                // indirect jump
                case ARM::tBRIND:
                case ARM::tBX:
                case ARM::tBXNS:
                    IndirectJumps.push_back(&MI);
                    break;

                // indirect call
                case ARM::tBLXr:
                case ARM::tBLXNSr:
                case ARM::tBX_CALL:
                case ARM::tTAILJMPr:
                    hasIndirectCall = true;
                    IndirectCalls.push_back(&MI);
                    break;
                
                case ARM::t2BTI:
                    BTIs.push_back(&MI);
                    break;

                case ARM::tBL:
                case ARM::tBLXi:
                case ARM::tTAILJMPd:
                case ARM::tTAILJMPdND:
                case ARM::tSVC:
                case ARM::t2SMC:
                case ARM::t2HVC:
                    DirectCalls.push_back(&MI);
                    break;
            }
        }
    }

    Function &F = MF.getFunction();
    FunctionType *FTy = F.getFunctionType();
    uint16_t SID = FTy->getSID();
    uint16_t UID = hash_value(F.getName());
    if(UID == 0xf3af || UID == 0xe97f) UID = hash_value(UID); // 0xf3af is bti, 0xe97f is sg
    uint32_t CID = (UID << 16) | SID;
    
    // evaluation only
    CID = 0xdeadbeef;
    SID = CID & 0xffff;
    UID = CID >> 16;
    // evaluation

    /* Indirect Calls
     * mov r10, #target_sid
     * push scratchReg
     * ttt scratchReg, targetReg
     * cbz scratchReg, 1f
     * ldr scratchReg, [targetReg, #-5]
     * uxth scratchReg, scratchReg
     * sub scratchReg, r10
     * bne XCFI_fault
     * 1:
     * blx targetReg
     * pop scratchReg
     */
    for (MachineInstr *MI: IndirectCalls) {
        const DebugLoc &DL = MI->getDebugLoc();
        std::vector<MachineInstr*> Insts;
        std::vector<MachineInstr*> InstsAfter;

        uint32_t targetSID = MI->getSID();
        
        // evaluation only
        targetSID = 0xbeef;
        // evaluation

        Register targetReg;
        if (MI->getOpcode() == ARM::tBLXr || MI->getOpcode() == ARM::tBLXNSr)
            targetReg = MI->getOperand(2).getReg();
        else
            targetReg = MI->getOperand(0).getReg();

        std::vector<Register> ScratchRegs = findFreeRegistersBefore(*MI, targetReg);
        Register ScratchReg1, ScratchReg2;

        bool isScratchReg1Pushed = false;
        bool isScratchReg2Pushed = false;
        if (ScratchRegs.empty() || ScratchRegs.size() < 2) {
            ScratchReg1 = ARM::R4;
            Insts.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                            .addExternalSymbol(MF.createExternalSymbolName("str "+ std::string(TRI->getName(ScratchReg1)) + ", [sp, #-4]\n"))
                            .addImm(1)
                            .addExternalSymbol(""));
            // push scratchReg

            isScratchReg1Pushed = true;

            if (ScratchRegs.empty()) {
                ScratchReg2 = ARM::R5;
                Insts.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                                .addExternalSymbol(MF.createExternalSymbolName("str "+ std::string(TRI->getName(ScratchReg2)) + ", [sp, #-8]\n"))
                                .addImm(1)
                                .addExternalSymbol(""));
                isScratchReg2Pushed = true;
            } else {
                ScratchReg2 = ScratchRegs[0];
            }
        }else{
            ScratchReg1 = ScratchRegs[0];
            ScratchReg2 = ScratchRegs[1];
        }
        
        Insts.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                .addExternalSymbol(MF.createExternalSymbolName("mrs "+ std::string(TRI->getName(ScratchReg1)) + ", primask\ncpsid i\n"))
                .addImm(1)
                .addExternalSymbol(""));

        // mov r10, #target_sid
        Insts.push_back(BuildMI(MF, DL, TII->get(ARM::t2MOVi16), ARM::R10).addImm(targetSID));

        // ttt scratchReg, targetReg
        if (TrustZoneCFIEnabled) {
            // ttt scratchReg, targetReg
            Insts.push_back(BuildMI(MF, DL, TII->get(ARM::t2TTT), ScratchReg2).addReg(targetReg));
            // cbz scratchReg, 1f
            Insts.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                .addExternalSymbol(MF.createExternalSymbolName("cmp "+ std::string(TRI->getName(ScratchReg2)) + ", #0\n" + "beq 1f\n"))
                .addImm(0)
                .addExternalSymbol(""));
        }
        // ldr scratchReg, [targetReg, #-5]
        Insts.push_back(BuildMI(MF, DL, TII->get(ARM::t2LDRi8))
                .addReg(ScratchReg2)
                .addReg(targetReg)
                .addImm(-5)
                .add(predOps(ARMCC::AL, 0)));

        // uxth scratchReg, scratchReg
        Insts.push_back(BuildMI(MF, DL, TII->get(ARM::t2UXTH), ScratchReg2).addReg(ScratchReg2).addImm(0).add(predOps(ARMCC::AL, 0)));
        // sub scratchReg, r10
        Insts.push_back(BuildMI(MF, DL, TII->get(ARM::t2SUBrr))
                .addReg(ScratchReg2, RegState::Define)
                .addReg(ScratchReg2)
                .addReg(ARM::R10)
                .add(predOps(ARMCC::AL))
                .addReg(ARM::CPSR, RegState::Define));
        // bne XCFI_fault
        if (TrustZoneCFIEnabled) {
            // 1:
            Insts.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                .addExternalSymbol(MF.createExternalSymbolName("bne XCFI_fault\n1:\n"))
                .addImm(1)
                .addExternalSymbol(""));
        } else {
            Insts.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                .addExternalSymbol(MF.createExternalSymbolName("bne XCFI_fault\n"))
                .addImm(1)
                .addExternalSymbol(""));
        }

        Insts.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                .addExternalSymbol(MF.createExternalSymbolName("msr primask, "+ std::string(TRI->getName(ScratchReg1)) + "\n"))
                .addImm(1)
                .addExternalSymbol(""));

        if (isScratchReg1Pushed) {
            // pop scratchReg
            Insts.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                            .addExternalSymbol(MF.createExternalSymbolName("ldr "+ std::string(TRI->getName(ScratchReg1)) + ", [sp, #-4]\n"))
                            .addImm(1)
                            .addExternalSymbol(""));
            isScratchReg1Pushed = false;
        }
        if (isScratchReg2Pushed) {
            // pop scratchReg
            Insts.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                            .addExternalSymbol(MF.createExternalSymbolName("ldr "+ std::string(TRI->getName(ScratchReg2)) + ", [sp, #-8]\n"))
                            .addImm(1)
                            .addExternalSymbol(""));
            isScratchReg2Pushed = false;
        }

        insertInstsBefore(*MI, Insts);
        insertInstsAfter(*MI, InstsAfter);
    }

    if (TrustZoneCFIEnabled) {
        for (MachineInstr *MI: DirectCalls) {
            const DebugLoc &DL = MI->getDebugLoc();

            StringRef SymName;
            const GlobalValue *GV = nullptr;
            for (const MachineOperand &MO: MI->operands()) {
                if (MO.isGlobal()) {
                    GV = MO.getGlobal();
                    if (GV) SymName = GV->getName();

                    if (const Function *TargetFunc = dyn_cast<Function>(GV)) {
                        if (TargetFunc->hasFnAttribute("cmse_nonsecure_entry")) {
                            FunctionType *FTy = TargetFunc->getFunctionType();
                            uint32_t targetTID = FTy->getSID() & 0xffff;
                            MachineInstr *Inst = BuildMI(MF, DL, TII->get(ARM::t2MOVi16), ARM::R8).addImm(targetTID);
                            insertInstBefore(*MI, Inst);
                        }
                    }
                }
            }

        }
    }

    return true;
}

namespace llvm {
    FunctionPass *createARMXCFIForwardCFI(void) {
        return new ARMXCFIForwardCFI();
    }
}
