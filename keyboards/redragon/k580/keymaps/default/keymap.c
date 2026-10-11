#include QMK_KEYBOARD_H

// Macro Keys (for default keymap)
enum {
  U_G1 = SAFE_RANGE,
  U_G2,
  U_G3,
  U_G4,
  U_G5,
  U_REC,
  U_VOLBRI // Volume/Brightness toggle
};
bool BRI = false; // Keep track of volume/brightness for the encoder

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
//COM is Row, KeyPu is Column
	[0] = LAYOUT_default(
		/*COM16*/	  /*COM17*/	  /*COM18*/	/*COM19*/	/*COM20*/	/*COM21*/	/*COM22*/	/*COM23*/	/*COM24*/	/*COM25*/ /*COM25*/	/*COM27*/	/*COM28*/	/*COM29*/	/*COM30*/	/*COM31*/   /*COM32*/	/*COM33*/
        KC_ESC,   KC_F1,   	KC_F2,   	KC_F3,   	KC_F4,   	KC_F5,   	KC_F6,   	KC_F7,   	KC_F8,    KC_F9,    KC_F10,   KC_F11,  	KC_F12,  	KC_PSCR, 	KC_SCRL, 	KC_PAUS,    KC_NUM,  KC_PSLS,
        KC_GRV,   KC_1,    	KC_2,    	KC_3,    	KC_4,    	KC_5,    	KC_6,    	KC_7,    	KC_8,     KC_9,    	KC_0,     KC_MINS, 	KC_EQL,  	KC_BSPC, 	KC_INS,  	KC_HOME,    KC_PGUP, KC_PAST,
        KC_TAB,   KC_Q,    	KC_W,    	KC_E,    	KC_R,    	KC_T,    	KC_Y,    	KC_U,    	KC_I,     KC_O,    	KC_P,     KC_LBRC, 	KC_RBRC, 	KC_BSLS, 	KC_DEL,  	KC_END,     KC_PGDN, KC_PMNS,
        KC_CAPS,  KC_A,   	KC_S,    	KC_D,    	KC_F,    	KC_G,    	KC_H,    	KC_J,    	KC_K,     KC_L,    	KC_SCLN,  KC_QUOT, 	          KC_ENT,   KC_P7,    KC_P8,      KC_P9,   KC_PPLS,
        KC_LSFT,  KC_Z,     KC_X,    	KC_C,    	KC_V,    	KC_B,    	KC_N,    	KC_M,    	KC_COMM,  KC_DOT,  	KC_SLSH,             	        KC_RSFT,  KC_UP,    KC_P4,      KC_P5,   KC_P6,
        KC_LCTL,  KC_LGUI,  KC_LALT, 	KC_SPC,  	KC_RALT, 	KC_RGUI, 	MO(0),   	KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RIGHT, KC_P0, 	  KC_PDOT, 	KC_P1,  	KC_P2,    KC_P3,      KC_PENT,
        /* row 6 — macro keys */                                    U_G1,     U_G2,     U_G3,      U_G4,    U_G5,     U_REC,
        /* row 7 — media keys */                                                                                                U_VOLBRI, KC_MPRV, KC_MPLY, KC_MNXT),
};
/*
/* Macro LED support
                    {G1, G2, G3, G4, G5, REC} */
//int macro_state[] = {0,  0,  0,  0,  0,  0  };

// Map Macro state to RGB channels of LED indices 77 and 78
/*
void set_macro_leds(int state[]) {
    rgb_matrix_set_color(118, state[2]*255, state[1]*255, state[0]*255);
    rgb_matrix_set_color(119, state[5]*255, state[4]*255, state[3]*255);
}

void macro_led_toggle(int i) {
    macro_state[i] = 1-macro_state[i];
    set_macro_leds(macro_state);
}

void macro_led_on(int i) {
    macro_state[i] = 1;
    set_macro_leds(macro_state);
}

void macro_led_off(int i) {
    macro_state[i] = 0;
    set_macro_leds(macro_state);
}

void macro_led_clear(void) {
    for (int i = 0; i < 6; i++){
        macro_state[i] = 0;
    }
    set_macro_leds(macro_state);
}

*/
// Macro support (for default keymap)

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
    case U_VOLBRI:
        if (record->event.pressed) {
            // switch volume/brightness for encoder
            BRI = !BRI;
        } else {
            // on release
        }
        break;/*
    case U_G1:
        if (record->event.pressed) {
            macro_led_toggle(0);
        }
        break;
    case U_G2:
        if (record->event.pressed) {
            macro_led_toggle(1);
        }
        break;
    case U_G3:
        if (record->event.pressed) {
            macro_led_toggle(2);
        }
        break;
    case U_G4:
        if (record->event.pressed) {
            macro_led_toggle(3);
        }
        break;
    case U_G5:
        if (record->event.pressed) {
            macro_led_toggle(4);
        }
        break;
    case U_REC:
        if (record->event.pressed) {
            macro_led_toggle(5);
        }
        break;*/
    }
    return true;
}

//encoder support


#ifdef ENCODER_MAP_ENABLE
bool encoder_update_user(uint8_t index, bool clockwise) {
    if (index == 0) { // First encoder 
        if (clockwise) {
            if(BRI){
                rgb_matrix_increase_val_noeeprom();
            }else{
                tap_code(KC_VOLU);
            }
        } else {
            if(BRI){
                rgb_matrix_decrease_val_noeeprom();
            }else{
                tap_code(KC_VOLD);
            }
        }
    }
    return true;
}
#endif


void keyboard_post_init_user(void) {}
void housekeeping_task_user(void) {}

bool pre_process_record_user(uint16_t keycode, keyrecord_t *record) { return true; }
void post_process_record_user(uint16_t keycode, keyrecord_t *record) {}
//void keyboard_post_init_user(void) {
  // Customise these values to desired behaviour
/*debug_enable=false;
debug_matrix=false;
debug_keyboard=false;
debug_mouse=false;*/
//}

                /*COM16*/	/*COM17*/	/*COM18*/	/*COM19*/	/*COM20*/	/*COM21*/	/*COM22*/	/*COM23*/	/*COM24*/	/*COM25*/ /*COM25*/	/*COM27*/	/*COM28*/	/*COM29*/	/*COM30*/	/*COM31*/
	/*KeyPu0*/	//KC_ESC,   KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,    KC_F12,  KC_PSCR,  KC_SCRL, KC_PAUS,
	/*KeyPu1*/	//KC_GRV,   KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,   KC_EQL,  KC_BSPC,  KC_INS,  KC_HOME,
	/*KeyPu2*/	//KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,   KC_RBRC, KC_BSLS, 	KC_DEL,  KC_END,
	/*KeyPu3*/	//KC_LCTL,  KC_A,     KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,   KC_ENT,            KC_PGUP, KC_PGDN,
	/*KeyPu4*/	//KC_LSFT,  KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,  KC_RSFT,  		  			         KC_UP,
	/*KeyPu5*/	//KC_LCTL,  KC_LGUI,  KC_LALT,  KC_SPC    KC_RALT,  KC_TRNS,  KC_APP,   KC_RCTL,   			                        KC_LEFT,   KC_DOWN,  KC_RGHT, KC_NUM,   KC_PENT,
  	/*KeyPu6*/	//KC_PDOT,  KC_P1,    KC_P2,    KC_P3,    KC_P4,    KC_P5,    KC_P6,    KC_P7,    KC_P8,    KC_P9,    KC_P0,    KC_PPLS,   KC_PMNS,  KC_PAST, KC_PSLS),


/*G1:COM22
G2:COM23, Keypu6
G3:COM24, Keypu6
G4:COM25, Keypu6
G5:COM26, Keypu6
Rec:COM27, Keypu6
PDOT:COM28, Keypu5
p7: COM30,
*/

            /*COM16*/	/*COM17*/	/*COM18*/	/*COM19*/	/*COM20*/	/*COM21*/	/*COM22*/	/*COM23*/	/*COM24*/	/*COM25*/ /*COM25*/	/*COM27*/	/*COM28*/	/*COM29*/	/*COM30*/	/*COM31*/
/*KeyPu0     ESC,       F1,   	  F2,   	  F3,   	  F4,   	  F5,   	  F6,   	  F7,   	  F8,   	  F9,   	  F10,      F11,      F12,      PSCR,     SCRL,     PAUS,
KeyPu1       GRV,       1,    	  2,    	  3,    	  4,    	  5,    	  6,    	  7,    	  8,    	  9,    	  0,    	  MINS,     EQL,      BSPC,     INS,      HOME,
KeyPu2       TAB,       Q,    	  W,    	  E,    	  R,    	  T,    	  Y,    	  U,    	  I,    	  O,    	  P,    	  LBRC,     RBRC,     BSLS, 	  DEL,      END,
KeyPu3      LSFT,       A,    	  S,    	  D,    	  F,    	  G,    	  H,    	  J,    	  K,    	  L,    	SCLN,       QUOT,               ENT,      N7,       PGDN
KeyPu4      LCTL,       Z,    	  X,    	  C,    	  V,    	  B,    	  N,    	  M,    	  COMM,  DOT,   SLSH,  RSFT,  		  			                    UP,
/*KeyPu1*/
/*KeyPu2*/
/*KeyPu3*/
/*KeyPu4*/
/*KeyPu5*/
/*KeyPu6*/

/* Keys left to configure are:
Numlock
Numpad Slash
Numpad Asterisk
Numpad Minus
Numpad Plus
Numpad Enter
Page Up
Page Down

Potential pins that can be these keys:
PORT7_0
PORT7_1
PORT1_0
PORT1_1
PORT1_2
PORT1_3
PORT1_4
PORT1_5
PORT1_6
PORT1_7
PORT2_0
PORT2_1
PORT2_2
PORT2_3
PORT2_4
PORT2_5
PORT2_6
PORT2_7
PORT5_0
PORT5_1
PORT5_2
PORT5_3
PORT5_4
PORT5_5
PORT5_6
PORT5_7
PORT6_0
PORT6_1
PORT6_2
PORT6_3
PORT6_4
PORT6_5
PORT6_6
PORT6_7
PORT4_7





*/
