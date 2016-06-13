
#define BUFDELIM "\xa5\xc9"
#define CALLDELIM "\xb7\xe3"

struct sysRec {
    u_int16_t nr;
    u_int64_t args[6];
};

int parseSysRec(struct slice *b, struct sysRec *x);
int parseSysRecArr(struct slice *b, int maxRecs, struct sysRec *x, int *nRecs);
void showSysRec(struct sysRec *x);
void showSysRecArr(struct sysRec *x, int n);
unsigned long doSysRec(struct sysRec *x);
unsigned long doSysRecArr(struct sysRec *x, int n);

int getStdFile(int typ);

