void (*volatile sys_yield)(void);
void (*volatile hello)(void);

void
umain(int argc, char **argv) {
    int i, j;

    
    for (j = 0; j < 3; ++j) {
        for (i = 0; i < 10000; ++i)
            ;
        hello();
        sys_yield();
    }
}
