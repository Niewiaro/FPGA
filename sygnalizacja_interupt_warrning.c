#include <stdio.h>

#include <io.h>

#include <system.h>

#include "alt_types.h"

#include "altera_avalon_pio_regs.h"

#include "sys/alt_irq.h"
 //#include "sys/alt_timestamp.h"
#include <unistd.h>

#include "definition.h"

typedef enum {
    PH_RED,
    PH_RED_YELLOW,
    PH_GREEN,
    PH_YELLOW,
    PH_RED_GREEN,
    PH_NONO
}
Phase;

// Stan kontrolera
struct controller_state {
    int powerOn; // SW0
    int emergencyBlink; // SW1
    Phase currentPhase; // faza aktywnej osi
};

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

struct interrupt_data {
    volatile int * leds_addr;
    volatile int * hex_addr;
    volatile int * sw_slider_addr;
};

volatile int count = 0;

static void handle_sliders_interrupt(struct interrupt_data * data) {
	printf("Interrupt!\n");
   IOWR(data -> hex_addr, 1, HEX_O);

   volatile int cap = IORD_ALTERA_AVALON_PIO_EDGE_CAP(SW_SLIDERS_BASE);
   IOWR_ALTERA_AVALON_PIO_EDGE_CAP(SW_SLIDERS_BASE, cap);

   (void)IORD_ALTERA_AVALON_PIO_EDGE_CAP(SW_SLIDERS_BASE);
}


int main() {
    printf("Main...\n");

    volatile int * leds = (int * )(LEDS_BASE);
    volatile int * hex = (int * )(HEX_BASE);
    volatile int * sw_sliders = (int * )(SW_SLIDERS_BASE);

    for (int i = 0; i <= 5; i++) {
		IOWR(hex, i, 0);
	}

    struct interrupt_data data;

    data.leds_addr = leds;
    data.hex_addr = hex;
    data.sw_slider_addr = sw_sliders;


    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(SW_SLIDERS_BASE, 0);
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(SW_SLIDERS_BASE, 0b0000000010);

    alt_ic_isr_register(SW_SLIDERS_IRQ_INTERRUPT_CONTROLLER_ID, SW_SLIDERS_IRQ, handle_sliders_interrupt, & data, 0x0);
    //alt_ic_isr_register(PUSHBUTTON_IRQ_INTERRUPT_CONTROLLER_ID, PUSHBUTTON_IRQ, handle_pushbuttons_interrupt, &data, 0x0);

    alt_ic_irq_enable(SW_SLIDERS_IRQ_INTERRUPT_CONTROLLER_ID, SW_SLIDERS_IRQ);

    struct controller_state st;
        st.powerOn = 0;
        st.emergencyBlink = 0;
        st.currentPhase = PH_GREEN;

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

            if (desired_phase == PH_NONO) {
                    IOWR(HEX_BASE, 3, HEX_N);
                    IOWR(HEX_BASE, 2, HEX_O);
                } else if (is_legal_next(st.currentPhase, desired_phase)) {
                st.currentPhase = desired_phase;
            }

        } while (1);

    printf("End...\n");
    return 0;
}
