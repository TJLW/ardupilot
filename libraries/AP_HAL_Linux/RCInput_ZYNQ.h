#pragma once

/*
  This class implements RCInput on the ZYNQ / ZyboPilot platform with custom
  logic doing the edge detection of the PPM sum input
 */

#include "RCInput.h"

namespace Linux {

class RCInput_ZYNQ : public RCInput {
public:
    void init() override;
    void _timer_tick(void) override;

private:
#if CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_LINUX_OCPOC_ZYNQ
    static const int TICK_PER_US=50;
    static const int TICK_PER_S=50000000;
#else
    static const int TICK_PER_US=100;
    static const int TICK_PER_S=100000000;
#endif


#if CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_LINUX_ZYBOZ7_ZYNQ || \
    CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_LINUX_ULTRA96_ZYNQMP
	// Memory mapped keyhole register to pwm decoder
    uint16_t *pwm_channel_inputs;
#else
	// Memory mapped keyhole register to pulse input FIFO
	volatile uint32_t *pulse_input;
#endif

    // time spent in the low state
    uint32_t _s0_time;
};

}
