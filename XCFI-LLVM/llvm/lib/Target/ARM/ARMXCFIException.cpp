#include "ARMXCFIException.h"
#include "ARMXCFIOptions.h"
#include "ARMXCFIInstrumentor.h"
#include "ARMBaseInstrInfo.h"
#include "Utils/ARMBaseInfo.h"
#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/Pass.h"

using namespace llvm;
using namespace llvm::XCFI;

extern bool TrustZoneCFIEnabled;

char ARMXCFIException::ID = 0;
ARMXCFIException::ARMXCFIException(): MachineFunctionPass(ID) {}

static const std::vector<std::string> handlers = {
    "z_arm_svc",
    "z_arm_debug_monitor",
    "z_arm_pendsv",
    "sys_clock_isr",
};

/*
* prologue instrumentation code
* tst lr, #0x40
* it eq
* pacgeq r12, r10, lr
* beq 1f
* tst lr, #0x4
* ite eq
* mrseq r0, msp_ns
* mrsne r0, psp_ns
* add r0, #20
* ldmia r0, {R1-R3}
* eor r1, r1, r2
* eor r1, r1, r3
* eor r1, r1, lr
* pacg r12, r10, r1
* 1:
*/
void ARMXCFIException::insertPrologueInstrumentation(MachineFunction &MF, MachineInstr &MI) {
    const TargetInstrInfo* TII = MF.getSubtarget().getInstrInfo();
    MachineFrameInfo &MFI = MF.getFrameInfo();

    Register PredReg;
    ARMCC::CondCodes Pred = getInstrPredicate(MI, PredReg);

    std::vector<MachineInstr *> NewMIs;
    const DebugLoc &DL = MI.getDebugLoc();
    
    Register ScratchReg;
    ScratchReg = ARM::R0;

    unsigned RegNo = ScratchReg.id() - 73;

    std::string asmStr;

    if (XCFI::mode() == XCFI::All){
        asmStr = "ldr r10, [pc, #-12]";
        
    }

    NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
            .addExternalSymbol(MF.createExternalSymbolName(asmStr))
            .addImm(1)
            .addExternalSymbol(""));
        
    if(TrustZoneCFIEnabled) {
        NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::t2TSTri), ARM::LR)
                                .addImm(64)
                                .add(predOps(Pred, PredReg)));
        NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM)) //pacgne, bne
                                .addExternalSymbol(MF.createExternalSymbolName("it ne\npacgne r12, r10, lr\nbne 1f\n"))
                                .addImm(1)
                                .addExternalSymbol(""));
        NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::t2TSTri), ARM::LR)
                                .addImm(4)
                                .add(predOps(Pred, PredReg)));
        NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                                .addExternalSymbol(MF.createExternalSymbolName("ite eq\nmrseq r0, msp_ns\nmrsne r0, psp_ns\n"))
                                .addImm(1)
                                .addExternalSymbol(""));
    }else{
        NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::t2TSTri), ARM::LR)
                                .addImm(4)
                                .add(predOps(Pred, PredReg)));
        NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                                .addExternalSymbol(MF.createExternalSymbolName("ite eq\nmrseq r0, msp\nmrsne r0, psp\n"))
                                .addImm(1)
                                .addExternalSymbol(""));
    }
    NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::t2ADDri), ARM::R0)
                            .addReg(ARM::R0)
                            .addImm(20)
                            .add(predOps(Pred, PredReg))
                            .add(condCodeOp()));
    NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                            .addExternalSymbol(MF.createExternalSymbolName("ldmia r0!, {r1-r3}\n"))
                            .addImm(1)
                            .addExternalSymbol(""));
    NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::t2EORrr), ARM::R1)
                            .addReg(ARM::R1)
                            .addReg(ARM::R2)
                            .add(predOps(Pred, PredReg))
                            .add(condCodeOp()));
    NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::t2EORrr), ARM::R1)
                            .addReg(ARM::R1)
                            .addReg(ARM::R3)
                            .add(predOps(Pred, PredReg))
                            .add(condCodeOp()));
    NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::t2EORrr), ARM::R1)
                            .addReg(ARM::R1)
                            .addReg(ARM::LR)
                            .add(predOps(Pred, PredReg))
                            .add(condCodeOp()));
    if(TrustZoneCFIEnabled) {
        NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                            .addExternalSymbol(MF.createExternalSymbolName("pacg r12, r10, r1\n1:\n"))
                                .addImm(1)
                                .addExternalSymbol(""));
    }else{
        NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                            .addExternalSymbol(MF.createExternalSymbolName("pacg r12, r10, r1\n"))
                            .addImm(1)
                            .addExternalSymbol(""));
    }

    insertInstsAfter(MI, NewMIs);
}

/*
* epilogue instrumentation code
*
* tst lr, #0x40
* it eq
* autgeq r12, r10, lr
* beq 1f
* tst lr, #0x4
* ite eq
* mrseq r0, msp_ns
* mrsne r0, psp_ns
* add r0, #20
* ldmia r0, {R1-R3}
* eor r1, r1, r2
* eor r1, r1, r3
* eor r1, r1, lr
* autg r12, r1, r10
* 1:
* movw r0, #func_tid
* movt r0, #func_uid
* xor r10, r0
*/
void ARMXCFIException::insertEpilogueInstrumentation(MachineFunction &MF, MachineInstr &MI) {
    const TargetInstrInfo* TII = MF.getSubtarget().getInstrInfo();
    MachineFrameInfo &MFI = MF.getFrameInfo();

    Register PredReg;
    ARMCC::CondCodes Pred = getInstrPredicate(MI, PredReg);

    std::vector<MachineInstr *> NewMIs;
    const DebugLoc &DL = MI.getDebugLoc();
    
    Register ScratchReg;
    ScratchReg = ARM::R0;

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

    if(TrustZoneCFIEnabled) {
        NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::t2TSTri), ARM::LR)
                                .addImm(64)
                                .add(predOps(Pred, PredReg)));
        NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM)) //autgne, bne
                                .addExternalSymbol(MF.createExternalSymbolName("it ne\nautgne r12, r10, lr\nbne 1f\n"))
                                .addImm(1)
                                .addExternalSymbol(""));
        NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::t2TSTri), ARM::LR)
                                .addImm(4)
                                .add(predOps(Pred, PredReg)));
        NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                                .addExternalSymbol(MF.createExternalSymbolName("ite eq\nmrseq r0, msp_ns\nmrsne r0, psp_ns\n"))
                                .addImm(1)
                                .addExternalSymbol(""));
    }else{
        NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::t2TSTri), ARM::LR)
                                .addImm(4)
                                .add(predOps(Pred, PredReg)));
        NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                                .addExternalSymbol(MF.createExternalSymbolName("ite eq\nmrseq r0, msp\nmrsne r0, psp\n"))
                                .addImm(1)
                                .addExternalSymbol(""));
    }
    NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::t2ADDri), ARM::R0)
                            .addReg(ARM::R0)
                            .addImm(20)
                            .add(predOps(Pred, PredReg))
                            .add(condCodeOp()));
    NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                            .addExternalSymbol(MF.createExternalSymbolName("ldmia r0!, {r1-r3}\n"))
                            .addImm(1)
                            .addExternalSymbol(""));
    NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::t2EORrr), ARM::R1)
                            .addReg(ARM::R1)
                            .addReg(ARM::R2)
                            .add(predOps(Pred, PredReg))
                            .add(condCodeOp()));
    NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::t2EORrr), ARM::R1)
                            .addReg(ARM::R1)
                            .addReg(ARM::R3)
                            .add(predOps(Pred, PredReg))
                            .add(condCodeOp()));
    NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::t2EORrr), ARM::R1)
                            .addReg(ARM::R1)
                            .addReg(ARM::LR)
                            .add(predOps(Pred, PredReg))
                            .add(condCodeOp()));
    // movw r10, #sid
    NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::t2MOVi16), ARM::R10).addImm(SID));
    // movt r10, #uid
    NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::t2MOVTi16), ARM::R10).addReg(ARM::R10).addImm(UID));

    if(TrustZoneCFIEnabled) {
        NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                            .addExternalSymbol(MF.createExternalSymbolName("autg r12, r10, r1\n1:"))
                                .addImm(1)
                                .addExternalSymbol(""));
    }else{
        NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                            .addExternalSymbol(MF.createExternalSymbolName("autg r12, r10, r1"))
                            .addImm(1)
                            .addExternalSymbol(""));
    }

    insertInstsAfter(MI, NewMIs);
}

bool ARMXCFIException::runOnMachineFunction(MachineFunction &MF) {
    if (XCFIDisabled() || XCFI::mode() == XCFI::Forward) return true;
    if (std::find(handlers.begin(), handlers.end(), MF.getName()) == handlers.end()) return true;

    std::vector<MachineInstr *> eraseInstrs;
    std::vector<MachineInstr *> prologueInstrs;
    std::vector<MachineInstr *> epilogueInstrs;

    for(MachineBasicBlock &MBB : MF) {
        for (MachineBasicBlock::iterator MBBI = MBB.begin(), MBBE = MBB.end(); MBBI != MBBE; ++MBBI) {
            MachineInstr &MI = *MBBI;
            if (XCFI::mode() == XCFI::Backward && MI == MI.getMF()->front().getFirstNonDebugInstr()){
                prologueInstrs.push_back(&MI);
                MachineBasicBlock::iterator NextIt = MBBI;
                NextIt++;
                eraseInstrs.push_back(&*NextIt);
            }
            switch (MI.getOpcode()) {
                case ARM::t2BTI: {
                    prologueInstrs.push_back(&MI);
                    
                    MachineBasicBlock::iterator NextIt = MBBI;
                    for (int i = 0; i < 2 && NextIt != MBBE; ++i) {
                        ++NextIt;
                        if (NextIt != MBBE) {
                            eraseInstrs.push_back(&*NextIt);
                        }
                    }
                    break;
                }
                case ARM::t2LDMIA_UPD: {
                    if(!MI.getFlag(MachineInstr::FrameDestroy)) break;
                    epilogueInstrs.push_back(&MI);
 
                    MachineBasicBlock::iterator NextIt = MBBI;
                    for (int i = 0; i < 3 && NextIt != MBBE; ++i) {
                        ++NextIt;
                        if (NextIt != MBBE) {
                            eraseInstrs.push_back(&*NextIt);
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
    
    for (MachineInstr *MI : prologueInstrs) {
        insertPrologueInstrumentation(MF, *MI);
    }

    for (MachineInstr *MI : epilogueInstrs) {
        insertEpilogueInstrumentation(MF, *MI);
    }

    for (MachineInstr *MI : eraseInstrs) {
        MI->eraseFromParent();
    }
    
    return true;
}

namespace llvm {
    FunctionPass *createARMXCFIException(void) {
        return new ARMXCFIException();
    }
}
