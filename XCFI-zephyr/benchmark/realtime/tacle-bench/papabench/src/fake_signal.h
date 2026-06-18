#define FAKE_SIGNAL_CONCAT2(a,b) a##b
#define FAKE_SIGNAL_CONCAT(a,b) FAKE_SIGNAL_CONCAT2(a,b)

#ifndef SIGNAL_PREFIX
#define SIGNAL_PREFIX
#endif

#define SIGNAL(signame) void FAKE_SIGNAL_CONCAT(SIGNAL_PREFIX, signame)(void)
