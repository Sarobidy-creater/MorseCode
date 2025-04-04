#include "main.h"
#include <stdio.h>
#include <string.h>

// Déclaration des périphériques ADC, Timer et UART
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart2;

// Définition des seuils et durées
#define THRESHOLD 2000         // Seuil pour détecter un son
#define DOT_DURATION 500       // Durée maximale pour un point (en millisecondes)

// Buffers de stockage du code Morse et du texte décodé
char morse_code[100];
char texte[100];
uint8_t texte_index = 0;
uint8_t morse_index = 0;

// Variables pour mesurer les durées de son et de silence
uint32_t sound_start = 0;
uint32_t sound_end = 0;
uint32_t silence_d = 0;
uint32_t silence_f = 0;
uint32_t sound_d = 0;
uint32_t sound_f = 0;
uint8_t is_sound_detected = 0;

// Tableaux contenant l'alphabet et leur équivalent Morse
char tabAlphabet[36] = {'a','b','c','d','e','f','g','h','i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
const char *codeMorse[36] = {"01", "1000", "1010", "100", "0", "0010", "110", "0000", "00", "0111", "101", "0100", "11", "10", "111", "0110", "1101", "010", "000", "1", "001", "0001", "011", "1001", "1011", "1100", "11111", "01111", "00111", "00011", "00001", "00000", "10000", "11000", "11100", "11110"};

// Déclarations des fonctions
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART2_UART_Init(void);
void UART_SendString(char *str);
int recherche_morse(char* str);

// Fonction principale
int main(void) {
    HAL_Init();                      // Initialisation de la HAL
    SystemClock_Config();           // Configuration de l'horloge
    MX_GPIO_Init();                 // Initialisation des GPIO
    MX_ADC1_Init();                 // Initialisation de l'ADC
    MX_TIM2_Init();                 // Initialisation du Timer 2
    MX_USART2_UART_Init();          // Initialisation de l'UART

    HAL_ADC_Start(&hadc1);          // Démarrage de l'ADC
    HAL_TIM_Base_Start_IT(&htim2); // Démarrage du timer avec interruption

    while (1) {
        if (HAL_ADC_PollForConversion(&hadc1, 1000) == HAL_OK) {
            uint32_t adc_value = HAL_ADC_GetValue(&hadc1);  // Lecture de la valeur ADC
            uint32_t max_adc_value = adc_value;

            sound_d = __HAL_TIM_GET_COUNTER(&htim2);
            sound_f = __HAL_TIM_GET_COUNTER(&htim2);

            // Sur 50 ms, on cherche le pic maximal du son
            while ((sound_f - sound_d) * (1000.0 / 5250.0) < 50){
                adc_value = HAL_ADC_GetValue(&hadc1);
                sound_f = __HAL_TIM_GET_COUNTER(&htim2);
                if (adc_value > max_adc_value){
                    max_adc_value = adc_value;
                }
            }

            // Si un son est détecté
            if (max_adc_value > THRESHOLD && !is_sound_detected) {
                sound_start = __HAL_TIM_GET_COUNTER(&htim2);
                is_sound_detected = 1;
                UART_SendString("[DETECT] Début du son\n");
                silence_f = __HAL_TIM_GET_COUNTER(&htim2);
            }

            // Si le son s'arrête
            if (max_adc_value < THRESHOLD && is_sound_detected) {
                silence_d = __HAL_TIM_GET_COUNTER(&htim2);
                silence_f = 0;
                uint32_t sound_duration = (__HAL_TIM_GET_COUNTER(&htim2) - sound_start) * (1000.0 / 5250.0);

                char str[50];
                sprintf(str, "[INFO] Durée du son : %lu ms\r\n", sound_duration);
                UART_SendString(str);

                // Enregistre comme point ou tiret selon la durée
                if (sound_duration < DOT_DURATION) {
                    morse_code[morse_index] = '0';  // Point
                } else {
                    morse_code[morse_index] = '1';  // Tiret
                }
                morse_index++;

                UART_SendString("[CODE] Séquence Morse : ");
                UART_SendString(morse_code);
                UART_SendString("\r\n");

                is_sound_detected = 0;
            }

            // Fin de lettre si silence long
            if ((silence_f - silence_d) * (1000.0 / 5250.0) > 900 && silence_f != 0 && silence_d != 0){
                int ind = recherche_morse(morse_code);
                if (ind != -1){
                    texte[texte_index] = tabAlphabet[ind];
                } else {
                    texte[texte_index] = '?'; // Si code inconnu
                }
                texte_index++;

                // Réinitialisation du code Morse
                for (int k = 0 ; k < 7 ; k++){
                    morse_code[k] = '\0';
                }
                morse_index = 0;

                UART_SendString("[TEXTE] Lettre détectée : ");
                UART_SendString(texte);
                UART_SendString("\r\n");

                silence_f = silence_d = 0;
            }

            // Fin de mot si très long silence
            if ((silence_f - silence_d) * (1000.0 / 5250.0) > 1900 && silence_f != 0 && silence_d != 0){
                texte[texte_index] = '_';
                texte_index++;
                silence_f = silence_d = 0;
            }
        }
    }
}

// Fonction de recherche de la lettre correspondant à une séquence Morse
int recherche_morse(char *str){
	int index = -1;
	int i = 0;
	while (index == -1 && i < 36){
		if (strcmp(codeMorse[i], str) == 0){
			index = i;
		}
		i++;
	}
	return index;
}

/* Configuration de l'ADC */
void MX_ADC1_Init(void) {
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);

    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

/* Configuration du Timer TIM2 */
void MX_TIM2_Init(void) {
	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 16000 - 1;              // Horloge divisée pour obtenir 1 ms par incrément
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 0xFFFF;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	HAL_TIM_Base_Init(&htim2);
	HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

/* Configuration de l'UART (USART2) */
void MX_USART2_UART_Init(void) {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

/* Envoie une chaîne de caractères via UART */
void UART_SendString(char *str) {
    HAL_UART_Transmit(&huart2, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

/* Configuration de l'horloge système */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                    | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/* Configuration des GPIO */
static void MX_GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
}
