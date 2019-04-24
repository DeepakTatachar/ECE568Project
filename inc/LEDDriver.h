typedef enum c_state { R_UP, R_DWN_G_UP, G_DWN_B_UP, B_DWN, END } COLOR_STATE;

void initLED();

void setColor(uint8_t r, uint8_t g, uint8_t b);

void colorTest();
