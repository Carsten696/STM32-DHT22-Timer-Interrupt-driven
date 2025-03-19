Thanks to Claude 3.5 Sonnet for the (somwhat unhelpful) help.

# DHT22 Library for STM32H7 - Installation Guide

## Hardware Requirements
- STM32H7 microcontroller (e.g., NUCLEO-H753ZI)
- DHT22 temperature and humidity sensor
- Connections:
  - DHT22 data pin → PD3
  - Optional: Debug pin → PD4
  - Pull-up resistor (4.7kΩ) on data pin

## Software Requirements
- STM32CubeIDE
- HAL Library for STM32H7

## CubeMX Configuration

1. **Timer6 Configuration:**
   ```
   Mode: Internal Clock
   Prescaler: 127
   Counter Period: 4
   ```
   Enable Timer6 global interrupt in NVIC

2. **GPIO Configuration:**
   - PD3: GPIO_Output (will be switched by software)
   - PD4: GPIO_Output (for debug signals)

## Library Installation

1. **Copy Files:**
   - `DHT22.h` to `Core/Inc` folder
   - `DHT22.c` to `Core/Src` folder

2. **Add to main.c:**
   ```c
   /* Private variables */
   extern TIM_HandleTypeDef htim6;

   /* USER CODE BEGIN 2 */
   DHT22_Init();
   ```

## Timer Operation

The library uses Timer6 for precise timing during DHT22 communication:
- Timer starts when measurement begins
- Generates 10µs interrupts during data acquisition
- Automatically stops after measurement completion

## SDMMC Interface Conflicts

Important note regarding SDMMC interface conflicts:
- Timer interrupts (any timer) interfere with SDMMC communication. I used this this approach: STM32 SDMMC (4-Bit Mode) FatFS Example; see here:
https://deepbluembedded.com/stm32-sdmmc-tutorial-examples-dma/#stm32-sdmmc-4bit-mode-fatfs-example-project
- Tested with different timers (TIM2, TIM3, TIM5, TIM6) - all showed conflicts
- So do make sure that you do not fire any timer interrupts during SDMMC1 (that is what I tested) operation. However, I did not test which specific (SDMMC / FatFs) functions are sensitive to timer interrupts. The SD-Card would not mount was where the error occured in my case. This library uses the respective timer interrupt starting with DHT22_Start_Reading and switches off the interrupts in the ISR if the data is deemed to be complete.

## Usage Example

```c
int main(void)
{
    uint16_t temperature, humidity;
    
    DHT22_Init();
    
    while (1)
    {
        DHT22_Start_Reading();
        HAL_Delay(50);  // Wait for measurement
        
        if(DHT22_Get_Data(&temperature, &humidity) == DHT22_OK)
        {
            // Temperature = temperature/10 °C
            // Humidity = humidity/10 %
        }
        
        HAL_Delay(2000);  // Minimum 2 seconds between measurements
    }
}
```

## Important Notes

1. Timer generates precise 10µs interrupts during measurement
2. Minimum 2 seconds between measurements required
3. Values need to be divided by 10 for °C and %
4. Pull-up resistor (4.7kΩ) required on data pin

## Functions

- `DHT22_Init()`: Initializes GPIO and Timer
- `DHT22_Start_Reading()`: Starts new measurement
- `DHT22_Get_Data()`: Reads measurement values

## Return Values

```c
typedef enum {
    DHT22_OK = 0,
    DHT22_NO_DATA = 1,
    DHT22_CHECKSUM_ERR = 2,
    DHT22_TIMEOUT = 3
} DHT22_Status;
```

## Debug Features
PD4 can be used for signal analysis:
- Shows DHT22 signal edge changes
- Indicates bit detection (1 pulse for '0', 2 pulses for '1')
- Helps with timing analysis

## Known Issues
- Timer interrupts conflict with SDMMC interface
- In my circuit I found signal spikes during reading the data which made it difficult to use signal flank interrupts to detect the bits. ToDo: Robustify that solution.
