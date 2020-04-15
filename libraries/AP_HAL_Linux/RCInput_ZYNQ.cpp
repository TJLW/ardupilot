#include "RCInput_ZYNQ.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include <AP_HAL/AP_HAL.h>

#include "GPIO.h"

#if CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_LINUX_OCPOC_ZYNQ
#define RCIN_ZYNQ_PULSE_INPUT_BASE  0x43ca0000
#elif CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_LINUX_ZYBOZ7_ZYNQ
#define RCIN_NUM_CHANNELS  6
#elif CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_LINUX_ULTRA96_ZYNQMP
#define RCIN_NUM_CHANNELS  6
#define RCIN_PWM_INPUT_BASE_ULTRA96_ZYNQMP 0x0080001000
#else
#define RCIN_ZYNQ_PULSE_INPUT_BASE  0x43c10000
#endif

extern const AP_HAL::HAL& hal;

using namespace Linux;

void RCInput_ZYNQ::init()
{
	#if CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_LINUX_ZYBOZ7_ZYNQ

		// The custom hardware for the ZyboZ7 processes PWM inputs and provides
		//	a period value for each of the 8 channels
		int pwm_reader_fd = open("/dev/uio0", O_RDWR|O_SYNC|O_CLOEXEC);
    if (pwm_reader_fd == -1) {
    AP_HAL::panic("Unable to open pwm_reader registers at /dev/uio0");
    }
		pwm_channel_inputs = (uint16_t*) mmap(0, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, pwm_reader_fd, 0x0);


    #elif CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_LINUX_ULTRA96_ZYNQMP

    int mem_fd = open("/dev/mem", O_RDWR|O_SYNC|O_CLOEXEC);
    if (mem_fd == -1) {
        AP_HAL::panic("Unable to open /dev/mem");
    }
    pwm_channel_inputs = (uint16_t*) mmap(0, 0x1000, PROT_READ|PROT_WRITE,
                                                      MAP_SHARED, mem_fd, RCIN_PWM_INPUT_BASE_ULTRA96_ZYNQMP);
    close(mem_fd);


	#else

    int mem_fd = open("/dev/mem", O_RDWR|O_SYNC|O_CLOEXEC);
    if (mem_fd == -1) {
        AP_HAL::panic("Unable to open /dev/mem");
    }
    pulse_input = (volatile uint32_t*) mmap(0, 0x1000, PROT_READ|PROT_WRITE,
                                                      MAP_SHARED, mem_fd, RCIN_ZYNQ_PULSE_INPUT_BASE);
    close(mem_fd);

	#endif

    _s0_time = 0;
}

/*
  called at 1kHz to check for new pulse capture data from the PL pulse timer
 */
void RCInput_ZYNQ::_timer_tick()
{
	#if CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_LINUX_ZYBOZ7_ZYNQ || \
        CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_LINUX_ULTRA96_ZYNQMP
        _update_periods(pwm_channel_inputs, RCIN_NUM_CHANNELS);

	#else
    uint32_t v;

    // all F's means no samples available
    while((v = *pulse_input) != 0xffffffff) {
        // Hi bit indicates pin state, low bits denote pulse length
        if(v & 0x80000000)
            _s0_time = (v & 0x7fffffff)/TICK_PER_US;
        else
            _process_rc_pulse(_s0_time, (v & 0x7fffffff)/TICK_PER_US);
    }
	#endif

}
