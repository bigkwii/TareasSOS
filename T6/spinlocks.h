enum { OPEN, CLOSED};
void spinLock(volatile int *psl) ;
void spinUnlock(int *psl);
