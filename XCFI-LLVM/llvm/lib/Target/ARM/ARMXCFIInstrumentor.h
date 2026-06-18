#ifndef ARM_XCFI_INSTRUMENTOR
#define ARM_XCFI_INSTRUMENTOR

#include "llvm/ADT/ArrayRef.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/Register.h"
#include <deque>

namespace llvm {
	struct ARMXCFIInstrumentor {
		void insertInstBefore(MachineInstr &MI, MachineInstr* Inst);
		void insertInstAfter(MachineInstr &MI, MachineInstr* Inst);
		void insertInstsBefore(MachineInstr &MI, ArrayRef<MachineInstr*> Insts);
		void insertInstsAfter(MachineInstr &MI, ArrayRef<MachineInstr*> Insts);
		void removeInst(MachineInstr &MI);
		
		std::vector<Register> findFreeRegistersBefore(const MachineInstr &MI, Register ExcludeReg, bool Thumb = false);
		std::vector<Register> findFreeRegistersAfter(const MachineInstr &MI, Register ExcludeReg, bool Thumb = false);
		Register findFreeRegisterBefore(const MachineInstr &MI, bool Thumb = false);
		Register findFreeRegisterAfter(const MachineInstr &MI, bool Thumb = false);
		bool isRegFreeBefore(Register Reg, const MachineInstr &MI, bool Thumb = false);
		bool isRegFreeAfter(Register Reg, const MachineInstr &MI, bool Thumb = false);

		void backupRegister(MachineInstr &MI, Register Reg, std::vector<MachineInstr*> &Insts);
		void restoreRegister(MachineInstr &MI, Register Reg, std::vector<MachineInstr*> &Insts);

		private:
			unsigned getITBlockSize(const MachineInstr &IT);
			MachineInstr* findIT(MachineInstr &MI, unsigned &distance);
			const MachineInstr *findIT(const MachineInstr &MI, unsigned &distance);
			std::deque<bool> decodeITMask(unsigned Mask);
			unsigned encodeITMask(std::deque<bool> DQMask);
	};
}

#endif
