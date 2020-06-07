#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

// The following definitions is taken from ESP8266_MP3_DECODER demo
// https://github.com/espressif/ESP8266_MP3_DECODER/blob/master/mp3/driver/i2s_freertos.c
// It is requred to set clock to I2S subsystem
void sdk_rom_i2c_writeReg_Mask(uint32_t block, uint32_t host_id,
        uint32_t reg_add, uint32_t Msb, uint32_t Lsb, uint32_t indata);

#ifndef i2c_bbpll
#define i2c_bbpll                               0x67
#define i2c_bbpll_en_audio_clock_out            4
#define i2c_bbpll_en_audio_clock_out_msb        7
#define i2c_bbpll_en_audio_clock_out_lsb        7
#define i2c_bbpll_hostid                        4
#endif


#include <FreeRTOS.h>
#include <event_groups.h>

#include <esp/timer.h>
#include <esp/i2s_regs.h>
#include <common_macros.h>
#include <esp/gpio.h>

#define IR_GPIO_NUM 14  // MTCK pin (I2S CLK pin)

#include <ir/tx.h>

EventGroupHandle_t tx_flags;
#define TX_FLAG_READY (1 << 0)


static void gen_carrier() {
    iomux_set_function(gpio_to_iomux(IR_GPIO_NUM), IOMUX_GPIO14_FUNC_I2SI_WS);

    I2S.CONF = SET_MASK_BITS(I2S.CONF, I2S_CONF_RX_START);
}


static void clr_carrier() {
    gpio_enable(IR_GPIO_NUM, GPIO_OUTPUT);
    gpio_write(IR_GPIO_NUM, 0);
    I2S.CONF = CLEAR_MASK_BITS(I2S.CONF, I2S_CONF_RX_START);
}


static void hw_timer_init(void (*isr)(void*), void *arg) {
    timer_set_interrupts(FRC1, false);
    timer_set_run(FRC1, false);

    _xt_isr_attach(INUM_TIMER_FRC1, isr, arg);

    timer_set_interrupts(FRC1, true);
}


static inline void hw_timer_pause() {
    timer_set_run(FRC1, false);
}


static inline void hw_timer_arm(uint32_t us) {
    timer_set_timeout(FRC1, us);
    timer_set_run(FRC1, true);
}


static void IRAM ir_tx_timer_handler(ir_encoder_t *encoder) {
    hw_timer_pause();

    int16_t pulse = encoder->get_next_pulse(encoder);
    if (pulse == 0) {
        // Done with transmission
        clr_carrier();
        encoder->free(encoder);

        BaseType_t task_woken = 0;
        xEventGroupSetBitsFromISR(tx_flags, TX_FLAG_READY, &task_woken);

        portEND_SWITCHING_ISR(task_woken);
        return;
    }

    if (pulse > 0) {
        gen_carrier();
    } else {
        clr_carrier();
    }
    hw_timer_arm(abs(pulse));
}


void ir_tx_init() {
    // Start I2C clock for I2S subsystem
    sdk_rom_i2c_writeReg_Mask(
        i2c_bbpll, i2c_bbpll_hostid,
        i2c_bbpll_en_audio_clock_out,
        i2c_bbpll_en_audio_clock_out_msb,
        i2c_bbpll_en_audio_clock_out_lsb,
        1
    );

    // Clear I2S subsystem
    CLEAR_MASK_BITS(I2S.CONF, I2S_CONF_RESET_MASK);
    SET_MASK_BITS(I2S.CONF, I2S_CONF_RESET_MASK);
    CLEAR_MASK_BITS(I2S.CONF, I2S_CONF_RESET_MASK);

    // Set i2s clk freq 
    I2S.CONF = SET_FIELD(I2S.CONF, I2S_CONF_BCK_DIV, 62);
    I2S.CONF = SET_FIELD(I2S.CONF, I2S_CONF_CLKM_DIV, 2);
    I2S.CONF = SET_FIELD(I2S.CONF, I2S_CONF_BITS_MOD, 0);

    tx_flags = xEventGroupCreate();
    xEventGroupSetBits(tx_flags, TX_FLAG_READY);
}


int ir_tx_send(ir_encoder_t *encoder) {
    if (xEventGroupWaitBits(tx_flags, TX_FLAG_READY, pdTRUE, pdTRUE, 200 / portTICK_PERIOD_MS) == 0)
        return -1;

    hw_timer_init((void(*)(void *)) ir_tx_timer_handler, encoder);

    ir_tx_timer_handler(encoder);

    return 0;
}

