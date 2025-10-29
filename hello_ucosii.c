#include <io.h>
#include <stdio.h>
#include <system.h>

#include "definition.h"
#include "includes.h"

/* Definition of Task Stacks */
#define TASK_STACKSIZE 2048
OS_STK task1_stk[TASK_STACKSIZE];
OS_STK task2_stk[TASK_STACKSIZE];
OS_STK task3_stk[TASK_STACKSIZE];
OS_STK task4_stk[TASK_STACKSIZE];
OS_STK task5_stk[TASK_STACKSIZE];
OS_STK task6_stk[TASK_STACKSIZE];

#define DELAY_MILISECOND 0
#define DELAY_SECOND 1
#define DELAY_MINUTES 0
#define DELAY_HOURS 0

OS_EVENT *SWBox1;
OS_EVENT *STBox1;

/* Definition of Task Priorities */

#define TASK1_PRIORITY 1
#define TASK2_PRIORITY 2
#define TASK3_PRIORITY 3
#define TASK4_PRIORITY 4
#define TASK5_PRIORITY 6
#define TASK6_PRIORITY 5

typedef enum {
    PH_RED,
    PH_RED_YELLOW,
    PH_GREEN,
    PH_YELLOW,
    PH_RED_GREEN,
    PH_NONO
} Phase;

const char* phase_names[] = {
    "PH_RED",
    "PH_RED_YELLOW",
    "PH_GREEN",
    "PH_YELLOW",
    "PH_RED_GREEN",
    "PH_NONO"
};

// Stan kontrolera
struct controller_state {
    int powerOn;         // SW0
    int emergencyBlink;  // SW1
    Phase currentPhase;  // faza aktywnej osi
};

struct controller_state st;

Phase desired_phase = PH_RED;

int leds_output = 0x0;

static Phase decode_desired_phase(const int r, const int y, const int g) {
    if (r && y && !g) {
        return PH_RED_YELLOW;
    }
    if (g && !r && !y) {
        return PH_GREEN;
    }
    if (y && !r && !g) {
        return PH_YELLOW;
    }
    if (r && !y && !g) {
        return PH_RED;
    }
    if (r && !y && g) {
        return PH_RED_GREEN;
    }
    return PH_NONO;
}

static int is_legal_next(Phase from, Phase to) {
    return (from == PH_RED && to == PH_RED_YELLOW) ||
           (from == PH_RED_YELLOW && to == PH_GREEN) ||
           (from == PH_GREEN && to == PH_YELLOW) ||
           (from == PH_YELLOW && to == PH_RED) ||
           (from == PH_RED && to == PH_RED_GREEN) ||
           (from == PH_RED_GREEN && to == PH_RED);
}

void task1(void *pdata) {
    while (1) {
        int sw;
        int *msg;
        sw = IORD(SW_SLIDERS_BASE, 0);

        printf("Slider = %d\n", sw);
        OSMboxPostOpt(SWBox1, &sw, OS_POST_OPT_BROADCAST);

        OSTimeDlyHMSM(DELAY_HOURS, DELAY_MINUTES, DELAY_SECOND,
                      DELAY_MILISECOND);
    }
}
/* Prints "Hello World" and sleeps for three seconds */
void task2(void *pdata) {
    while (1) {
        INT8U err;
        int *num;

        num = OSMboxPend(SWBox1, 0, &err);

//        printf("T=%d\n", *num);

        if (*num & SW0) {
            st.powerOn = 1;
            leds_output |= LED0;
        } else {
            st.powerOn = 0;
            st.currentPhase = PH_RED;
            leds_output &= ~LED0;
        }
        OSTimeDlyHMSM(DELAY_HOURS, DELAY_MINUTES, DELAY_SECOND,
                      DELAY_MILISECOND);
    }
}

void task3(void *pdata) {
    while (1) {
        INT8U err;
        int *num;

        num = OSMboxPend(SWBox1, 0, &err);

//        printf("Q=%d\n", *num);

        if (st.powerOn && *num & SW1) {
            st.emergencyBlink = 1;
            st.currentPhase = PH_RED;
            leds_output |= LED1;
        } else {
            leds_output &= ~LED1;
            st.emergencyBlink = 0;
        }
        OSTimeDlyHMSM(DELAY_HOURS, DELAY_MINUTES, DELAY_SECOND,
                      DELAY_MILISECOND);
    }
}

void task4(void *pdata) {
    while (1) {
        IOWR(LEDS_BASE, 0, leds_output);
//        printf("ledy=%d\n", leds_output);
        OSTimeDlyHMSM(DELAY_HOURS, DELAY_MINUTES, DELAY_SECOND,
                      DELAY_MILISECOND);
    }
}

void task5(void *pdata) {
    while (1) {
        if (st.powerOn && !st.emergencyBlink) {
            if (st.currentPhase == PH_RED) {
                leds_output |= LED2;
                IOWR(HEX_BASE, 0, SEGA);
                IOWR(HEX_BASE, 5, ZERO);
                continue; // contiune przerywa taski o niższych prio
            } else {
                leds_output &= ~LED2;
            }
            if (st.currentPhase == PH_RED_YELLOW) {
                leds_output |= LED2 | LED3;
                IOWR(HEX_BASE, 0, SEGA | SEGG);
                IOWR(HEX_BASE, 5, ONE);
                continue;
            } else {
                leds_output &= ~(LED2 | LED3);
            }
            if (st.currentPhase == PH_GREEN) {
                leds_output |= LED4;
                IOWR(HEX_BASE, 0, SEGD);
                IOWR(HEX_BASE, 5, TWO);
                continue;
            } else {
                leds_output &= ~LED4;
            }
            if (st.currentPhase == PH_YELLOW) {
                leds_output |= LED3;
                IOWR(HEX_BASE, 0, SEGG);
                IOWR(HEX_BASE, 5, THREE);
                continue;
            } else {
                leds_output &= ~LED3;
            }
            if (st.currentPhase == PH_RED_GREEN) {
                leds_output |= LED2 | LED4;
                IOWR(HEX_BASE, 0, SEGA | SEGC);
                IOWR(HEX_BASE, 5, FOUR);
                continue;
            } else {
                leds_output &= ~(LED2 | LED4);
            }
        } else {
            leds_output &= ~(LED2 | LED3 | LED4);
        }
        OSTimeDlyHMSM(DELAY_HOURS, DELAY_MINUTES, DELAY_SECOND,
                      DELAY_MILISECOND);
    }
}

void task6(void *pdata) {
    while (1) {
    	printf("Aktualna faza: %s\n", phase_names[st.currentPhase]);
        if (st.powerOn && !st.emergencyBlink) {
            int sw2 = 0;
            int sw3 = 0;
            int sw4 = 0;

            INT8U err;
            int *num;

            num = OSMboxPend(SWBox1, 0, &err);
            if (*num & SW2) {
                sw2 = 1;
            } else {
                sw2 = 0;
            }
            if (*num & SW3) {
                sw3 = 1;
            } else {
                sw3 = 0;
            }
            if (*num & SW4) {
                sw4 = 1;
            } else {
                sw4 = 0;
            }

            desired_phase = decode_desired_phase(sw2, sw3, sw4);

            if (desired_phase == PH_NONO) {
                IOWR(HEX_BASE, 3, HEX_N);
                IOWR(HEX_BASE, 2, HEX_O);
            } else if (is_legal_next(st.currentPhase, desired_phase)) {
                st.currentPhase = desired_phase;
            }
        }
        OSTimeDlyHMSM(DELAY_HOURS, DELAY_MINUTES, DELAY_SECOND,
                      DELAY_MILISECOND);
    }
}

void setup() {
    printf("Setup...\n");

    for (int i = 0; i <= 5; i++) {
        IOWR(HEX_BASE, i, 0);
    }

    st.powerOn = 0;
    st.emergencyBlink = 0;
    st.currentPhase = PH_RED;
}

/* The main function creates two task and starts multi-tasking */
int main(void) {
    printf("Main...\n");
    setup();

    SWBox1 = OSMboxCreate((void *)0);

    OSTaskCreateExt(task1, NULL, (void *)&task1_stk[TASK_STACKSIZE - 1],
                    TASK1_PRIORITY, TASK1_PRIORITY, task1_stk, TASK_STACKSIZE,
                    NULL, 0);

    OSTaskCreateExt(task2, NULL, (void *)&task2_stk[TASK_STACKSIZE - 1],
                    TASK2_PRIORITY, TASK2_PRIORITY, task2_stk, TASK_STACKSIZE,
                    NULL, 0);

    OSTaskCreateExt(task3, NULL, (void *)&task3_stk[TASK_STACKSIZE - 1],
                    TASK3_PRIORITY, TASK3_PRIORITY, task3_stk, TASK_STACKSIZE,
                    NULL, 0);

    OSTaskCreateExt(task4, NULL, (void *)&task4_stk[TASK_STACKSIZE - 1],
                    TASK4_PRIORITY, TASK4_PRIORITY, task4_stk, TASK_STACKSIZE,
                    NULL, 0);

    OSTaskCreateExt(task5, NULL, (void *)&task5_stk[TASK_STACKSIZE - 1],
                    TASK5_PRIORITY, TASK5_PRIORITY, task5_stk, TASK_STACKSIZE,
                    NULL, 0);

    OSTaskCreateExt(task6, NULL, (void *)&task6_stk[TASK_STACKSIZE - 1],
                    TASK6_PRIORITY, TASK6_PRIORITY, task6_stk, TASK_STACKSIZE,
                    NULL, 0);

    OSStart();
    return 0;
}
