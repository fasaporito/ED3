/*



 */
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
#define BUFFER_SIZE 2 // 2 valores del ADC
uint16_t dmaBuffer[BUFFER_SIZE];  // Buffer DMA para almacenar los datos de ADC
#define CANAL_ADC_BATERIA 0 // Canal ADC para el nivel de batería
#define CANAL_ADC_TEMPERATURA 1 // Canal ADC para el LM35
#define UART_BAUD_RATE 9600 // Tasa de baud para comunicación UART

//void configurarPWM(void);
//void establecerAnguloServo(uint8_t angulo);
//void configurarBoton(void);
//void retardo_ms(uint32_t ms);
//void configurarADC(void);
//void configurarUART(void);
//void configurarTemporizador(void);
//void configurarDMA(void);

void configurarPWM() {
    // Configuración del pin P2.0 como PWM1.1
    PINSEL_CFG_Type PinConfig;
    PinConfig.Funcnum = 1;       // Función PWM
    PinConfig.OpenDrain = PINSEL_PINMODE_NORMAL;
    PinConfig.Pinmode = PINSEL_PINMODE_PULLUP;
    PinConfig.Portnum = 2;       // Puerto 2
    PinConfig.Pinnum = 0;        // Pin 0
    PINSEL_ConfigPin(&PinConfig);

    // Inicializar PWM
    PWM_TIMERCFG_Type PWMConfig;
    PWMConfig.PrescaleOption = PWM_TIMER_PRESCALE_USVAL; // Escala en microsegundos
    PWMConfig.PrescaleValue = 1;                         // 1 µs

    PWM_Init(LPC_PWM1, PWM_MODE_TIMER, &PWMConfig);

    // Configurar el período
    uint32_t periodMatch = 20000; // Período en microsegundos (20 ms para un servo de 50Hz)
    PWM_MatchUpdate(LPC_PWM1, 0, periodMatch, PWM_MATCH_UPDATE_NOW);

    // Configurar el ancho de pulso para PWM1.1
    uint32_t pulseMatch = 1500;  // Pulso en microsegundos (1.5 ms para la velocidad de apertura)
    PWM_MatchUpdate(LPC_PWM1, 1, pulseMatch, PWM_MATCH_UPDATE_NOW);

    // Configurar acciones de coincidencia
    PWM_MATCHCFG_Type MatchConfig;
    MatchConfig.MatchChannel = 0;  // Selecciona MR0
    MatchConfig.IntOnMatch = DISABLE;
    MatchConfig.ResetOnMatch = ENABLE; // Reiniciar el contador en MR0
    MatchConfig.StopOnMatch = DISABLE;

    PWM_ConfigMatch(LPC_PWM1, &MatchConfig); // Configurar MR0
    MatchConfig.ResetOnMatch = DISABLE;     // No reiniciar para MR1
    PWM_ConfigMatch(LPC_PWM1, &MatchConfig); // Configurar MR1

    // Habilitar PWM1.1
    PWM_ChannelCmd(LPC_PWM1, 1, ENABLE);

    // Habilitar el PWM y el contador
    PWM_ResetCounter(LPC_PWM1);
    PWM_CounterCmd(LPC_PWM1, ENABLE);
    PWM_Cmd(LPC_PWM1, ENABLE);
}

void establecerAnguloServo(uint8_t angulo) {
    // Convertir el ángulo en un ancho de pulso y actualizar el PWM
    uint32_t anchoPulso = 1000 + (angulo * 1000) / 180; // Convertir ángulo a ancho de pulso
    PWM_MatchUpdate(LPC_PWM1, 1, anchoPulso, PWM_MATCH_UPDATE_NOW);
}

void configurarADC() {
    // Configuración del ADC para el nivel de batería y el sensor LM35
    PINSEL_CFG_Type pinConfig;

    // P0.23 como entrada ADC para el nivel de batería
    pinConfig.Portnum = PINSEL_PORT_0;
    pinConfig.Pinnum = PINSEL_PIN_23;
    pinConfig.Funcnum = PINSEL_FUNC_1;  // Función ADC
    PINSEL_ConfigPin(&pinConfig);

    // P0.24 como entrada ADC para el sensor de temperatura LM35
    pinConfig.Pinnum = PINSEL_PIN_24;
    PINSEL_ConfigPin(&pinConfig);

    // Inicializa el ADC con frecuencia de 200 kHz
    ADC_Init(LPC_ADC, 200000);

    // Habilitar los canales correspondientes para los sensores
    ADC_ChannelCmd(LPC_ADC, 0, ENABLE);  // Canal 0 para la batería (P0.23)
    ADC_ChannelCmd(LPC_ADC, 1, ENABLE);  // Canal 1 para la temperatura (P0.24)

    // Habilitar la interrupción de la conversión completa del ADC
    ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, ENABLE);
    ADC_IntConfig(LPC_ADC, ADC_ADINTEN1, ENABLE);

    // Habilitar la interrupción del ADC en el NVIC
    NVIC_EnableIRQ(ADC_IRQn);
}

void configurarUART() {
    // Configuración de UART para transmisión de datos
    PINSEL_CFG_Type pinConfig;
    pinConfig.Portnum = PINSEL_PORT_0;
    pinConfig.Pinnum = PINSEL_PIN_0;
    pinConfig.Funcnum = PINSEL_FUNC_2;
    PINSEL_ConfigPin(&pinConfig);

    UART_CFG_Type UARTConfig;
    UARTConfig.Baud_rate = UART_BAUD_RATE;
    UARTConfig.Parity = UART_PARITY_NONE;
    UARTConfig.Databits = UART_DATABIT_8;
    UARTConfig.Stopbits = UART_STOPBIT_1;
    UART_Init(LPC_UART3, &UARTConfig);
    UART_TxCmd(LPC_UART3, ENABLE);
}

void configurarTemporizador() {
    // Configuración del temporizador para interrupción cada 30 segundos
    TIM_TIMERCFG_Type configuracionTemporizador;
    configuracionTemporizador.PrescaleOption = TIM_PRESCALE_USVAL;
    configuracionTemporizador.PrescaleValue = 1000; // 1ms

    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &configuracionTemporizador);
    TIM_MATCHCFG_Type configuracionMatch;
    configuracionMatch.MatchChannel = 0;
    configuracionMatch.IntOnMatch = ENABLE;
    configuracionMatch.ResetOnMatch = ENABLE;
    configuracionMatch.StopOnMatch = DISABLE;
    configuracionMatch.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    configuracionMatch.MatchValue = 30000; // 30 segundos

    TIM_ConfigMatch(LPC_TIM0, &configuracionMatch);
    NVIC_EnableIRQ(TIMER0_IRQn); // Habilita la interrupción del temporizador
    TIM_Cmd(LPC_TIM0, ENABLE); // Activa el temporizador
}

void configurarBoton() {

    // Configuración del pin P2.10 como eint
    PINSEL_CFG_Type PinConfig;
    PinConfig.Funcnum = 1;       // Función eint
    PinConfig.OpenDrain = PINSEL_PINMODE_NORMAL;
    PinConfig.Pinmode = PINSEL_PINMODE_PULLUP;
    PinConfig.Portnum = 2;       // Puerto 2
    PinConfig.Pinnum = 10;        // Pin 10
    PINSEL_ConfigPin(&PinConfig);

    GPIO_SetDir(PUERTO_BOTON, (1 << PIN_BOTON), 0);
    GPIO_IntCmd(PUERTO_BOTON, (1 << PIN_BOTON), 0);
    GPIO_ClearInt(PUERTO_BOTON, (1 << PIN_BOTON));
    NVIC_EnableIRQ(EINT3_IRQn);
}

void configurarDMA() {
    // Inicialización del DMA
    GPDMA_Init();

    // Configurar la fuente y destino del DMA
    // Canal de DMA 0
    GPDMA_Channel_CFG_Type dmaCfg;
    dmaCfg.ChannelNum = 0;  // Usamos el canal 0 del DMA
    dmaCfg.TransferSize = 1;  // Transfiere 8 bits
    dmaCfg.TransferType = GPDMA_TRANSFERTYPE_P2P; //Perif a perif
    dmaCfg.TransferWidth = GPDMA_WIDTH_WORD; // Configurar como palabra de 32 bits
    dmaCfg.SrcConn = GPDMA_CONN_ADC; // Fuente del periférico (ADC)
    dmaCfg.DstConn = GPDMA_CONN_UART1_Tx; // Destino UART

    // Configuración del canal DMA para la transferencia de ADC a UART
    GPDMA_Setup(&dmaCfg);

    NVIC_EnableIRQ(DMA_IRQn);         // Habilitar la interrupción de DMA
}

void TIMER0_IRQHandler(void) {
    // Inicia una conversión de ADC y habilita el DMA
    ADC_StartCmd(LPC_ADC, ADC_START_NOW);
    // Limpia la interrupción del temporizador
    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
}

void EINT3_IRQHandler(void) {
    if (GPIO_GetIntStatus(PUERTO_BOTON, PIN_BOTON, 0) != 0) {
        static uint8_t anguloActual = 0;
        static int direccion = 1;

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
        GPIO_ClearInt(PUERTO_BOTON, (1 << PIN_BOTON));
    }
}

void ADC_IRQHandler(void) {
    // Verificar cuál canal generó la interrupción
    if (ADC_ChannelGetStatus(LPC_ADC, CANAL_ADC_BATERIA, ADC_DATA_DONE)) {
        // Lee el valor convertido del canal de batería
        uint16_t valorBateria = ADC_ChannelGetData(LPC_ADC, CANAL_ADC_BATERIA);
        dmaBuffer[0] = valorBateria; // Guarda en el buffer DMA
    }
    if (ADC_ChannelGetStatus(LPC_ADC, CANAL_ADC_TEMPERATURA, ADC_DATA_DONE)) {
        // Lee el valor convertido del canal de temperatura
        uint16_t valorTemperatura = ADC_ChannelGetData(LPC_ADC, CANAL_ADC_TEMPERATURA);
        dmaBuffer[1] = valorTemperatura; // Guarda en el buffer DMA
    }
}

int main(void) {
    configurarPWM();
    configurarBoton();
    configurarADC();
    configurarUART();
    configurarTemporizador();
    configurarDMA();
    while (1) {
        __WFI(); // Esperar interrupción
    }
}
