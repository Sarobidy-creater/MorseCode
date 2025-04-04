
# Morse Code Decoder & Encoder (STM32 Project)

This project implements a full Morse code system using STM32 microcontroller peripherals. It includes both **Morse decoding** (based on sound detection) and **Morse playback** (via buzzer output). Designed using STM32 HAL libraries.

---

## Project Structure

- `main.c`: Contains the core logic for both the decoder and encoder.
- `main.h`: Header file (not included here) assumed to declare peripherals and external functions.

---

## Features

### Morse Decoder (Sound to Text)
- Detects audio input via ADC.
- Uses a timer to measure pulse durations.
- Converts audio pulses to Morse code.
- Decodes Morse code to alphanumeric text.
- Sends results over UART (USART2).

### Morse Encoder (Text to Sound)
- Accepts a predefined string (e.g., `"sos"`).
- Converts each character to Morse.
- Plays tones (dots and dashes) on a buzzer using GPIO.
- Timed signal generation using TIM1.

---

## Hardware Requirements

- STM32 MCU (e.g., STM32F4 series)
- ADC input for sound capture
- Timer peripheral for timekeeping (TIM1 and TIM2)
- UART interface for debugging/output (USART2)
- GPIO output (e.g., PA0) connected to a piezo buzzer

---

## Key Parameters

- `THRESHOLD`: ADC threshold for sound detection (`2000`)
- `DOT_DURATION`: Max duration for a dot (`500ms decoder`, `300ms encoder`)
- `DASH_DURATION`: Dash duration (`900ms`)
- `LETTER_SPACE`, `WORD_SPACE`: Used to distinguish Morse letters and words.

---

## Usage

### Decoder Loop (in main.c)
1. ADC continuously samples audio input.
2. If signal exceeds `THRESHOLD`, it records the duration.
3. Signal length determines dot or dash (`< DOT_DURATION` → dot).
4. Gaps determine letter or word boundaries.
5. Morse code is converted into text.

### Encoder Function
```c
char phrase[] = "sos";
convertirEtJouerMorse(phrase);
```
- Plays the string in Morse code using buzzer on PA0.

---

## UART Output Example (Decoder)

```
[DETECT] Début du son
[INFO] Durée du son : 450 ms
[CODE] Séquence Morse : 0
[TEXTE] Lettre détectée : e
```

---

## License

This code is © 2025 STMicroelectronics. Provided AS-IS unless stated otherwise.

