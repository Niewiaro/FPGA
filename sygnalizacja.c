#include <io.h>

#include <stdio.h>

#include <system.h>

#include "definition.h"

typedef enum {
    PH_RED,
    PH_RED_YELLOW,
    PH_GREEN,
    PH_YELLOW,
    PH_RED_GREEN
}
Phase;

// Stan kontrolera
typedef struct {
    int powerOn; // SW0
    int emergencyBlink; // SW1
    Phase currentPhase; // faza aktywnej osi
}
ControllerState;

static Phase decode_desired_phase(const int r,
    const int y,
        const int g) {
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
}

static int is_legal_next(Phase from, Phase to) {
    return (from == PH_RED && to == PH_RED_YELLOW) ||
        (from == PH_RED_YELLOW && to == PH_GREEN) ||
        (from == PH_GREEN && to == PH_YELLOW) ||
        (from == PH_YELLOW && to == PH_RED) ||
        (from == PH_RED && to == PH_RED_GREEN) ||
        (from == PH_RED_GREEN && to == PH_RED);
}

int main() {
    printf("Start!\n");

    ControllerState st;
    st.powerOn = 0;
    st.emergencyBlink = 0;
    st.currentPhase = PH_RED;

    int swstate = 0;
    int sw2 = 0;
    int sw3 = 0;
    int sw4 = 0;

    Phase desired_phase = PH_RED;

    do {
        swstate = IORD(SW_SLIDERS_BASE, 0);
        IOWR(LEDS_BASE, 0, 0);
        for(int i = 0; i<=5; i++){
        	IOWR(HEX_BASE, i, 0);
        }

        if (swstate & SW0) {
            st.powerOn = 1;
            IOWR(LEDS_BASE, 0, LED0);
        } else {
            st.powerOn = 0;
            st.currentPhase = PH_RED;
            continue;
        }

        if (swstate & SW1) {
            st.emergencyBlink = 1;
            st.currentPhase = PH_RED;
            IOWR(LEDS_BASE, 0, LED1);
            continue;
        } else {
            st.emergencyBlink = 0;
        }

        if (st.currentPhase == PH_RED) {
            IOWR(LEDS_BASE, 0, LED2);
            IOWR(HEX_BASE, 0, SEGA);
            IOWR(HEX_BASE, 5, ZERO);
        }
        if (st.currentPhase == PH_RED_YELLOW) {
            IOWR(LEDS_BASE, 0, LED2 | LED3);
            IOWR(HEX_BASE, 0, SEGA | SEGG);
            IOWR(HEX_BASE, 5, ONE);
        }
        if (st.currentPhase == PH_GREEN) {
            IOWR(LEDS_BASE, 0, LED4);
            IOWR(HEX_BASE, 0, SEGD);
            IOWR(HEX_BASE, 5, TWO);
        }
        if (st.currentPhase == PH_YELLOW) {
            IOWR(LEDS_BASE, 0, LED3);
            IOWR(HEX_BASE, 0, SEGG);
            IOWR(HEX_BASE, 5, THREE);
        }
        if (st.currentPhase == PH_RED_GREEN) {
            IOWR(LEDS_BASE, 0, LED2 | LED4);
            IOWR(HEX_BASE, 0, SEGA | SEGC);
            IOWR(HEX_BASE, 5, FOUR);
        }

        if (swstate & SW2) {
            sw2 = 1;
        } else {
            sw2 = 0;
        }
        if (swstate & SW3) {
            sw3 = 1;
        } else {
            sw3 = 0;
        }
        if (swstate & SW4) {
            sw4 = 1;
        } else {
            sw4 = 0;
        }

        desired_phase = decode_desired_phase(sw2, sw3, sw4);

        if (is_legal_next(st.currentPhase, desired_phase)) {
            st.currentPhase = desired_phase;
        }

    } while (1);

    printf("Jesli to czytasz to nie dziala\n");
    return 0;
}
