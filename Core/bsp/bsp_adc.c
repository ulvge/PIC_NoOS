
#include "stm32h7xx_hal.h"
#include "bsp_adc.h"

ADC_HandleTypeDef AdcHandle;
ADC_ChannelConfTypeDef        sConfig;

#define ADC_CONVERTED_DATA_BUFFER_SIZE   (8)

static uint16_t g_ADCxConvertedData[ADC_CONVERTED_DATA_BUFFER_SIZE] __attribute__((section(".MY_SECTION"), aligned(32)));

// ADC 分辨率
#define ADC_VOLTAGE_INCREMENT   0.01294f
// ADC 电压转换系数
#define ADC_3V3_5V_K            1.55556f

__attribute__((unused)) static bool g_adcConvertFinished = false;
void ADC_init(void)
{
    /* ### - 1 - Initialize ADC peripheral #################################### */
    AdcHandle.Instance = ADCx;
    if (HAL_ADC_DeInit(&AdcHandle) != HAL_OK) {
        /* ADC de-initialization Error */
        Error_Handler();
    }

    AdcHandle.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV64;                      /* Asynchronous clock mode, input ADC clock divided by 2*/
    AdcHandle.Init.Resolution = ADC_RESOLUTION_12B;                            /* 16-bit resolution for converted data */
    AdcHandle.Init.ScanConvMode = DISABLE;                                     /* Sequencer disabled (ADC conversion on only 1 channel: channel set on rank 1) */
    AdcHandle.Init.EOCSelection = ADC_EOC_SINGLE_CONV;                         /* EOC flag picked-up to indicate conversion end */
    AdcHandle.Init.LowPowerAutoWait = DISABLE;                                 /* Auto-delayed conversion feature disabled */
    AdcHandle.Init.ContinuousConvMode = ENABLE;                                /* Continuous mode enabled (automatic conversion restart after each conversion) */
    AdcHandle.Init.NbrOfConversion = 1;                                        /* Parameter discarded because sequencer is disabled */
    AdcHandle.Init.DiscontinuousConvMode = DISABLE;                            /* Parameter discarded because sequencer is disabled */
    AdcHandle.Init.NbrOfDiscConversion = 1;                                    /* Parameter discarded because sequencer is disabled */
    AdcHandle.Init.ExternalTrigConv = ADC_SOFTWARE_START;                      /* Software start to trig the 1st conversion manually, without external event */
    AdcHandle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;       /* Parameter discarded because software trigger chosen */
    AdcHandle.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR; /* ADC DMA circular requested */
    AdcHandle.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;                         /* DR register is overwritten with the last conversion result in case of overrun */
    AdcHandle.Init.OversamplingMode = DISABLE;                                 /* No oversampling */
    /* Initialize ADC peripheral according to the passed parameters */
    if (HAL_ADC_Init(&AdcHandle) != HAL_OK) {
        Error_Handler();
    }

    /* ### - 2 - Start calibration ############################################ */
    if (HAL_ADCEx_Calibration_Start(&AdcHandle, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED) != HAL_OK) {
        Error_Handler();
    }

    /* ### - 3 - Channel configuration ######################################## */
    sConfig.Channel = ADCx_CHANNEL;                  /* Sampled channel number */
    sConfig.Rank = ADC_REGULAR_RANK_1;               /* Rank of sampled channel number ADCx_CHANNEL */
    sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5; /* Sampling time (number of clock cycles unit) */
    sConfig.SingleDiff = ADC_SINGLE_ENDED;           /* Single-ended input channel */
    sConfig.OffsetNumber = ADC_OFFSET_NONE;          /* No offset subtraction */
    sConfig.Offset = 0;                              /* Parameter discarded because offset correction is disabled */
    sConfig.OffsetSignedSaturation = DISABLE;
    if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK) {
        Error_Handler();
    }

    /* ### - 4 - Start conversion in DMA mode ################################# */
    if (HAL_ADC_Start_DMA(&AdcHandle,
                          (uint32_t *)g_ADCxConvertedData,
                          ADC_CONVERTED_DATA_BUFFER_SIZE) != HAL_OK) {
        Error_Handler();
    }
}
/// adc cplt 回调函数
__attribute__((unused)) static void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    g_adcConvertFinished = true;
}
/**
 * @brief 获取ADC值
 * @return 返回电压值
 */
float ADC_get_value(void)
{
    uint32_t adcSum = 0;
    // 取样数据 累加
    for (size_t i = 0; i < ADC_CONVERTED_DATA_BUFFER_SIZE; i++)
    {
        adcSum += g_ADCxConvertedData[i];
    }
    
    /* 计算平均值 */
    float vol3V3 = (adcSum * ADC_VOLTAGE_INCREMENT) / ADC_CONVERTED_DATA_BUFFER_SIZE ;
    /* 转换 电压值 */
    float vol5V = vol3V3 * ADC_3V3_5V_K;
    return vol5V;
}
