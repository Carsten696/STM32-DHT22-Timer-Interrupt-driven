Thanks to Claude 3.5 Sonnet for the (somwhat unhelpful) help.

# DHT22 Bibliothek für STM32H7 - Installationsanleitung

## Hardware-Voraussetzungen
- STM32H7 Mikrocontroller (z.B. NUCLEO-H753ZI)
- DHT22 Temperatursensor
- Verbindungen:
  - DHT22 Datenpin → PD3
  - Optional: Debug-Pin → PD4

## Software-Voraussetzungen
- STM32CubeIDE
- HAL Bibliothek für STM32H7

## CubeMX Konfiguration

1. **Timer3 Konfiguration:**
   ```
   Mode: Internal Clock
   Prescaler: 127
   Counter Period: 4
   ```
   Im NVIC: Timer3 global interrupt aktivieren

2. **GPIO Konfiguration:**
   - PD3: GPIO_Output (wird später durch Software umgeschaltet)
   - PD4: GPIO_Output (für Debug-Signale)

## Installation der Bibliothek

1. **Dateien kopieren:**
   - `DHT22.h` in den `Core/Inc` Ordner
   - `DHT22.c` in den `Core/Src` Ordner

2. **In main.c hinzufügen:**
   ```c
   /* Private variables */
   extern TIM_HandleTypeDef htim3;

   /* USER CODE BEGIN 2 */
   DHT22_Init();
   ```

3. **Beispiel für die Verwendung:**
   ```c
   int main(void)
   {
       /* MCU Configuration etc */
       uint16_t temperature, humidity;
       
       /* Initialize all configured peripherals */
       DHT22_Init();
       
       while (1)
       {
           DHT22_Start_Reading();
           HAL_Delay(50);  // Warten auf Messung
           
           if(DHT22_Get_Data(&temperature, &humidity) == DHT22_OK)
           {
               // Temperatur = temperature/10 °C
               // Luftfeuchtigkeit = humidity/10 %
           }
           
           HAL_Delay(2000);  // Mind. 2 Sekunden zwischen Messungen
       }
   }
   ```

## Wichtige Hinweise

1. Der Timer muss genau 10µs Interrupts erzeugen
2. Zwischen den Messungen mindestens 2 Sekunden warten
3. Die Werte müssen durch 10 geteilt werden für °C bzw. %
4. Pull-up Widerstand (4.7kΩ) am Datenpin erforderlich

## Funktionen

- `DHT22_Init()`: Initialisiert GPIO und Timer
- `DHT22_Start_Reading()`: Startet eine neue Messung
- `DHT22_Get_Data()`: Liest die Messwerte aus

## Rückgabewerte

```c
typedef enum {
    DHT22_OK = 0,
    DHT22_NO_DATA = 1,
    DHT22_CHECKSUM_ERR = 2,
    DHT22_TIMEOUT = 3
} DHT22_Status;
```

## Debug-Möglichkeiten
PD4 kann zur Signalanalyse verwendet werden. Im Code sind Debug-Ausgaben vorgesehen (aktuell auskommentiert).
