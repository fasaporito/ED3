#include "lpc17xx_pinsel.h"
#include "lpc17xx_pwm.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_uart.h"

#define PUERTO_BOTON 2
#define PIN_BOTON 10
#define ANGULO_MAXIMO 180
#define PASO_ANGULO 1 // Grados a aumentar o disminuir por iteración
#define RETARDO_MS 20 // Retardo entre cambios de ángulo

#define CANAL_ADC_BATERIA 0 // Canal ADC para el nivel de batería
#define CANAL_ADC_TEMPERATURA 1 // Canal ADC para el LM35
#define UART_BAUD_RATE 9600 // Tasa de baud para comunicación UART

void configurarPWM(void);
void establecerAnguloServo(uint8_t angulo);
void configurarBoton(void);
void retardo_ms(uint32_t ms);
void configurarADC(void);
void configurarUART(void);
void configurarTemporizador(void);
void configurarDMA(void);

void TIMER0_IRQHandler(void) {
    // Lee el nivel de batería y la temperatura
    uint16_t nivelBateria = ADC_ChannelGetData(LPC_ADC, CANAL_ADC_BATERIA);
    uint16_t nivelTemperatura = ADC_ChannelGetData(LPC_ADC, CANAL_ADC_TEMPERATURA);

    // Transmite los datos vía UART
    UART_Send(LPC_UART3, (uint8_t*)&nivelBateria, sizeof(nivelBateria), BLOCKING);
    UART_Send(LPC_UART3, (uint8_t*)&nivelTemperatura, sizeof(nivelTemperatura), BLOCKING);

    // Limpia la interrupción del temporizador
    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
}

void configurarPWM(void) {
    // Configuración del pin PWM para el servo

    PINSEL_CFG_Type pinConfig;
    pinConfig.Portnum = PINSEL_PORT_2;  // Puerto 2
    pinConfig.Pinnum = PINSEL_PIN_0;   // Pin 2.0
    pinConfig.Funcnum = PINSEL_FUNC_1; // Configura P2.0 como PWM1.1
    pinConfig.Pinmode = PINSEL_PINMODE_PULLUP; // Configuración por defecto
    pinConfig.OpenDrain = PINSEL_PINMODE_NORMAL; // Configuración por defecto
    PINSEL_ConfigPin(&pinConfig);

    // Inicializar el PWM
    PWM_Init(LPC_PWM1, PWM_MODE_TIMER, NULL); // No se pasa estructura de configuración

    // Configurar el período del PWM (20 ms = 50 Hz)
    PWM_MatchUpdate(LPC_PWM1, 0, 20000, PWM_MATCH_UPDATE_NOW); // Período en MR0
    PWM_ConfigMatch(LPC_PWM1, &(PWM_MATCHCFG_Type){
        .MatchChannel = 0,
        .IntOnMatch = DISABLE,
        .ResetOnMatch = ENABLE,  // Reiniciar el contador en coincidencia con MR0
        .StopOnMatch = DISABLE   // Continuar contando
    });

    // Configurar el canal PWM1.1 para el servo
    PWM_ChannelConfig(LPC_PWM1, 1, PWM_CHANNEL_SINGLE_EDGE); // Canal 1, modo de un solo flanco
    PWM_MatchUpdate(LPC_PWM1, 1, 1500, PWM_MATCH_UPDATE_NOW);  // Pulso inicial (1.5 ms)
    PWM_ConfigMatch(LPC_PWM1, &(PWM_MATCHCFG_Type){
        .MatchChannel = 1,
        .IntOnMatch = DISABLE,
        .ResetOnMatch = DISABLE,
        .StopOnMatch = DISABLE
    });

    // Habilitar el canal PWM1.1
    PWM_ChannelCmd(LPC_PWM1, 1, ENABLE);

    // Iniciar el contador del PWM
    PWM_CounterCmd(LPC_PWM1, ENABLE);

    // Habilitar el PWM
    PWM_Cmd(LPC_PWM1, ENABLE);
}

void establecerAnguloServo(uint8_t angulo) {
    // Convertir el ángulo en un ancho de pulso y actualizar el PWM
    uint32_t anchoPulso = 1000 + (angulo * 1000) / 180; // Convertir ángulo a ancho de pulso
    PWM_MatchUpdate(LPC_PWM1, 1, anchoPulso, PWM_MATCH_UPDATE_NOW);
}

void configurarADC(void) {
    // Configuración del ADC para el nivel de batería y el sensor LM35
    PINSEL_CFG_Type pinConfig;
    pinConfig.Portnum = PINSEL_PORT_0;
    pinConfig.Pinnum = PINSEL_PIN_23; // P0.23 como entrada ADC para nivel de batería
    pinConfig.Funcnum = PINSEL_FUNC_1;
    PINSEL_ConfigPin(&pinConfig);

    pinConfig.Pinnum = PINSEL_PIN_24; // P0.24 como entrada ADC para temperatura
    PINSEL_ConfigPin(&pinConfig);

    ADC_Init(LPC_ADC, 200000); // Inicializa el ADC con frecuencia de 200 kHz
    ADC_ChannelCmd(LPC_ADC, CANAL_ADC_BATERIA, ENABLE);
    ADC_ChannelCmd(LPC_ADC, CANAL_ADC_TEMPERATURA, ENABLE);
}

void configurarUART(void) {
    // Configuración de UART para transmisión de datos
    PINSEL_CFG_Type pinConfig;
    pinConfig.Portnum = PINSEL_PORT_0;
    pinConfig.Pinnum = PINSEL_PIN_2;
    pinConfig.Funcnum = PINSEL_FUNC_1;
    PINSEL_ConfigPin(&pinConfig);

    pinConfig.Pinnum = PINSEL_PIN_3;
    PINSEL_ConfigPin(&pinConfig);

    UART_CFG_Type UARTConfigStruct;
    UARTConfigStruct.Baud_rate = UART_BAUD_RATE;
    UARTConfigStruct.Parity = UART_PARITY_NONE;
    UARTConfigStruct.Databits = UART_DATABIT_8;
    UARTConfigStruct.Stopbits = UART_STOPBIT_1;
    UART_Init(LPC_UART3, &UARTConfigStruct);
    UART_TxCmd(LPC_UART3, ENABLE);
}

void configurarTemporizador(void) {
    // Configuración del temporizador para interrupción cada 30 segundos
    TIM_TIMERCFG_Type configuracionTemporizador;
    configuracionTemporizador.PrescaleOption = TIM_PRESCALE_USVAL;
    configuracionTemporizador.PrescaleValue = 1000000; // 1 segundo por tick

    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &configuracionTemporizador);
    TIM_MATCHCFG_Type configuracionMatch;
    configuracionMatch.MatchChannel = 0;
    configuracionMatch.IntOnMatch = ENABLE;
    configuracionMatch.ResetOnMatch = ENABLE;
    configuracionMatch.StopOnMatch = DISABLE;
    configuracionMatch.MatchValue = 30; // 30 segundos

    TIM_ConfigMatch(LPC_TIM0, &configuracionMatch);
    NVIC_EnableIRQ(TIMER0_IRQn); // Habilita la interrupción del temporizador
    TIM_Cmd(LPC_TIM0, ENABLE); // Activa el temporizador
}

void configurarDMA(void) {
    // Configuración básica del DMA
    GPDMA_Channel_CFG_Type DMAConfig;
    DMAConfig.ChannelNum = 0;
    DMAConfig.SrcMemAddr = (uint32_t)&LPC_ADC->ADGDR;
    DMAConfig.DstMemAddr = (uint32_t)&LPC_UART3->THR;
    DMAConfig.TransferSize = 2;
    DMAConfig.TransferWidth = GPDMA_WIDTH_WORD;
    DMAConfig.TransferType = GPDMA_TRANSFERTYPE_P2M;
    GPDMA_Setup(&DMAConfig);
}

void configurarBoton(void) {
    // Configura el pin como entrada
    GPIO_SetDir(PUERTO_BOTON, (1 << PIN_BOTON), 0); // Cambié GPIO_INPUT por GPIO_MODE_INPUT

    // Configura la interrupción en flanco ascendente (cuando el botón es presionado)
    GPIO_IntCmd(PUERTO_BOTON, (1 << PIN_BOTON), ENABLE);  // Configura la interrupción en el pin
    GPIO_ClearInt(PUERTO_BOTON, (1 << PIN_BOTON));        // Limpia la interrupción

    // Habilita la interrupción del GPIO
    NVIC_EnableIRQ(EINT3_IRQn); // En LPC1769, la interrupción para los pines GPIO se maneja con EINT3
}

void EINT3_IRQHandler(void) {
    // Verifica si la interrupción es por el pin configurado
    if (GPIO_GetIntStatus(PUERTO_BOTON, PIN_BOTON, 0) != 0) { // Cambié GPIO_INT_RISING a GPIO_INT_RISING_EDGE
        // Realiza la acción deseada, por ejemplo, cambiar el ángulo del servo
        static uint8_t anguloActual = 0;
        static int direccion = 1; // 1 para avanzar, -1 para retroceder

        if (anguloActual < ANGULO_MAXIMO && direccion == 1) {
            anguloActual += PASO_ANGULO;
            if (anguloActual >= ANGULO_MAXIMO) {
                anguloActual = ANGULO_MAXIMO;
            }
            establecerAnguloServo(anguloActual);
        } else if (anguloActual > 0 && direccion == -1) {
            anguloActual -= PASO_ANGULO;
            if (anguloActual <= 0) {
                anguloActual = 0;
            }
            establecerAnguloServo(anguloActual);
        }

        direccion = ((GPIO_ReadValue(PUERTO_BOTON) & (1 << PIN_BOTON)) == 0) ? 1 : -1;
        GPIO_ClearInt(PUERTO_BOTON, (1 << PIN_BOTON));  // Limpiar la bandera de interrupción
    }
}

int main(void) {
	configurarPWM();
    configurarBoton();
    configurarADC();
    configurarUART();
    configurarTemporizador();
    configurarDMA();

        // se maneja por interrupciones.
    while (1) {
        __WFI();  // Sleep mode para reducir el uso del procesador
    }
}
