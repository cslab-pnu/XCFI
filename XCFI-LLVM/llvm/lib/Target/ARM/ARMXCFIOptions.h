#pragma once
namespace llvm::XCFI {
	enum Mode { All, Forward, Backward, tz, Off};

	Mode mode();
	bool XCFIDisabled() ;
	inline bool XCFIEnabled() { return !XCFIDisabled(); }
}
