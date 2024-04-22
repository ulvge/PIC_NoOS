/**
 * @file shell.c
 * @author Letter (NevermindZZT@gmail.com)
 * @version 3.0.0
 * @date 2019-12-30
 * 
 * @copyright (c) 2020 Letter
 * 
 */

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "main.h" 
#include "shell_ext.h"
#include "shell_port.h"
#include "bsp_adc.h"
#include "debug_print.h"
#include "stdlib.h" 

static void display_usage(void)
{
    LOG_RAW("\t\tadc ,read voltage from pin \r\n");
    return;
}

static int adc_shell(int argc, char *argv[])
{
    if (argc != 1)
    {
        display_usage();
        return 0;
    }
    float vol5v = ADC_get_value();
    LOG_RAW("The voltage of XBEAM is = %0.2f v\r\n", vol5v);

    return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, adc, adc_shell, read adc value);


