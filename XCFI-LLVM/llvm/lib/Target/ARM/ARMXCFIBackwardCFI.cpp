#include "ARMXCFIBackwardCFI.h"
#include "ARMXCFIOptions.h"
#include "ARMXCFIInstrumentor.h"
#include "ARMBaseInstrInfo.h"
#include "ARMMachineFunctionInfo.h"
#include "MCTargetDesc/ARMMCTargetDesc.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Pass.h"
#include <string>

using namespace llvm;
using namespace llvm::XCFI;

extern bool SecureCFIEnabled;

bool isSecureFunc = false;

char ARMXCFIBackwardCFI::ID = 0;
ARMXCFIBackwardCFI::ARMXCFIBackwardCFI(): MachineFunctionPass(ID) {}

bool ARMXCFIBackwardCFI::runOnMachineFunction(MachineFunction &MF) {
    if (!XCFIEnabled()) return true;
    if (XCFI::mode() == XCFI::Forward) return true;
    if (MF.getName() == "XCFI_fault") return true;

    const TargetInstrInfo* TII = MF.getSubtarget().getInstrInfo();
    const TargetRegisterInfo* TRI = MF.getSubtarget().getRegisterInfo();

    bool hasIndirectJump = false; 
    std::vector<MachineInstr*> PACs, AUTs;
    for (MachineBasicBlock &MBB: MF) {
        for (MachineInstr &MI: MBB) {
            switch (MI.getOpcode()) {
                case ARM::t2PAC:
                    PACs.push_back(&MI);
                    break;
                case ARM::t2AUT:
                    AUTs.push_back(&MI);
                    break;
            }
        }
    }

    Function &F = MF.getFunction();
    FunctionType *FTy = F.getFunctionType();
    ARMFunctionInfo *AFI = MF.getInfo<ARMFunctionInfo>();
    
    uint16_t SID = FTy->getSID();
    uint16_t UID = hash_value(F.getName());
    if(UID == 0xf3af || UID == 0xe97f) UID = hash_value(UID); // 0xf3af is bti, 0xe97f is sg
    uint32_t CID = (UID << 16) | SID;
    
    // evaluation only
    CID = 0xdeadbeef;
    SID = CID & 0xffff;
    UID = CID >> 16;
    // evaluation

    /* ldr r10, [pc, #-c]
     * pacg r12, lr, r10
     */
    for (MachineInstr *MI: PACs) {
        const DebugLoc &DL = MI->getDebugLoc();
        std::vector<MachineInstr*> Insts;

        std::string asmStr;

        if (XCFI::mode() == XCFI::All)
            asmStr = "ldr r10, [pc, #-12]";
        else
            asmStr = "ldr r10, [pc, #-8]";

        Insts.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                .addExternalSymbol(MF.createExternalSymbolName(asmStr))
                .addImm(1)
                .addExternalSymbol(""));

        if (SecureCFIEnabled && AFI->isCmseNSEntryFunction()) {
            std::vector<Register> ScratchRegs = findFreeRegistersBefore(*MI, ARM::R10);
            Register ScratchReg1, ScratchReg2;

            bool isScratchReg1Pushed = false;
            bool isScratchReg2Pushed = false;
            if (ScratchRegs.empty() || ScratchRegs.size() < 2) {
                ScratchReg1 = ARM::R4;
                Insts.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                                .addExternalSymbol(MF.createExternalSymbolName("str "+ std::string(TRI->getName(ScratchReg1)) + ", [sp, #-4]\n"))
                                .addImm(1)
                                .addExternalSymbol(""));
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
            
            asmStr = "movw "+ std::string(TRI->getName(ScratchReg1)) +", #0xd468\n"
                     "movt "+ std::string(TRI->getName(ScratchReg1)) +", #0x3100\n"
                     "cmp sp, "+ std::string(TRI->getName(ScratchReg1)) +"\n"
                     "beq 1f\n"
                     "ldr "+ std::string(TRI->getName(ScratchReg1)) +", [sp]\n"
                     "ldr "+ std::string(TRI->getName(ScratchReg2)) +", [sp, #4]\n"
                     "eor r10, r10, "+ std::string(TRI->getName(ScratchReg1)) +"\n"
                     "eor r10, r10, "+ std::string(TRI->getName(ScratchReg2)) +"\n"
                     "1:\n";
            Insts.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                    .addExternalSymbol(MF.createExternalSymbolName(asmStr))
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
        }

        // pacg r12, lr, r10
        Insts.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                .addExternalSymbol("pacg r12, lr, r10")
                .addImm(1)
                .addExternalSymbol(""));
        
        insertInstsBefore(*MI, Insts);
        removeInst(*MI);
    }

    /*
     * movw r10, #sid
     * movt r10, #uid
     * autg r12, lr, r10
     */
    for (MachineInstr *MI: AUTs) {
       const DebugLoc &DL = MI->getDebugLoc();
       std::vector<MachineInstr*> Insts;
       std::string asmStr;
       
        // movw r10, #sid
        Insts.push_back(BuildMI(MF, DL, TII->get(ARM::t2MOVi16), ARM::R10).addImm(SID));
        // movt r10, #uid
        Insts.push_back(BuildMI(MF, DL, TII->get(ARM::t2MOVTi16), ARM::R10).addReg(ARM::R10).addImm(UID));

        if (SecureCFIEnabled && AFI->isCmseNSEntryFunction()) {
            std::vector<Register> ScratchRegs = findFreeRegistersBefore(*MI, ARM::R10);
            Register ScratchReg1, ScratchReg2;

            bool isScratchReg1Pushed = false;
            bool isScratchReg2Pushed = false;
            if (ScratchRegs.empty() || ScratchRegs.size() < 2) {
                ScratchReg1 = ARM::R4;
                Insts.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                                .addExternalSymbol(MF.createExternalSymbolName("str "+ std::string(TRI->getName(ScratchReg1)) + ", [sp, #-4]\n"))
                                .addImm(1)
                                .addExternalSymbol(""));
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

            asmStr = "movw "+ std::string(TRI->getName(ScratchReg1)) +", #0xd468\n"
                     "movt "+ std::string(TRI->getName(ScratchReg1)) +", #0x3100\n"
                     "cmp sp, "+ std::string(TRI->getName(ScratchReg1)) +"\n"
                     "beq 1f\n"
                     "ldr "+ std::string(TRI->getName(ScratchReg1)) +", [sp]\n"
                     "ldr "+ std::string(TRI->getName(ScratchReg2)) +", [sp, #4]\n"
                     "eor r10, r10, "+ std::string(TRI->getName(ScratchReg1)) +"\n"
                     "eor r10, r10, "+ std::string(TRI->getName(ScratchReg2)) +"\n"
                     "1:\n";
            Insts.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                    .addExternalSymbol(MF.createExternalSymbolName(asmStr))
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
        }

        // autg r12, lr, r10
        Insts.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
               .addExternalSymbol("autg r12, lr, r10")
               .addImm(1)
               .addExternalSymbol(""));
       
       insertInstsBefore(*MI, Insts);
       removeInst(*MI);
    }

    return true;
}

namespace llvm {
    FunctionPass *createARMXCFIBackwardCFI(void) {
        return new ARMXCFIBackwardCFI();
    }
}
