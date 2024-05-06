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
#include "shell_ext.h"
#include "shell_port.h"
#include "bsp_gpio.h"
#include "debug_print.h"
#include "stdlib.h" 
#include "main.h" 

static void parse_arguments(int argc, char **argv);

static int setGPIOActiveHandler(int argc, char **argv, int index);
static int getGPIOActive(int argc, char **argv, int index);

static int (*const handlerList[])(int, char **, int) =  {
    setGPIOActiveHandler,
    getGPIOActive,
    NULL
};

static char *const arglist[] = {
    "-s",
    "-g",
    NULL
};

static void display_usage(void)
{
    LOG_RAW("\t\tgpio\r\n");
    LOG_RAW("Usage: get or set gpio  <arguments>\r\n");
    LOG_RAW("Arguments:\r\n");

    LOG_RAW("\n*** gpio cmd index ***\r\n");
    LOG_RAW("\t-g: get the gpio is actived,eg: gpio -g 0\r\n");
    LOG_RAW("\t-s <active>:set the gpio active,eg: gpio -s 0 1\r\n");
    GPIO_printIdexAndName();
    return;
}

static void parse_arguments(int argc, char **argv)
{
    int i, j;

    if (argc <= 1)
    {
        display_usage();
        return;
    }

    for (i = 1; i < argc; i++)
    {
        j = 0;
        while (arglist[j] != NULL)
        {
            if (strcmp(argv[i], arglist[j]) == 0)
            {
                int retval;

                /* Match!  Handle this argument (and skip the specified
                   number of arguments that were just handled) */
                retval = handlerList[j](argc, argv, i);
                if (retval >= 0)
                    i += retval;
                else
                {
                    LOG_E("Cannot handle argument: %s\r\n", arglist[j]);
                }
            }
            j++;
        }
    }
}
static void int16ToBinaryString(int16_t num, char *binaryString) {
    int index = 0;
    bool is0 = true;
    // 从最高位开始，逐位检查并存放到字符串数组中
    for (int i = 31; i >= 0; i--) {
        int bit = GET_BIT(num, i);
        if (is0 && (bit == 1)) {
            is0 = false;
        }
        if (is0){
            continue;
        }
        binaryString[index++] = GET_BIT(num, i) + '0';
        // 在每四位之后添加一个空格
        if (i % 4 == 0 && i != 0)
            binaryString[index++] = ' ';
    }
    // 在字符串末尾添加结束符
    if (index == 0){
        binaryString[index++] = '0';
    }
    binaryString[index] = '\0';
}
static int getGPIOActive(int argc, char **argv, int index)
{
    GPIO_Idex gpioIndex = (GPIO_Idex)atoi(argv[argc - 1]);
    GPIO_PinState config;
    const char **name;
    int res = GPIO_isPinActive(gpioIndex, &config);
    GPIO_getPinName(gpioIndex, name);
    if (res == -1) {
        LOG_RAW("gpio index = %d: get gpio error, check the input para\r\n", gpioIndex);
    }else{
        char binaryString[40];
        int16ToBinaryString(res, binaryString);
        LOG_RAW("index ,    name,              config ActiveSignal,  is actived/val \r\n");
        LOG_RAW("  %d,    %-20s,          %d,               %s\r\n", gpioIndex, *name, config, binaryString);
    }
    return 0;
}
static int setGPIOActiveHandler(int argc, char **argv, int index)
{
    GPIO_Idex gpioIndex = (GPIO_Idex)atoi(argv[argc - 1]);
    FunctionalState isActive = (FunctionalState)atoi(argv[argc - 1]);
    GPIO_PinState config;
    if (!GPIO_setPinStatus(gpioIndex, isActive, &config)) {
        LOG_RAW("gpio index = %d: set gpio error, check the input para\r\n", gpioIndex);
    }else{
        LOG_RAW("set gpio,    config ActiveSignal,  set actived\r\n");
        LOG_RAW("  %d,           %d,                  %d\r\n", gpioIndex, config, isActive);
    }
    return 0;
}

static int GPIO_shell(int argc, char *argv[])
{
    parse_arguments(argc, argv);
    return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, gpio, GPIO_shell, get/set gpio);


