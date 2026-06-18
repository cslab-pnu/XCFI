#include "ARMXCFISecureException.h"
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

char ARMXCFISecureException::ID = 0;
ARMXCFISecureException::ARMXCFISecureException(): MachineFunctionPass(ID) {}

static const std::vector<std::string> secure_handlers = {
    "DebugMon_Handler", "SysTick_Handler", "PendSV_Handler",
    "NONSEC_WATCHDOG_RESET_REQ_Handler", "NONSEC_WATCHDOG_Handler",
    "SLOWCLK_Timer_Handler", "TFM_TIMER0_IRQ_Handler",
    "TIMER1_Handler", "TIMER2_Handler",
    "MPC_Handler", "PPC_Handler",
    "MSC_Handler", "BRIDGE_ERROR_Handler",
    "COMBINED_PPU_Handler",
    "NPU0_Handler",
    "TIMER3_AON_Handler",
    "CPU0_CTI_0_Handler", "CPU0_CTI_1_Handler",
    "System_Timestamp_Counter_Handler",
    "UARTRX0_Handler", "UARTTX0_Handler",
    "UARTRX1_Handler", "UARTTX1_Handler",
    "UARTRX2_Handler", "UARTTX2_Handler",
    "UARTRX3_Handler", "UARTTX3_Handler",
    "UARTRX4_Handler", "UARTTX4_Handler",
    "UART0_Combined_Handler", "UART1_Combined_Handler",
    "UART2_Combined_Handler", "UART3_Combined_Handler",
    "UART4_Combined_Handler", "UARTOVF_Handler",
    "ETHERNET_Handler", "I2S_Handler",
    "TOUCH_SCREEN_Handler", "USB_Handler", "SPI_ADC_Handler",
    "SPI_SHIELD0_Handler", "SPI_SHIELD1_Handler",
    "DMA_Channel_0_Handler", "DMA_Channel_1_Handler",
    "GPIO0_Combined_Handler", "GPIO1_Combined_Handler",
    "GPIO2_Combined_Handler", "GPIO3_Combined_Handler",
    "GPIO0_0_Handler", "GPIO0_1_Handler", "GPIO0_2_Handler",
    "GPIO0_3_Handler", "GPIO0_4_Handler", "GPIO0_5_Handler",
    "GPIO0_6_Handler", "GPIO0_7_Handler", "GPIO0_8_Handler",
    "GPIO0_9_Handler", "GPIO0_10_Handler", "GPIO0_11_Handler",
    "GPIO0_12_Handler", "GPIO0_13_Handler", "GPIO0_14_Handler", "GPIO0_15_Handler",
    "GPIO1_0_Handler", "GPIO1_1_Handler", "GPIO1_2_Handler",
    "GPIO1_3_Handler", "GPIO1_4_Handler", "GPIO1_5_Handler",
    "GPIO1_6_Handler", "GPIO1_7_Handler", "GPIO1_8_Handler",
    "GPIO1_9_Handler", "GPIO1_10_Handler", "GPIO1_11_Handler",
    "GPIO1_12_Handler", "GPIO1_13_Handler", "GPIO1_14_Handler", "GPIO1_15_Handler",
    "GPIO2_0_Handler", "GPIO2_1_Handler", "GPIO2_2_Handler",
    "GPIO2_3_Handler", "GPIO2_4_Handler", "GPIO2_5_Handler",
    "GPIO2_6_Handler", "GPIO2_7_Handler", "GPIO2_8_Handler",
    "GPIO2_9_Handler", "GPIO2_10_Handler", "GPIO2_11_Handler",
    "GPIO2_12_Handler", "GPIO2_13_Handler", "GPIO2_14_Handler", "GPIO2_15_Handler",
    "GPIO3_0_Handler", "GPIO3_1_Handler",
    "GPIO3_2_Handler", "GPIO3_3_Handler",
    "UARTRX5_Handler", "UARTTX5_Handler", "UART5_Combined_Handler",
    "ARM_VSI0_Handler", "ARM_VSI1_Handler",
    "ARM_VSI2_Handler", "ARM_VSI3_Handler",
    "ARM_VSI4_Handler", "ARM_VSI5_Handler",
    "ARM_VSI6_Handler", "ARM_VSI7_Handler",
};

void ARMXCFISecureException::insertPrologueInstrumentation(MachineFunction &MF, MachineInstr &MI) {
    const TargetInstrInfo* TII = MF.getSubtarget().getInstrInfo();
    MachineFrameInfo &MFI = MF.getFrameInfo();

    Register PredReg;
    ARMCC::CondCodes Pred = getInstrPredicate(MI, PredReg);

    std::vector<MachineInstr *> NewMIs;
    const DebugLoc &DL = MI.getDebugLoc();

    std::string asmString = "ldr r10, [pc, #-12]\n"
                            "tst lr, #0x40\n" // Check S bit
                            "beq 1f\n"  

                            "tst lr, #0x4\n" // Check SPSEL bit
                            "ite ne\n"        // when s_exception called from s-state
                            "mrsne r0, psp\n"
                            "mrseq r0, msp\n"
                            "b 4f\n"
                            
                            "1:\n"            /* S=0: came from NS */
                            /* nested check: compare psp_s with 0x31000000 */
                            "movw r1, #0x0000\n"
                            "movt r1, #0x3100\n"
                            "mrs r0, psp\n"
                            "cmp r0, r1\n"
                            "beq 3f\n"
                            
                            /* Nested */
                            "tst lr, #0x4\n"     // check spsel bit               
                            "beq 2f\n"

                            /* ns_thread and nested */
                            "ldmia r0, {r1, r2}\n"
                            "eor lr, lr, r1\n"
                            "eor lr, lr, r2\n"
                            "mrs r0, psp_ns\n"    
                            "b 4f\n"
                            
                            "2:\n" /* ns_handler and nested */
                            "sub r0, #20\n" 
                            "ldmia r0, {r1-r3}\n"
                            "eor lr, lr, r1\n"
                            "eor lr, lr, r2\n"   
                            "eor lr, lr, r3\n"
                            "mrs r0, msp_ns\n"
                            "b 4f\n"

                            "3:\n" /* ns and no nested */
                            "tst lr, #0x4\n"     // check spsel bit               
                            "ite ne\n"
                            "mrsne r0, psp_ns\n"
                            "mrseq r0, msp_ns\n"

                            "4:\n"
                            "sub r0, #20\n"
                            "ldmia r0, {r1-r3}\n"
                            "eor r1, r1, r2\n"
                            "eor r1, r1, r3\n"
                            "eor r1, r1, lr\n"
                            "pacg r12, r10, r1\n";

    NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                            .addExternalSymbol(MF.createExternalSymbolName(asmString))
                            .addImm(1)
                            .addExternalSymbol(""));

    insertInstsAfter(MI, NewMIs);
}

void ARMXCFISecureException::insertEpilogueInstrumentation(MachineFunction &MF, MachineInstr &MI) {
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

    std::string asmString = "movw r10, #"+std::to_string(SID)+"\n"
                            "movt r10, #"+std::to_string(UID)+"\n"
                            "tst lr, #0x40\n" // Check S bit
                            "beq 5f\n"  

                            "tst lr, #0x4\n" // Check SPSEL bit
                            "ite ne\n"        // when s_exception called from s-state
                            "mrsne r0, psp\n"
                            "mrseq r0, msp\n"
                            "b 8f\n"
                            
                            "5:\n"            /* S=0: came from NS */
                            /* nested check: compare psp_s with 0x31000000 */
                            "movw r1, #0x0000\n"
                            "movt r1, #0x3100\n"
                            "mrs r0, psp\n"
                            "cmp r0, r1\n"
                            "beq 7f\n"
                            
                            /* Nested */
                            "tst lr, #0x4\n"     // check spsel bit               
                            "beq 6f\n"

                            /* ns_thread and nested */
                            "ldmia r0, {r1, r2}\n"
                            "eor lr, lr, r1\n"
                            "eor lr, lr, r2\n"
                            "mrs r0, psp_ns\n"    
                            "b 8f\n"
                            
                            "6:\n" /* ns_handler and nested */
                            "sub r0, #20\n" 
                            "ldmia r0, {r1-r3}\n"
                            "eor lr, lr, r1\n"
                            "eor lr, lr, r2\n"   
                            "eor lr, lr, r3\n"
                            "mrs r0, msp_ns\n"
                            "b 8f\n"

                            "7:\n" /* ns and no nested */
                            "tst lr, #0x4\n"     // check spsel bit               
                            "ite ne\n"
                            "mrsne r0, psp_ns\n"
                            "mrseq r0, msp_ns\n"

                            "8:\n"
                            "sub r0, #20\n"
                            "ldmia r0, {r1-r3}\n"
                            "eor r1, r1, r2\n"
                            "eor r1, r1, r3\n"
                            "eor r1, r1, lr\n"
                            "autg r12, r10, r1\n";

    NewMIs.push_back(BuildMI(MF, DL, TII->get(ARM::INLINEASM))
                            .addExternalSymbol(MF.createExternalSymbolName(asmString))
                            .addImm(1)
                            .addExternalSymbol(""));

    insertInstsAfter(MI, NewMIs);
}

bool ARMXCFISecureException::runOnMachineFunction(MachineFunction &MF) {
    if (XCFIDisabled() || XCFI::mode() == XCFI::Forward) return true;
    if (std::find(secure_handlers.begin(), secure_handlers.end(), MF.getName()) == secure_handlers.end()) return true;

    std::vector<MachineInstr *> eraseInstrs;
    std::vector<MachineInstr *> prologueInstrs;
    std::vector<MachineInstr *> epilogueInstrs;

    for(MachineBasicBlock &MBB : MF) {
        for (MachineBasicBlock::iterator MBBI = MBB.begin(), MBBE = MBB.end(); MBBI != MBBE; ++MBBI) {
            MachineInstr &MI = *MBBI;
            switch (MI.getOpcode()) {
                case ARM::t2BTI: {
                    prologueInstrs.push_back(&MI);
                    
                    MachineBasicBlock::iterator NextIt = MBBI;
                    for (int i = 0; i < 3 && NextIt != MBBE; ++i) {
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
                    for (int i = 0; i < 4 && NextIt != MBBE; ++i) {
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

    for (MachineInstr *MI : eraseInstrs) {
        //MI->eraseFromParent();
    }
    
    for (MachineInstr *MI : prologueInstrs) {
        insertPrologueInstrumentation(MF, *MI);
    }

    for (MachineInstr *MI : epilogueInstrs) {
        insertEpilogueInstrumentation(MF, *MI);
    }
    
    return true;
}

namespace llvm {
    FunctionPass *createARMXCFISecureException(void) {
        return new ARMXCFISecureException();
    }
}
