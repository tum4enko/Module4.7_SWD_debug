/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#define LED_PIN       13U    /* PC13 — синій світлодіод, світиться при 0 */
#define BTN_PIN       0U     /* PA0  — кнопка KEY, замикає на GND */
#define DEBOUNCE_MS   20U

static const uint32_t k_periods_ms[] = { 1000U, 500U, 250U, 100U, 2000U };
#define PERIODS_COUNT (sizeof(k_periods_ms) / sizeof(k_periods_ms[0]))

volatile uint32_t g_ms         = 0;      /* мілісекунди від старту (рахує TIM2) */
volatile uint32_t g_period_ms  = 1000U;  /* повний період блимання */
volatile uint32_t g_period_idx = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void     led_init(void);
static void     led_toggle(void);
static void     button_init(void);
static uint32_t button_is_pressed(void);
static uint32_t apb1_timer_clock(void);
static void     tim2_init_1khz(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  led_init();
  button_init();
  tim2_init_1khz();

  uint32_t last_toggle_ms = 0;
  uint32_t last_sample_ms = 0;
  uint32_t btn_stable     = 0;   /* 0 = відпущена */
  uint32_t btn_counter    = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t now = g_ms;

    /* Blink: перемикаємо кожні півперіоду */
    if ((now - last_toggle_ms) >= (g_period_ms / 2U)) {
      last_toggle_ms = now;
      led_toggle();
    }

    /* Кнопка: опитування раз на 1 мс + антибрязкіт 20 мс */
    if (now != last_sample_ms) {
      last_sample_ms = now;
      uint32_t raw = button_is_pressed();
      if (raw != btn_stable) {
        if (++btn_counter >= DEBOUNCE_MS) {
          btn_stable  = raw;
          btn_counter = 0;
          if (btn_stable) {                       /* момент натискання */
            g_period_idx = (g_period_idx + 1U) % PERIODS_COUNT;
            g_period_ms  = k_periods_ms[g_period_idx];
          }
        }
      } else {
        btn_counter = 0;
      }
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 12;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
static void led_init(void)
{
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;        /* тактування порту C */
  (void)RCC->AHB1ENR;                         /* пауза після ввімкнення клоку */

  GPIOC->MODER   &= ~(3U << (LED_PIN * 2U));  /* скинути режим */
  GPIOC->MODER   |=  (1U << (LED_PIN * 2U));  /* 01 = output */
  GPIOC->OTYPER  &= ~(1U << LED_PIN);         /* push-pull */
  GPIOC->OSPEEDR &= ~(3U << (LED_PIN * 2U));  /* low speed */
  GPIOC->PUPDR   &= ~(3U << (LED_PIN * 2U));  /* без підтяжки */

  GPIOC->BSRR = (1U << LED_PIN);              /* PC13 = 1 → LED вимкнено */
}

static void led_toggle(void)
{
  /* Як у HAL_GPIO_TogglePin: атомарно через BSRR */
  uint32_t odr = GPIOC->ODR;
  GPIOC->BSRR = ((odr & (1U << LED_PIN)) << 16U) | (~odr & (1U << LED_PIN));
}

static void button_init(void)
{
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  (void)RCC->AHB1ENR;

  /* ТІЛЬКИ read-modify-write! PA13/PA14 = SWDIO/SWCLK */
  GPIOA->MODER &= ~(3U << (BTN_PIN * 2U));    /* 00 = input */
  GPIOA->PUPDR &= ~(3U << (BTN_PIN * 2U));
  GPIOA->PUPDR |=  (1U << (BTN_PIN * 2U));    /* 01 = pull-up */
}

static uint32_t button_is_pressed(void)
{
  return (GPIOA->IDR & (1U << BTN_PIN)) ? 0U : 1U;  /* натиснута = 0 на піні */
}

static uint32_t apb1_timer_clock(void)
{
  uint32_t ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;
  if (ppre1 < 4U) {
    return SystemCoreClock;
  }
  uint32_t div = 1U << (ppre1 - 3U);
  return (SystemCoreClock / div) * 2U;        /* таймери APB1 тактуються ×2 */
}

static void tim2_init_1khz(void)
{
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
  (void)RCC->APB1ENR;

  TIM2->CR1  = 0;
  TIM2->PSC  = (apb1_timer_clock() / 1000000U) - 1U;  /* лічильник 1 МГц */
  TIM2->ARR  = 1000U - 1U;                             /* переповнення раз на 1 мс */
  TIM2->EGR  = TIM_EGR_UG;
  TIM2->SR   = 0;
  TIM2->DIER = TIM_DIER_UIE;

  NVIC_SetPriority(TIM2_IRQn, 5);
  NVIC_EnableIRQ(TIM2_IRQn);

  TIM2->CR1 = TIM_CR1_CEN;
}

void TIM2_IRQHandler(void)
{
  if (TIM2->SR & TIM_SR_UIF) {
    TIM2->SR = ~TIM_SR_UIF;
    g_ms++;
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
