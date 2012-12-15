#ifndef AVRLCMETERH
#define AVRLCMETERH

#define sbi(port, pin)   ((port) |= (uint8_t)(1 << pin))
#define cbi(port, pin)   ((port) &= (uint8_t)~(1 << pin))

#define REEDRELAY       PB2
#define LCSWITCH        PB1
#define ZEROSWITCH      PB0

#endif
