#pragma once

inline int MonWidth = 2560; // Width of the monitor, this does not controller the overlay size but is used for calculations
inline int MonHeight = 1440; // Height of the monitor, this does not controller the overlay size but is used for calculations

#define PRINT_DEBUG 1

#if PRINT_DEBUG == 1
#define dbgPrint(...) do { std::cout << __VA_ARGS__ << " "; } while(0)
#else
// When disabled, the macro does nothing.
#define dbgPrint(...) do {} while(0)
#endif