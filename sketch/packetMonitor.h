// Packet Monitor

// Variable Definitions
const int maxCh = 13;

/* Display settings */
const int minRow = 0;
const int maxRow = 127;
const int minLine = 0;
const int maxLine = 63;

/* render settings */
const int Row1 = 0;
const int Row2 = 22;
const int Row3 = 35;
const int Row4 = 65;
const int Row5 = 92;
const int Row6 = 115;

const int LineVal = 47;

//===== Run-Time variables =====//
unsigned long prevTime   = 0;
unsigned long curTime    = 0;
unsigned long pkts       = 0;
unsigned long no_deauths = 0;
unsigned long deauths    = 0;
int curChannel           = 1;
unsigned long maxVal     = 0;
double multiplicator     = 0.0;
bool canBtnPress         = true;
unsigned int val[128];
