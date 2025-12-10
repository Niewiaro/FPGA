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

static void handle_sliders_interrupt(struct interrupt_data * data, struct controller_state * st) {
    //        int sw = IORD(data -> sw_slider_addr, 0);
    //        IOWR(data -> leds_addr, 0, sw);

    int swstate = 0;
    int sw2 = 0;
    int sw3 = 0;
    int sw4 = 0;

    Phase desired_phase = PH_RED;

    swstate = IORD(data -> sw_slider_addr, 0);
    IOWR(data -> leds_addr, 0, 0);

    if (swstate & SW0) {
        st -> powerOn = 1;
        IOWR(data -> leds_addr, 0, LED0);
    } else {
        st -> powerOn = 0;
        st -> currentPhase = PH_RED;
        return;
    }

    if (swstate & SW1) {
        st -> emergencyBlink = 1;
        st -> currentPhase = PH_RED;
        IOWR(LEDS_BASE, 0, LED1);
        return;
    } else {
        st -> emergencyBlink = 0;
    }

    if (st -> currentPhase == PH_RED) {
        IOWR(data -> leds_addr, 0, LED2);
        IOWR(data -> hex_addr, 0, SEGA);
        IOWR(data -> hex_addr, 5, ZERO);
    } else if (st -> currentPhase == PH_RED_YELLOW) {
        IOWR(data -> leds_addr, 0, LED2 | LED3);
        IOWR(data -> hex_addr, 0, SEGA | SEGG);
        IOWR(data -> hex_addr, 5, ONE);
    } else if (st -> currentPhase == PH_GREEN) {
        IOWR(data -> leds_addr, 0, LED4);
        IOWR(data -> hex_addr, 0, SEGD);
        IOWR(data -> hex_addr, 5, TWO);
    } else if (st -> currentPhase == PH_YELLOW) {
        IOWR(data -> leds_addr, 0, LED3);
        IOWR(data -> hex_addr, 0, SEGG);
        IOWR(data -> hex_addr, 5, THREE);
    } else if (st -> currentPhase == PH_RED_GREEN) {
        IOWR(data -> leds_addr, 0, LED2 | LED4);
        IOWR(data -> hex_addr, 0, SEGA | SEGC);
        IOWR(data -> hex_addr, 5, FOUR);
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
        IOWR(data -> hex_addr, 3, HEX_N);
        IOWR(data -> hex_addr, 2, HEX_O);
    } else if (is_legal_next(st->currentPhase, desired_phase)) {
        st->currentPhase = desired_phase;
        for (int i = 2; i <= 3; i++) {
			IOWR(data -> hex_addr, i, 0);
		}
    }

}

//static void handle_pushbuttons_interrupt(struct interrupt_data *data)
//{
//	int pb = IORD(data->pushbuttons_addr,0);
//
//
//	switch(pb)
//	{
//               ...
//
//		default:
//			IOWR(data->hex_addr, 0, pb);
//			break;
//	}
//	count2 ++;
//}

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

    struct controller_state st;
    st.powerOn = 0;
    st.emergencyBlink = 0;
    st.currentPhase = PH_GREEN;

    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(SW_SLIDERS_BASE, 0xf);

    alt_ic_isr_register(SW_SLIDERS_IRQ_INTERRUPT_CONTROLLER_ID, SW_SLIDERS_IRQ, handle_sliders_interrupt, & data, & st);
    //alt_ic_isr_register(PUSHBUTTON_IRQ_INTERRUPT_CONTROLLER_ID, PUSHBUTTON_IRQ, handle_pushbuttons_interrupt, &data, 0x0);

    alt_ic_irq_enable(SW_SLIDERS_IRQ_INTERRUPT_CONTROLLER_ID, SW_SLIDERS_IRQ);

    while (1);

    printf("End...\n");
    return 0;
}
