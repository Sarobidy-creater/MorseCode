/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim1;

/* Function prototypes -------------------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
void Timer_Wait(uint16_t ms);

// Définition des durées en millisecondes
#define DOT_DURATION          300     // Durée d'un point
#define DASH_DURATION         900     // Durée d'un tiret
#define SYMBOL_SPACE          300     // Espace entre symboles
#define LETTER_SPACE          1000     // Espace entre lettres
#define WORD_SPACE            2000     // Espace entre mots

// Définition du pin du buzzer
#define BUZZER_PIN            GPIO_PIN_0
#define BUZZER_PORT           GPIOA

// Dictionnaire Morse
const char *morse[36] = {
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",
    "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",
    "..-", "...-", ".--", "-..-", "-.--", "--..",
    "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----."
};

// Initialisation du Timer 1
void MX_TIM1_Init(void) {
    __HAL_RCC_TIM1_CLK_ENABLE();

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 16000 - 1;  // 16 MHz / 16000 = 1 kHz (1 ms par tick)
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 65535;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&htim1) != HAL_OK) {
        Error_Handler();
    }
}

// Fonction pour attendre un certain temps via TIM1
void Timer_Wait(uint16_t ms) {
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    HAL_TIM_Base_Start(&htim1);
    while (__HAL_TIM_GET_COUNTER(&htim1) < ms);
    HAL_TIM_Base_Stop(&htim1);
}

void buzzerOn() {
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
}

void buzzerOff() {
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
}

void playDot() {
    buzzerOn();
    Timer_Wait(DOT_DURATION);
    buzzerOff();
    Timer_Wait(SYMBOL_SPACE);
}

void playDash() {
    buzzerOn();
    Timer_Wait(DASH_DURATION);
    buzzerOff();
    Timer_Wait(SYMBOL_SPACE);
}

void playMorseChar(char c) {
    c = toupper(c);

    if (c >= 'A' && c <= 'Z') {
        const char *code = morse[c - 'A'];

        for (int i = 0; code[i] != '\0'; i++) {
            if (code[i] == '.') playDot();
            else if (code[i] == '-') playDash();
        }
        Timer_Wait(LETTER_SPACE - SYMBOL_SPACE);
    }
    else if (c >= '0' && c <= '9') {
        const char *code = morse[c - '0' + 26];
        for (int i = 0; code[i] != '\0'; i++) {
            if (code[i] == '.') playDot();
            else if (code[i] == '-') playDash();
        }
        Timer_Wait(LETTER_SPACE - SYMBOL_SPACE);
    }
    else if (c == ' ') {
        Timer_Wait(WORD_SPACE - LETTER_SPACE);
    }
}

void convertirEtJouerMorse(const char *phrase) {
    int i = 0;
    while (phrase[i] != '\0') {
        playMorseChar(phrase[i]);
        i++;
    }
}

void initBuzzer() {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = BUZZER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUZZER_PORT, &GPIO_InitStruct);

    buzzerOff();
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM1_Init();
    initBuzzer();

    char phrase[] = "sos";
    uint8_t firstPass = 1;  // Variable pour ignorer la première exécution
    convertirEtJouerMorse(phrase);
    while (1) {
    	if (!firstPass) {  // Ignore la première boucle
    	            convertirEtJouerMorse(phrase);
    	        } else {
    	            firstPass = 0;  // Désactive le flag après la première itération
    	        }
    	        Timer_Wait(200);  // Pause entre répétitions
    	    }
}

void SystemClock_Config(void) {
    // Configuration de l'horloge (inchangée)
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|LD2_Pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_0|LD2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}

#ifdef  USE_FULL_ASSERT
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
