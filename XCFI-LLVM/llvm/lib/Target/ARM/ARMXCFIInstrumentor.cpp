#include "ARMXCFIInstrumentor.h"
#include "ARMBaseInstrInfo.h"
#include "ARMSubtarget.h"
#include "MCTargetDesc/ARMBaseInfo.h"
#include "MCTargetDesc/ARMMCTargetDesc.h"
#include "Utils/ARMBaseInfo.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/LiveInterval.h"
#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/MC/MCRegister.h"
#include <cassert>
#include <deque>

using namespace llvm;

unsigned ARMXCFIInstrumentor::getITBlockSize(const MachineInstr &IT) {
    assert(IT.getOpcode() == ARM::t2IT && "Not an IT instruction");

    unsigned Mask = IT.getOperand(1).getImm() & 0xf;
    assert(Mask != 0 && "Invalid IT mask");

    if (Mask & 0x1) return 4;
    else if (Mask & 0x2) return 3;
    else if (Mask & 0x4) return 2;
    else return 1;
}

MachineInstr* ARMXCFIInstrumentor::findIT(MachineInstr &MI, unsigned &distance) {
    MachineInstr* Prev = &MI;
    unsigned dist = 0;
    while (Prev != nullptr && dist < 5 && Prev->getOpcode() != ARM::t2IT) {
        if (!Prev->isMetaInstruction()) ++dist;
        Prev = Prev->getPrevNode();
    }

    if (Prev != nullptr && dist < 5 && Prev->getOpcode() == ARM::t2IT) {
        if (getITBlockSize(*Prev) >= dist) {
            distance = dist;
            return Prev;
        }
    }
    return nullptr;
}

const MachineInstr* ARMXCFIInstrumentor::findIT(const MachineInstr &MI, unsigned &distance) {
    return findIT(const_cast<MachineInstr &>(MI), distance);
}

std::deque<bool> ARMXCFIInstrumentor::decodeITMask(unsigned Mask) {
    Mask &= 0xf;
    assert(Mask != 0 && "Invalid IT mask");

    std::deque<bool> DQMask { true };
    unsigned size = 4;
    for (unsigned i=0x1; i<0x10; i<<=1) {
        if (Mask & i) break;
        --size;
    }

    for (unsigned i=3; i>4-size; --i) 
        DQMask.push_back((Mask & (1 << i)) == 0);

    return DQMask;
}

unsigned ARMXCFIInstrumentor::encodeITMask(std::deque<bool> DQMask) {
    assert(!DQMask.empty() && "Invalid deque representation of an IT mask");
    assert(DQMask.size() <= 4 && "Invalid deque representation of an IT mask");
    assert(DQMask[0] && "Invalid deque representation of an IT mask");

    unsigned Mask = 0;
    for (unsigned i=1; i<DQMask.size(); ++i) {
        Mask |= DQMask[i] ? 0 : 1;
        Mask <<= 1;
    }
    Mask |= 1;
    Mask <<= (4 - DQMask.size());

    return Mask;
}

void ARMXCFIInstrumentor::insertInstBefore(MachineInstr &MI, MachineInstr *Inst) {
    insertInstsBefore(MI, { Inst });
}

void ARMXCFIInstrumentor::insertInstAfter(MachineInstr &MI, MachineInstr *Inst) {
    insertInstsAfter(MI, { Inst });
}

void ARMXCFIInstrumentor::insertInstsBefore(MachineInstr &MI, ArrayRef<MachineInstr*> Insts) {
    assert(!MI.isMetaInstruction() && "Cannot instrument meta instruction");

    MachineFunction &MF = *MI.getMF();
    MachineBasicBlock &MBB = *MI.getParent();
    const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();

    unsigned distance;
    MachineInstr* IT = findIT(MI, distance);

    for (MachineInstr* Inst: Insts) 
        MBB.insert(MI, Inst);

    if (IT != nullptr && distance != 0) {
        unsigned ITBlockSize = getITBlockSize(*IT);
        unsigned Mask = IT->getOperand(1).getImm() & 0xf;
        ARMCC::CondCodes firstCond = (ARMCC::CondCodes) IT->getOperand(0).getImm();
        std::deque<bool> DQMask = decodeITMask(Mask);
        bool sameAsFirstCond = DQMask[distance - 1];

        MachineBasicBlock::iterator firstMI(IT->getNextNode());
        MachineBasicBlock::iterator lastMI(MI);
        for (unsigned i=distance; i<=ITBlockSize; ) {
            ++lastMI;
            if (i == ITBlockSize || !lastMI->isMetaInstruction()) ++i;
        }

        auto it = DQMask.begin();
        for (unsigned i=0; i<distance-1; ++i) it++;
        size_t NumRealInsts = Insts.size();
        for (MachineInstr* Inst: Insts) {
            if (Inst->isMetaInstruction()) --NumRealInsts;
        }
        DQMask.insert(it, NumRealInsts, sameAsFirstCond);

        for (MachineBasicBlock::iterator i(firstMI); i!=lastMI; ) {
            std::deque<bool> NewDQMask;
            MachineBasicBlock::iterator j(i);
            for (unsigned k=0; k<4 && j!=lastMI; ++j) {
                if (j->isMetaInstruction()) continue;
                NewDQMask.push_back(DQMask.front());
                DQMask.pop_front();
                ++k;
            }
            bool flip = false;
            if (!NewDQMask[0]) {
                for (unsigned k=0; k<NewDQMask.size(); ++k)
                    NewDQMask[k] = !NewDQMask[k];
                flip = true;
            }
            BuildMI(MBB, i, IT->getDebugLoc(), TII->get(ARM::t2IT))
                .addImm(flip ? ARMCC::getOppositeCondition(firstCond) : firstCond)
                .addImm(encodeITMask(NewDQMask));
            i = j;
        }

        IT->eraseFromParent();
    }
}

void ARMXCFIInstrumentor::insertInstsAfter(MachineInstr &MI, ArrayRef<MachineInstr *> Insts) {
    assert(!MI.isMetaInstruction() && "Cannot instrument meta instruction");

    MachineFunction &MF = *MI.getMF();
    MachineBasicBlock &MBB = *MI.getParent();
    const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
    MachineBasicBlock::iterator NextMI(MI); ++NextMI;

    unsigned distance;
    MachineInstr* IT = findIT(MI, distance);

    for (MachineInstr* Inst: Insts)
        MBB.insert(NextMI, Inst);

    if (IT != nullptr && distance != 0) {
        unsigned ITBlockSize = getITBlockSize(*IT);
        unsigned Mask = IT->getOperand(1).getImm() & 0xf;
        ARMCC::CondCodes firstCond = (ARMCC::CondCodes) IT->getOperand(0).getImm();
        std::deque<bool> DQMask = decodeITMask(Mask);
        bool sameAsFirstCond = DQMask[distance - 1];

        MachineBasicBlock::iterator firstMI(IT->getNextNode());
        MachineBasicBlock::iterator lastMI(Insts.back());
        for (unsigned i=distance; i<=ITBlockSize; ) {
            ++lastMI;
            if (i == ITBlockSize || !lastMI->isMetaInstruction()) ++i;
        }

        auto it = DQMask.begin();
        for (unsigned i=0; i<=distance-1; ++i) it++;
        size_t NumRealInsts = Insts.size();
        for (MachineInstr* Inst: Insts) {
            if (Inst->isMetaInstruction()) --NumRealInsts;
        }
        DQMask.insert(it, NumRealInsts, sameAsFirstCond);

        for (MachineBasicBlock::iterator i(firstMI); i!=lastMI; ) {
            std::deque<bool> NewDQMask;
            MachineBasicBlock::iterator j(i);
            for (unsigned k=0; k<4 && j!=lastMI; ++j) {
                if (j->isMetaInstruction()) continue;
                NewDQMask.push_back(DQMask.front());
                DQMask.pop_front();
                ++k;
            }
            bool flip = false;
            if (!NewDQMask[0]) {
                for (unsigned k=0; k<NewDQMask.size(); ++k)
                    NewDQMask[k] = !NewDQMask[k];
                flip = true;
            }
            BuildMI(MBB, i, IT->getDebugLoc(), TII->get(ARM::t2IT))
                .addImm(flip ? ARMCC::getOppositeCondition(firstCond) : firstCond)
                .addImm(encodeITMask(NewDQMask));
            i = j;
        }

        IT->eraseFromParent();
    }
}

void ARMXCFIInstrumentor::removeInst(MachineInstr &MI) {
    assert(!MI.isMetaInstruction() && "Cannot instrument meta instruction");

    unsigned distance;
    MachineInstr *IT = findIT(MI, distance);
   
    if (IT != nullptr) {
        assert(distance != 0 && "Cannot remove an IT instruction directly");

        unsigned Mask = IT->getOperand(1).getImm() & 0xf;
        ARMCC::CondCodes firstCond = (ARMCC::CondCodes) IT->getOperand(0).getImm();
        std::deque<bool> DQMask = decodeITMask(Mask);

        auto it = DQMask.begin();
        for (unsigned i=0; i<distance-1; ++i) it++;
        DQMask.erase(it);

        if (DQMask.empty()) IT->eraseFromParent();
        else {
            if (!DQMask[0]) {
                for (unsigned i=0; i<DQMask.size(); ++i)
                    DQMask[i] = !DQMask[i];
                IT->getOperand(0).setImm(ARMCC::getOppositeCondition(firstCond));
            }
            IT->getOperand(1).setImm(encodeITMask(DQMask));
        }
    }

    MI.eraseFromParent();
}

bool ARMXCFIInstrumentor::isRegFreeBefore(Register Reg, const MachineInstr &MI, bool Thumb) {
    std::vector<Register> FreeRegs = findFreeRegistersBefore(MI, ARM::R10);
    auto it = std::find(FreeRegs.begin(), FreeRegs.end(), Reg);
    if (it == FreeRegs.end()) return false;
    else return true;
}

bool ARMXCFIInstrumentor::isRegFreeAfter(Register Reg, const MachineInstr &MI, bool Thumb) {
    std::vector<Register> FreeRegs = findFreeRegistersAfter(MI, ARM::R10);
    auto it = std::find(FreeRegs.begin(), FreeRegs.end(), Reg);
    if (it == FreeRegs.end()) return false;
    else return true;
}


Register ARMXCFIInstrumentor::findFreeRegisterBefore(const MachineInstr &MI, bool Thumb) {
    std::vector<Register> FreeRegs = findFreeRegistersBefore(MI, Thumb);
    if (FreeRegs.empty()) return 0;
    else return findFreeRegistersBefore(MI, Thumb).front();
}

Register ARMXCFIInstrumentor::findFreeRegisterAfter(const MachineInstr &MI, bool Thumb) {
    std::vector<Register> FreeRegs = findFreeRegistersAfter(MI, Thumb);
    if (FreeRegs.empty()) return 0;
    else return findFreeRegistersAfter(MI, Thumb).front();
}

std::vector<Register> ARMXCFIInstrumentor::findFreeRegistersBefore(const MachineInstr &MI, Register ExcludeReg, bool Thumb) {
    assert(!MI.isMetaInstruction() && "Cannot instrument meta instruction!");

    unsigned distance;
    const MachineInstr* IT = findIT(MI, distance);

    Register PredReg;
    ARMCC::CondCodes Pred = getInstrPredicate(MI, PredReg);

    const MachineFunction& MF = *MI.getMF();
    const MachineBasicBlock& MBB = *MI.getParent();
    const MachineRegisterInfo& MRI = MF.getRegInfo();
    const TargetRegisterInfo* TRI = MF.getSubtarget().getRegisterInfo();
    LivePhysRegs UsedRegs(*TRI);

    UsedRegs.addLiveOuts(MBB);

    MachineBasicBlock::const_iterator MBBI(MI);
    MachineBasicBlock::const_iterator I = MBB.end();
    while (I != MBBI) {
        unsigned distance2;
        const MachineInstr* IT2 = findIT(*--I, distance2);
        Register PredReg2;
        ARMCC::CondCodes Pred2 = getInstrPredicate(*I, PredReg2);

        if (IT2 != nullptr && IT == IT2) {
            if (Pred != Pred2) continue;
        }

        if (I->isReturn()) {
            UsedRegs.init(*TRI);
            for (auto CSR = MRI.getCalleeSavedRegs(); CSR && *CSR; ++CSR) 
                UsedRegs.addReg(*CSR);
        }

        UsedRegs.stepBackward(*I);
    }

    const auto LoGPRs = {
        ARM::R0, ARM::R1, ARM::R2, ARM::R3, ARM::R4, ARM::R5, ARM::R6, ARM::R7,
    };
    const auto HiGPRs = {
        ARM::R8, ARM::R9, ARM::R10, ARM::R11, ARM::R12, ARM::LR,
    };
    std::vector<Register> FreeRegs;
    for (Register Reg: LoGPRs) {
        if (!MRI.isReserved(Reg) && !UsedRegs.contains(Reg))
            FreeRegs.push_back(Reg);
    }
    if (!Thumb) {
        for (Register Reg: HiGPRs)
            if (!MRI.isReserved(Reg) && !UsedRegs.contains(Reg))
                FreeRegs.push_back(Reg);
    }

    FreeRegs.erase(std::remove(FreeRegs.begin(), FreeRegs.end(), ExcludeReg), FreeRegs.end());
   
    return FreeRegs;
}

std::vector<Register> ARMXCFIInstrumentor::findFreeRegistersAfter(const MachineInstr &MI, Register ExcludeReg, bool Thumb) {
    assert(!MI.isMetaInstruction() && "Cannot instrument meta instruction!");

    unsigned distance;
    const MachineInstr* IT = findIT(MI, distance);

    Register PredReg;
    ARMCC::CondCodes Pred = getInstrPredicate(MI, PredReg);

    const MachineFunction& MF = *MI.getMF();
    const MachineBasicBlock& MBB = *MI.getParent();
    const MachineRegisterInfo& MRI = MF.getRegInfo();
    const TargetRegisterInfo* TRI = MF.getSubtarget().getRegisterInfo();
    LivePhysRegs UsedRegs(*TRI);

    UsedRegs.addLiveOuts(MBB);

    MachineBasicBlock::const_iterator Terminator = MBB.getLastNonDebugInstr();
    if (Terminator != MBB.end() && Terminator->isReturn())
        UsedRegs.addUses(*Terminator);

    MachineBasicBlock::const_iterator MBBI(MI);
    MachineBasicBlock::const_iterator I = MBB.end();
    while (I != MBBI) {
        unsigned distance2;
        const MachineInstr* IT2 = findIT(*--I, distance2);
        Register PredReg2;
        ARMCC::CondCodes Pred2 = getInstrPredicate(*I, PredReg2);

        if (IT2 != nullptr && IT == IT2) {
            if (Pred != Pred2) continue;
        }

        if (I->isReturn()) {
            UsedRegs.init(*TRI);
            for (auto CSR = MRI.getCalleeSavedRegs(); CSR && *CSR; ++CSR) 
                UsedRegs.addReg(*CSR);
            UsedRegs.addUses(*I);
        }

        if (I != MBBI)
            UsedRegs.stepBackward(*I);
    }

    const auto LoGPRs = {
        ARM::R0, ARM::R1, ARM::R2, ARM::R3, ARM::R4, ARM::R5, ARM::R6, ARM::R7,
    };
    const auto HiGPRs = {
        ARM::R8, ARM::R9, ARM::R10, ARM::R11, ARM::R12, ARM::LR,
    };
    std::vector<Register> FreeRegs;
    for (Register Reg: LoGPRs) {
        if (!MRI.isReserved(Reg) && !UsedRegs.contains(Reg))
            FreeRegs.push_back(Reg);
    }
    if (!Thumb) {
        for (Register Reg: HiGPRs)
            if (!MRI.isReserved(Reg) && !UsedRegs.contains(Reg))
                FreeRegs.push_back(Reg);
    }
   
    return FreeRegs;
}

void ARMXCFIInstrumentor::backupRegister(MachineInstr &MI, Register Reg, std::vector<MachineInstr *> &Insts) {
    MachineFunction &MF = *MI.getMF();
    const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();

    Register PredReg;
    ARMCC::CondCodes Pred = getInstrPredicate(MI, PredReg);
    const DebugLoc &DL = MI.getDebugLoc();

    Insts.push_back(BuildMI(MF, DL, TII->get(ARM::t2STMDB_UPD))
            .addReg(ARM::SP, RegState::Define)
            .addReg(ARM::SP)
            .add(predOps(ARMCC::AL))
            .addReg(Reg));
}


void ARMXCFIInstrumentor::restoreRegister(MachineInstr &MI, Register Reg, std::vector<MachineInstr *> &Insts) {
    MachineFunction &MF = *MI.getMF();
    const TargetInstrInfo* TII = MF.getSubtarget().getInstrInfo();

    Register PredReg;
    ARMCC::CondCodes Pred = getInstrPredicate(MI, PredReg);
    const DebugLoc &DL = MI.getDebugLoc();

    Insts.push_back(BuildMI(MF, DL, TII->get(ARM::t2LDMIA_UPD))
            .addReg(ARM::SP, RegState::Define)
            .addReg(ARM::SP)
            .add(predOps(ARMCC::AL))
            .addReg(Reg));
}


