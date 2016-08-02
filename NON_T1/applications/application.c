/*
 * File      : application.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 * 2013-07-12     aozima       update for auto initial.
 */

/**
 * @addtogroup STM32
 */
/*@{*/

#include <board.h>
#include <rtthread.h>
#include <components.h>
#include "led.h"

#include "non.h"

ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t led_stack[256];
static struct rt_thread led_thread;

#ifdef RT_USING_HOOK
static void led_flash()
{
	volatile rt_uint32_t i;
	static rt_uint8_t sta = 0;

	while (1) {
		for(i = 0; i < 540000; i++);
		if(sta){
			rt_hw_led_on(1);
			sta = 0;
		}else{
			rt_hw_led_off(1);
			sta = 1;
		}
	}
}
#endif
static void led_thread_entry(void* parameter)
{
    rt_hw_led_init();

#ifdef RT_USING_HOOK
         rt_thread_idle_sethook(led_flash);
#endif
    while (1)
    {
        /* led1 on */
        rt_hw_led_on(0);
        rt_thread_delay( RT_TICK_PER_SECOND );

        /* led1 off */
        rt_hw_led_off(0);
        rt_thread_delay( RT_TICK_PER_SECOND );
    }
}


void rt_init_thread_entry(void* parameter)
{
    rt_components_init();
}
int rt_application_init(void)
{
    rt_thread_t init_thread;

    rt_err_t result;

    /* init led thread */
    result = rt_thread_init(&led_thread,
                            "led",
                            led_thread_entry,
                            RT_NULL,
                            (rt_uint8_t*)&led_stack[0],
                            sizeof(led_stack),
                            20,
                            5);
    if (result == RT_EOK){
        rt_thread_startup(&led_thread);
    }

    init_thread = rt_thread_create("init",
                                   rt_init_thread_entry, RT_NULL,
                                   2048, 8, 20);

    if (init_thread != RT_NULL)
        rt_thread_startup(init_thread);

    init_thread = rt_thread_create("non init",
									non_init, RT_NULL,
									2048, 20, 100);
    if (init_thread != RT_NULL){
        rt_thread_startup(init_thread);
    }

    return 0;
}

/*@}*/
