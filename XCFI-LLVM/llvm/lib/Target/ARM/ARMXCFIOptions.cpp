#include "ARMXCFIOptions.h"
#include "llvm/Support/CommandLine.h"
using namespace llvm;

namespace {
    cl::opt<XCFI::Mode> XCFIModeOpt(
        "enable-XCFI", cl::init(XCFI::Off),
        cl::desc("Select XCFI mode: all|forward|backward|off|abadi"),
        cl::values(
            clEnumValN(XCFI::All, "all", "Enable forward+backward CFI"),
            clEnumValN(XCFI::Forward, "forward", "Enable forward-edge CFI"),
            clEnumValN(XCFI::Backward, "backward", "Enable backward-edge CFI"),
            clEnumValN(XCFI::tz, "tz", "Enable secure state exception CFI"),
            clEnumValN(XCFI::Off, "off", "Disable XCFI")));
}

XCFI::Mode XCFI::mode() { return XCFIModeOpt.getValue(); }
bool XCFI::XCFIDisabled() { return XCFIModeOpt == XCFI::Off; }
