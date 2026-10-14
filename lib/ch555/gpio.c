#include "gpio.h"
#include <stdbool.h>
#include <stdint.h>
#include "ch555.h" 

__code uint8_t pin_to_pinmask[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

void pinModeInputHighQMK(uint8_t pin) {
    __data uint8_t port = PORTx(pin);
    __data uint8_t pinmask  = PIN_MASK(pin);
    switch (port) {
        // P0-P4: quasi-bidirectional (open drain with pull-up) with the latch high. The latch is written first,
        // so that a pin that was a high-Z input or an open-drain output with the latch low is not pulled low.
#ifdef PORT0
        case PORT0:
        P0        |= pinmask;
        P0_MOD_OC |= pinmask;
        P0_DIR_PU |= pinmask;
        break;
#endif
#ifdef PORT1
        case PORT1:
        P1        |= pinmask;
        P1_MOD_OC |= pinmask;
        P1_DIR_PU |= pinmask;
        break;
#endif
#ifdef PORT2
        case PORT2:
        P2        |= pinmask;
        P2_MOD_OC |= pinmask;
        P2_DIR_PU |= pinmask;
        break;
#endif
#ifdef PORT3
        case PORT3:
        P3        |= pinmask;
        P3_MOD_OC |= pinmask;
        P3_DIR_PU |= pinmask;
        break;
#endif
#ifdef PORT4
        case PORT4:
        P4        |= pinmask;
        P4_MOD_OC |= pinmask;
        P4_DIR_PU |= pinmask;
        break;
#endif
#ifdef PORT5
        case PORT5:
        P5_DIR    &= ~pinmask; /* input */
        P5_OUT_PU |=  pinmask; /* pullup */
        break;
#endif
#ifdef PORT7
        case PORT7:
        P7 &= ~(pinmask<<2); // _DIR
        P7 |=  pinmask; // _OUT_PU
        break;
#endif
    }

}

void pinModeInputZ(uint8_t pin) {
    uint8_t port = PORTx(pin);
    uint8_t pinmask  = PIN_MASK(pin);
    switch (port) {
#ifdef PORT0
        case PORT0:
        P0_MOD_OC &= ~pinmask;
        P0_DIR_PU &= ~pinmask;
        break;
#endif
#ifdef PORT1
        case PORT1:
        P1_MOD_OC &= ~pinmask;
        P1_DIR_PU &= ~pinmask;
        break;
#endif
#ifdef PORT2
        case PORT2:
        P2_MOD_OC &= ~pinmask;
        P2_DIR_PU &= ~pinmask;
        break;
#endif
#ifdef PORT3
        case PORT3:
        P3_MOD_OC &= ~pinmask;
        P3_DIR_PU &= ~pinmask;
        break;
#endif
#ifdef PORT4
        case PORT4:
        P4_MOD_OC &= ~pinmask;
        P4_DIR_PU &= ~pinmask;
        break;
#endif
#ifdef PORT5
        case PORT5:
        P5_DIR    &= ~pinmask; /* input */
        P5_OUT_PU &= ~pinmask; /* no pullup (high-Z) */
        break;
#endif
#ifdef PORT7
        case PORT7:
        P7 &= ~(pinmask<<2); // _DIR
        P7 &= ~pinmask; // _OUT_PU
        break;
#endif
    }
}
void pinModeOutputPushPull(uint8_t pin) {
    __data uint8_t port = PORTx(pin);
    __data uint8_t pinmask  = PIN_MASK(pin);
    switch (port) {
#ifdef PORT0
        case PORT0:
        P0_MOD_OC &= ~pinmask;
        P0_DIR_PU |=  pinmask;
        break;
#endif
#ifdef PORT1
        case PORT1:
        P1_MOD_OC &= ~pinmask;
        P1_DIR_PU |=  pinmask;
        break;
#endif
#ifdef PORT2
        case PORT2:
        P2_MOD_OC &= ~pinmask;
        P2_DIR_PU |=  pinmask;
        break;
#endif
#ifdef PORT3
        case PORT3:
        P3_MOD_OC &= ~pinmask;
        P3_DIR_PU |=  pinmask;
        break;
#endif
#ifdef PORT4
        case PORT4:
        P4_MOD_OC &= ~pinmask;
        P4_DIR_PU |=  pinmask;
        break;
#endif
#ifdef PORT5
        case PORT5:
        P5_DIR |= pinmask; /* push-pull output */
        break;
#endif
#ifdef PORT7
        case PORT7:
        P7 |=  (pinmask<<2); // _DIR
        break;
#endif
    }
}
void pinModeOutputOpen(uint8_t pin) {
    uint8_t port = PORTx(pin);
    uint8_t pinmask  = PIN_MASK(pin);
    switch (port) {
#ifdef PORT0
        case PORT0:
        P0_MOD_OC |=  pinmask;
        P0_DIR_PU &= ~pinmask;
        break;
#endif
#ifdef PORT1
        case PORT1:
        P1_MOD_OC |=  pinmask;
        P1_DIR_PU &= ~pinmask;
        break;
#endif
#ifdef PORT2
        case PORT2:
        P2_MOD_OC |=  pinmask;
        P2_DIR_PU &= ~pinmask;
        break;
#endif
#ifdef PORT3
        case PORT3:
        P3_MOD_OC |=  pinmask;
        P3_DIR_PU &= ~pinmask;
        break;
#endif
#ifdef PORT4
        case PORT4:
        P4_MOD_OC |=  pinmask;
        P4_DIR_PU &= ~pinmask;
        break;
#endif
        // P5 and P7 have no open-drain mode (datasheet 10.2): push-pull output, or input with or without pull-up
    }
}
void pinModeBiDi(uint8_t pin) {
    uint8_t port = PORTx(pin);
    uint8_t pinmask  = PIN_MASK(pin);
    switch (port) {
#ifdef PORT0
        case PORT0:
        P0_MOD_OC |=  pinmask;
        P0_DIR_PU |=  pinmask;
        break;
#endif
#ifdef PORT1
        case PORT1:
        P1_MOD_OC |=  pinmask;
        P1_DIR_PU |=  pinmask;
        break;
#endif
#ifdef PORT2
        case PORT2:
        P2_MOD_OC |=  pinmask;
        P2_DIR_PU |=  pinmask;
        break;
#endif
#ifdef PORT3
        case PORT3:
        P3_MOD_OC |=  pinmask;
        P3_DIR_PU |=  pinmask;
        break;
#endif
#ifdef PORT4
        case PORT4:
        P4_MOD_OC |=  pinmask;
        P4_DIR_PU |=  pinmask;
        break;
#endif
        // P5 and P7 have no quasi-bidirectional mode (datasheet 10.2)
    }
}
uint8_t readPin(uint8_t pin) {
    __data uint8_t port = PORTx(pin);
    __data uint8_t pinmask  = PIN_MASK(pin);
    switch (port) {
#ifdef PORT0
        case PORT0:
        return ((bool)(P0 & pinmask));
        break;
#endif
#ifdef PORT1
        case PORT1:
        return ((bool)(P1 & pinmask));
        break;
#endif
#ifdef PORT2
        case PORT2:
        return ((bool)(P2 & pinmask));
        break;
#endif
#ifdef PORT3
        case PORT3:
        return ((bool)(P3 & pinmask));
        break;
#endif
#ifdef PORT4
        case PORT4:
        return ((bool)(P4 & pinmask));
        break;
#endif
#ifdef PORT5
        case PORT5:
        return ((bool)(P5_IN & pinmask));
        break;
#endif
#ifdef PORT7
        case PORT7:
        return ((bool)(P7 & (pinmask<<4))); // bP7_{0,1}_IN
        break;
#endif
        default:
        return -1;
        break;
    }
}
void writePinHigh(uint8_t pin) {
    __data uint8_t port = PORTx(pin);
    __data uint8_t pinmask  = PIN_MASK(pin);
    switch (port) {
#ifdef PORT0
        case PORT0:
        P0 |=  pinmask;
        break;
#endif
#ifdef PORT1
        case PORT1:
        P1 |=  pinmask;
        break;
#endif
#ifdef PORT2
        case PORT2:
        P2 |=  pinmask;
        break;
#endif
#ifdef PORT3
        case PORT3:
        P3 |=  pinmask;
        break;
#endif
#ifdef PORT4
        case PORT4:
        P4 |=  pinmask;
        break;
#endif
#ifdef PORT5
        case PORT5:
        P5_OUT_PU |= pinmask;
        break;
#endif
#ifdef PORT7
        case PORT7:
        P7 |=  pinmask; // _OUT_PU
        break;
#endif
    }
}
void writePinLow(uint8_t pin) {
    __data uint8_t port = PORTx(pin);
    __data uint8_t pinmask  = PIN_MASK(pin);
    switch (port) {
#ifdef PORT0
        case PORT0:
        P0 &= ~pinmask;
        break;
#endif
#ifdef PORT1
        case PORT1:
        P1 &= ~pinmask;
        break;
#endif
#ifdef PORT2
        case PORT2:
        P2 &= ~pinmask;
        break;
#endif
#ifdef PORT3
        case PORT3:
        P3 &= ~pinmask;
        break;
#endif
#ifdef PORT4
        case PORT4:
        P4 &= ~pinmask;
        break;
#endif
#ifdef PORT5
        case PORT5:
        P5_OUT_PU &= ~pinmask;
        break;
#endif
#ifdef PORT7
        case PORT7:
        P7 &= ~pinmask; // _OUT_PU
        break;
#endif
    }
}
