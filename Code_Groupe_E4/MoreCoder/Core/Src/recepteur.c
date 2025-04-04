/* USER CODE BEGIN Header */
/**
 * Code pour traduire le code morse en texte
 * et le transmettre via UART lors de l'appui sur un bouton
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    char letter;
    const char *morse;
} MorseCode;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SEUIL_SIGNAL 500  		// Seuil pour considérer qu'un signal est détecté
#define SEUIL_POINT 200    		// Durée en ms d'un point
#define SEUIL_TRAIT 600   		// Durée en ms d'un trait
#define SEUIL_FIN_LETTRE 800 	// Durée en ms d'un silence pour marque la fin d'une lettre
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
const MorseCode morseTable[] = {
    {'A', ".-"}, {'B', "-..."}, {'C', "-.-."}, {'D', "-.."}, {'E', "."},
    {'F', "..-."}, {'G', "--."}, {'H', "...."}, {'I', ".."}, {'J', ".---"},
    {'K', "-.-"}, {'L', ".-.."}, {'M', "--"}, {'N', "-."}, {'O', "---"},
    {'P', ".--."}, {'Q', "--.-"}, {'R', ".-."}, {'S', "..."}, {'T', "-"},
    {'U', "..-"}, {'V', "...-"}, {'W', ".--"}, {'X', "-..-"}, {'Y', "-.--"},
    {'Z', "--.."}, {'1', ".----"}, {'2', "..---"}, {'3', "...--"}, {'4', "....-"},
    {'5', "....."}, {'6', "-...."}, {'7', "--..."}, {'8', "---.."}, {'9', "----."},
    {'0', "-----"}, {' ', "/"}  // Espace entre les mots en Morse = "/"
};
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
char messageMorse[128] = ""; // Déclaré comme variable globale pour être accessible dans l'interruption
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
char detect_morse(char *messageMorse, size_t bufferSize);
char morseToChar(const char *morse);
const char* charToMorse(char letter);
void text_to_morse(const char *text, char *output);
void morse_to_text(const char *textMorse, char *output, size_t outputSize);
uint32_t read_microphone(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Ecoutes le microphone et traduit l'intensité du son en lumière
uint32_t read_microphone(void)
{
    HAL_ADC_Start(&hadc1);  // Démarre la conversion de l'ADC
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);  // Attend la fin de la conversion
    return HAL_ADC_GetValue(&hadc1);  // Retourne la valeur mesurée par l'ADC
}

/* redirection printf dans la console*/
#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small
printf set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

PUTCHAR_PROTOTYPE
{
/* Place your implementation of fputc here */
/* e.g. write a character to the USART2 and Loop until the end of transmission */
HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
return ch;
}

/**
 * @brief Détecte les signaux du microphone et les convertit en code Morse.
 * Cette fonction analyse les durées des signaux reçus pour déterminer
 * s'ils correspondent à un point (.), un trait (-) ou une fin de lettre (espace).
 *
 * @param messageMorse Buffer où sera stocké le message Morse détecté
 * @param bufferSize Taille du buffer pour éviter les débordements
 * @return Le caractère morse détecté (., -, espace) ou '\0' si rien n'est détecté
 */
char detect_morse(char *messageMorse, size_t bufferSize)
{
    uint32_t microphone_value = read_microphone();

    static uint32_t start_time = 0;
    static int detecting = 0;

    if (microphone_value > SEUIL_SIGNAL && !detecting) { // Si un signal est détecté
        start_time = HAL_GetTick();
        detecting = 1;
    }
    else if (microphone_value <= SEUIL_SIGNAL && detecting) { // Fin du signal
        detecting = 0;
        uint32_t duration = HAL_GetTick() - start_time;

        if (duration < SEUIL_POINT) { // détecte un point et le stocke
            strncat(messageMorse, ".", bufferSize - strlen(messageMorse) - 1);
            return '.';
        }
        else if (duration >= SEUIL_POINT && duration < SEUIL_TRAIT) { // détecte un trait et le stocke
            strncat(messageMorse, "-", bufferSize - strlen(messageMorse) - 1);
            return '-';
        }
        else if (duration >= SEUIL_FIN_LETTRE) { // détecte la fin d'une lettre
            // Fin de lettre
            strncat(messageMorse, " ", bufferSize - strlen(messageMorse) - 1);
            return ' ';
        }
    }
    return '\0'; // Aucun caractère détecté
}

/**
 * @brief Convertit un code Morse en caractère ASCII.
 *
 * @param morse Chaîne contenant le code Morse.
 * @return Le caractère ASCII correspondant, ou '?' si inconnu.
 */
char morseToChar(const char *morse) {
    for (int i = 0; i < sizeof(morseTable) / sizeof(MorseCode); i++) {
        if (strcmp(morseTable[i].morse, morse) == 0) {
            return morseTable[i].letter; // la lettre
        }
    }
    return '?';  // Retourne '?' si inconnu
}

/**
 * @brief Récupère le code Morse correspondant à une lettre.
 *
 * @param letter Caractère à convertir en Morse.
 * @return Chaîne contenant le code Morse ou "?" si inconnu.
 */
const char* charToMorse(char letter) {
    // Convertir en majuscule si c'est une lettre minuscule
    if (letter >= 'a' && letter <= 'z') {
        letter = letter - 'a' + 'A';
    }

    for (int i = 0; i < sizeof(morseTable) / sizeof(MorseCode); i++) {
        if (morseTable[i].letter == letter) {
            return morseTable[i].morse; // le code morse de la lettre
        }
    }
    return "?";  // Retourne '?' si inconnu
}

/**
 * @brief Traduit un texte en code Morse et ajoute un espace entre chaque lettre en Morse.
 *
 * @param text   : Chaîne contenant le texte à convertir.
 * @param output : Buffer où sera stocké le message Morse.
 */
void text_to_morse(const char *text, char *output) {
    output[0] = '\0';  // Initialise la chaîne vide
    for (int i = 0; i < strlen(text); i++) {
        const char *morseCode = charToMorse(text[i]); //On traduit chaque caractère en morse
        strcat(output, morseCode); // Ajoute le code Morse dans la sortie
        strcat(output, " "); // Ajoute un espace entre chaque lettre
    }
}

/**
 * @brief Traduit un message Morse en texte ASCII.
 * Cette fonction gère les espaces simples (séparation des lettres)
 * et doubles espaces (séparation des mots).
 *
 * @param textMorse Chaîne contenant le code Morse.
 * @param output Buffer où sera stocké le texte traduit.
 * @param outputSize Taille du buffer de sortie pour éviter un débordement.
 */
void morse_to_text(const char *textMorse, char *output, size_t outputSize) {
    output[0] = '\0';// Initialise la sortie avec une chaîne vide
    char buffer[10] = ""; // Stocke un caractère Morse temporaire
    int bufIndex = 0;

    for (size_t i = 0; i <= strlen(textMorse); i++) {
        if (textMorse[i] == '.' || textMorse[i] == '-') {
            buffer[bufIndex++] = textMorse[i]; // Ajoute le caractère au buffer temporaire
        } else if (textMorse[i] == ' ' || textMorse[i] == '\0') {
            buffer[bufIndex] = '\0';			// Termine la chaîne de la lettre Morse
            if (bufIndex > 0) { // Si on a des caractères morse à traiter
                char decoded = morseToChar(buffer); // Convertit en caractère ASCII
                strncat(output, &decoded, 1);		// Ajoute le caractère décodé à la sortie
                bufIndex = 0;						// Réinitialise l'index du buffer temporaire
            }

            // Vérifie si c'est un espace double (indiquant un mot)
            if (textMorse[i] == ' ' && textMorse[i + 1] == ' ') {
                strncat(output, " ", outputSize - strlen(output) - 1); // Ajoute un espace entre les mots
                i++; // Sauter le deuxième espace
            }
        }
    }
}

/**
 * @brief Fonction de gestion d'interruption pour le bouton B1.
 * Cette fonction est appelée lorsque le bouton B1 est pressé.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == B1_Pin) {
    // Récupère le message morse actuel et le décode
    char textBuffer[64] = "";
    morse_to_text(messageMorse, textBuffer, sizeof(textBuffer));

    // Transmet le message au PC
    printf("\r\n=== MESSAGE TRANSMIS ===\r\n");
    printf("Message Morse : %s\r\n", messageMorse);
    printf("Texte décodé : %s\r\n", textBuffer);
    printf("======================\r\n\r\n");

    // Réinitialise le message morse pour une nouvelle acquisition
    messageMorse[0] = '\0';
  }
}
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
  MX_ADC1_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */
  printf("\r\n=== SYSTÈME DE DÉCODAGE MORSE DÉMARRÉ ===\r\n");
  printf("Utilisez le microphone pour capturer du morse.\r\n");
  printf("Appuyez sur le bouton B1 pour transmettre le message au PC.\r\n\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN 3 */
  while (1) {
    /* Capture et stocke les caractères morse détectés */
    char morseChar = detect_morse(messageMorse, sizeof(messageMorse));

    /* Affiche le caractère morse détecté (pour débogage) */
    if (morseChar != '\0') {
      printf("%c", morseChar);
    }

    /* Permet de voir le message en cours d'acquisition (toutes les secondes) */
    static uint32_t lastDisplayTime = 0;
    uint32_t currentTime = HAL_GetTick();

    if (strlen(messageMorse) > 0 && (currentTime - lastDisplayTime) >= 1000) {
      printf("\rMessage en cours : %s", messageMorse);
      lastDisplayTime = currentTime;
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */
  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */
  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */
  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* Enable interrupt for button B1 */
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* USER CODE END Error_Handler_Debug */
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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
