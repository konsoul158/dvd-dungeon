/**
  ******************************************************************************
  * @file    main.c
  * @author  3S0 FreeRTOs
  * @version V1.0
  * @date    24/10/2017
  * @brief   FreeRTOS Example project.
  ******************************************************************************
*/

/*
 *
 * Trabalho Final 1180799 e 1180889
 * 2021 - 2022
 *
 */

/* Standard includes. */
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <lcd.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "lcd.h"
#include "queue.h"
#include "semphr.h"

#include "math.h"



/* Task priorities. */
//#define mainFLASH_TASK_PRIORITY	( tskIDLE_PRIORITY + 1)
#define mainLCD_TASK_PRIORITY   ( tskIDLE_PRIORITY + 1)
#define mainUSART_TASK_PRIORITY	( tskIDLE_PRIORITY + 1)
#define mainMUSIC_TASK_PRIORITY	( tskIDLE_PRIORITY + 1)

///* The rate at which the flash task toggles the LED. */
//#define mainFLASH_DELAY			( ( TickType_t ) 1000 / portTICK_RATE_MS )
///* The rate at which the temperature is read. */
//#define mainTEMP_DELAY			( ( TickType_t ) 100 / portTICK_RATE_MS )

/* Configure RCC clocks */
static void prvSetupRCC( void );

/* Configure GPIO. */
static void prvSetupGPIO( void );


// Config Timers
void Timer3_Interrupt_func( void ); // Music Metronome Timer
void Timer4_func( void );           // Buzzer Sound Timer
void Timer2_Interrupt_func( void );

uint16_t RGB_Convert(uint16_t r, uint16_t g, uint16_t b);
void Play_Note(uint8_t note_nr, uint8_t oct, uint8_t on_off);


// Interrupções dos botões
void External_Interrupt_func();

// Funções SPI
void SPI2_Init( void );
void CONFIG_SREG( void );
void SPI_Write(uint8_t adress, uint8_t send_data);
uint16_t SPI_Read(uint8_t address);
void Ler_Acelerometro( void );

// Sprites
void mouse(uint8_t x, uint8_t y, uint16_t color);
void dvd(uint8_t x, uint8_t y, uint16_t color);
void CD(uint8_t x, uint8_t y, uint16_t color);

void atualizar_mouse( void );
void atualizar_player( void );
void atualizar_paredes( void );

void Time_Clock( void );

void Menu( void );
void Clear_Screen( void );
void Back_to_menu( void );
void Mute( void );

void History_Text( void );

void Win_Lose( void );


void Play_Music(uint16_t(*music)[3], uint16_t tempo, uint16_t t_max);

typedef struct XY_obj_vals
{
	int16_t x;
	int16_t y;
} XY_obj_vals;

typedef struct XYcolor_CDs
{
	int16_t x;
	int16_t y;
	uint16_t cor;
} XYcolor_CDs;


void CDs_perdidos( XYcolor_CDs cd1, XYcolor_CDs cd2, XYcolor_CDs cd3 );



/* Simple LED toggle task. */
static void prvTarefaMusica( void *pvParameters );

/* LCD activity task. */
static void prvLcdTask( void *pvParameters );



/********** Useful functions **********/
///* USART2 configuration. */
//static void prvSetupUSART2( void );
//
///* USART2 send message. */
//static void prvSendMessageUSART2(char *message);

/***************************************/

//static void prvSetupEXTI1( void ); //ex2

/* Task 1 handle variable. */
TaskHandle_t HandleMusica;
TaskHandle_t HandleLcd;



/* Mutexes e Semáforos*/
SemaphoreHandle_t xDisplayMutex;
SemaphoreHandle_t xMusicMutex;
SemaphoreHandle_t xMusicOnOffMutex;
SemaphoreHandle_t xClockMutex;
SemaphoreHandle_t xEtapaDoJogo;
SemaphoreHandle_t xWinLose;


/* Queues */
QueueHandle_t xAcelQueue;
QueueHandle_t xPlayerDisplayPosQueue;
QueueHandle_t xPlayerMapPosQueue;


int main( void )
{
	/*Setup the hardware, RCC, GPIO, etc...*/
    prvSetupRCC();
    prvSetupGPIO();

    // Inicializar SPI e configurar IC
	SPI2_Init();
	CONFIG_SREG();

    External_Interrupt_func();

    Timer4_func(); //Timer da freq do Buzzer
    Timer3_Interrupt_func(); //Music Metronome

    Timer2_Interrupt_func(); // Timer do temporizador

    srand(xTaskGetTickCount());//time(NULL));

    // Default
    XY_obj_vals Default_XY;
    Default_XY.x = 30;
    Default_XY.y = 100;

    xDisplayMutex    = xSemaphoreCreateMutex();
    xMusicMutex      = xSemaphoreCreateBinary();
    xMusicOnOffMutex = xSemaphoreCreateBinary();
    xClockMutex      = xSemaphoreCreateBinary();
    xWinLose         = xSemaphoreCreateBinary();

    xEtapaDoJogo = xSemaphoreCreateCounting(5, 0); // 0 Menu, 1 Texto, 2 Settings, 3 Jogo, 4 Final

    // Queue Dos valores do aceleremetro
    xAcelQueue = xQueueCreate( 1, sizeof( XY_obj_vals ) ); //ex1
//	if( xAcelQueue == 0 ) {}
//	else {}

	// Queue Dos valores da posição do Player no Display
	xPlayerDisplayPosQueue = xQueueCreate( 1, sizeof( XY_obj_vals ) ); //ex1
//	if( xPlayerDisplayPosQueue == 0 ) {}
//	else {}
	// Fill with Default
	static BaseType_t pxHigherPriorityTaskWoken;
	xQueueSendToBack(xPlayerDisplayPosQueue, &Default_XY, &pxHigherPriorityTaskWoken);

	// Queue Dos valores da posição do Player no Mapa
	xPlayerMapPosQueue = xQueueCreate( 1, sizeof( XY_obj_vals ) ); //ex1
//	if( xPlayerMapPosQueue == 0 ) {}
//	else {}
	// Fill with Default
//	static BaseType_t pxHigherPriorityTaskWoken;
	xQueueSendToBack(xPlayerMapPosQueue, &Default_XY, &pxHigherPriorityTaskWoken);

	/* Create the tasks */
 	xTaskCreate( prvTarefaMusica, "TarefaMusica", configMINIMAL_STACK_SIZE+256, NULL, mainMUSIC_TASK_PRIORITY+1, &HandleMusica );
 	xTaskCreate( prvLcdTask, "Lcd", configMINIMAL_STACK_SIZE+128, NULL, mainLCD_TASK_PRIORITY, &HandleLcd );

	/* Start the scheduler. */
	vTaskStartScheduler();

	/* Will only get here if there was not enough heap space to create the idle task. */
	return 0;
}
/*-----------------------------------------------------------*/


extern uint8_t intro_theme = 0;
static void prvTarefaMusica( void *pvParameters )
{
	uint16_t tempo = 1; // Time counts from 1 to t_max
//	t_max = C_nr*32 = Nr de compassos * 32 fusas por compasso

	//Música em formato BMP (Buzzer Music Protocol) (Musica convertida em python de um ficheiro MIDI de uma partitura para uma matriz)

	// Música original do compositor Diogo Faria
	// DVD Dungeon Intro
	uint16_t intro_t_max = 64;
	uint16_t intro_music[50][3] = {{1,3,11},
							  {2,0,0},
							  {3,4,2},
							  {4,0,0},
							  {5,4,6},
							  {6,0,0},
							  {7,4,2},
							  {8,0,0},
							  {9,4,6},
							  {10,0,0},
							  {11,4,9},
							  {12,0,0},
							  {14,4,12},
							  {16,0,0},
							  {18,4,9},
							  {20,0,0},
							  {21,4,2},
							  {22,0,0},
							  {23,4,5},
							  {24,0,0},
							  {25,4,9},
							  {26,0,0},
							  {27,4,5},
							  {28,0,0},
							  {29,4,9},
							  {30,0,0},
							  {31,4,12},
							  {32,0,0},
							  {34,5,3},
							  {36,0,0},
							  {38,4,12},
							  {40,0,0},
							  {41,4,5},
							  {42,0,0},
							  {43,4,8},
							  {44,0,0},
							  {45,4,12},
							  {46,0,0},
							  {47,4,8},
							  {48,0,0},
							  {49,4,12},
							  {50,0,0},
							  {51,5,3},
							  {52,0,0},
							  {54,5,6},
							  {56,0,0},
							  {58,5,3},
							  {60,0,0},
							  {62,5,4},
							  {64,0,0},
	};


	// DVD Dungeon Theme
	uint16_t theme_t_max = 512;
	static uint16_t theme_music[154][3] = {{4,4,1},
//	uint16_t t_max = 512;
//	static uint16_t music[154][3] = {{4,4,1},
							  {12,0,0},
							  {14,4,8},
							  {16,0,0},
							  {18,4,6},
							  {20,0,0},
							  {22,4,4},
							  {24,0,0},
							  {26,4,1},
							  {28,0,0},
							  {30,4,4},
							  {36,0,0},
							  {40,4,1},
							  {56,0,0},
							  {60,3,11},
							  {64,0,0},
							  {68,4,1},
							  {76,0,0},
							  {78,4,8},
							  {80,0,0},
							  {82,4,6},
							  {84,0,0},
							  {86,4,4},
							  {88,0,0},
							  {90,4,1},
							  {92,0,0},
							  {94,4,4},
							  {100,0,0},
							  {102,4,1},
							  {108,0,0},
							  {110,4,11},
							  {112,0,0},
							  {114,4,10},
							  {116,0,0},
							  {118,4,1},
							  {120,0,0},
							  {122,4,10},
							  {124,0,0},
							  {126,4,9},
							  {128,0,0},
							  {130,4,8},
							  {134,0,0},
							  {136,4,1},
							  {140,0,0},
							  {142,4,8},
							  {144,0,0},
							  {146,4,6},
							  {148,0,0},
							  {150,4,4},
							  {152,0,0},
							  {154,4,1},
							  {156,0,0},
							  {158,4,4},
							  {164,0,0},
							  {168,4,1},
							  {184,0,0},
							  {188,3,11},
							  {192,0,0},
							  {194,4,2},
							  {196,0,0},
							  {198,4,6},
							  {200,0,0},
							  {202,4,9},
							  {204,0,0},
							  {206,5,1},
							  {208,0,0},
							  {210,4,12},
							  {212,0,0},
							  {214,4,9},
							  {216,0,0},
							  {218,4,6},
							  {220,0,0},
							  {222,4,9},
							  {228,0,0},
							  {232,4,8},
							  {256,0,0},
							  {260,4,1},
							  {268,0,0},
							  {270,4,8},
							  {272,0,0},
							  {274,4,6},
							  {276,0,0},
							  {278,4,4},
							  {280,0,0},
							  {282,4,1},
							  {284,0,0},
							  {286,4,4},
							  {292,0,0},
							  {296,4,1},
							  {312,0,0},
							  {316,3,11},
							  {320,0,0},
							  {324,4,1},
							  {332,0,0},
							  {334,4,8},
							  {336,0,0},
							  {338,4,6},
							  {340,0,0},
							  {342,4,4},
							  {344,0,0},
							  {346,4,1},
							  {348,0,0},
							  {350,4,4},
							  {356,0,0},
							  {358,4,1},
							  {364,0,0},
							  {366,4,11},
							  {368,0,0},
							  {370,4,10},
							  {372,0,0},
							  {374,4,1},
							  {376,0,0},
							  {378,4,10},
							  {380,0,0},
							  {382,4,9},
							  {384,0,0},
							  {386,4,8},
							  {390,0,0},
							  {392,4,1},
							  {396,0,0},
							  {398,4,8},
							  {400,0,0},
							  {402,4,6},
							  {404,0,0},
							  {406,4,4},
							  {408,0,0},
							  {410,4,1},
							  {412,0,0},
							  {414,4,4},
							  {420,0,0},
							  {424,4,1},
							  {440,0,0},
							  {444,3,11},
							  {448,0,0},
							  {450,4,6},
							  {452,0,0},
							  {453,4,7},
							  {454,0,0},
							  {455,4,6},
							  {456,0,0},
							  {458,4,2},
							  {460,0,0},
							  {462,3,11},
							  {464,0,0},
							  {466,4,9},
							  {468,0,0},
							  {470,3,11},
							  {472,0,0},
							  {474,4,6},
							  {476,0,0},
							  {478,4,9},
							  {484,0,0},
							  {488,4,8},
							  {512,0,0},
	};/*Formato:              {t, o, n}
    	 t = time stamp do fim da nota
    	 o = oitava (da 4 à 7)
    	 n = numero da nota (0=pause, 1=C, 2=C#, 3=D, ... , 12=B) */

	uint16_t t_max;
    for( ;; )
	{
    	xSemaphoreTake( xMusicMutex, ( TickType_t) portMAX_DELAY ); // Wait for Timer3 Metronome

    	if (intro_theme == 1) Play_Music(intro_music, tempo, intro_t_max); //Intro Music
    	if (intro_theme == 0) // Intro Music
    	{
    		tempo = 1;
    		Play_Music(intro_music, tempo, intro_t_max);
    		t_max = intro_t_max;
    		intro_theme = 1;
    	}
    	if (intro_theme == 2) // Theme Music
    	{
    		Play_Music(theme_music, tempo, theme_t_max);
    		t_max = theme_t_max;
    	}

		tempo++;
		if (tempo >= t_max)
		{
			tempo = 1;
			intro_theme = 2;
		}

	}
}

uint16_t current_note_indx = 0;
void Play_Music(uint16_t(*music)[3], uint16_t tempo, uint16_t t_max)
{
	if (tempo == 1) current_note_indx = 0;
	uint8_t OnOff = uxSemaphoreGetCount( xMusicOnOffMutex );


	if (OnOff)
	{
		uint16_t dur_until = music[current_note_indx][0];
		uint16_t oct       = music[current_note_indx][1];
		uint16_t note_nr   = music[current_note_indx][2];

		if (tempo > dur_until) //Mudar para a nota seguinte caso o tempo da anterior tenha acabado
		{
			current_note_indx++;
			oct      = music[current_note_indx][1];
			note_nr  = music[current_note_indx][2];
		}
		Play_Note(note_nr, oct+1, OnOff);//music_on_off);

		tempo++;

		if (tempo >= t_max)
		{
			current_note_indx = 0;
		}
	}
	else TIM_Cmd(TIM4, 0); // Turn Off
}


uint8_t note_nr;
extern uint8_t out_end_flag = 0;
void Play_Note(uint8_t note_nr, uint8_t oct, uint8_t on_off)
{
	int16_t Octv[12] = {4368, 4123, 3892, 3674, 3468, 3273, 3089, 2916, 2752, 2598, 2452, 2314};
	uint16_t ARR;
	uint8_t mult = 0;

	if(note_nr == 0) TIM_Cmd(TIM4, 0);
	else
	{
		TIM_Cmd(TIM4, on_off);

		// Convert Octave to right ARR multiplication value
		if (oct < 4) mult = 8;
		else if (oct > 7) mult = 1;
		else mult = pow(2, (7-oct));


		ARR = Octv[note_nr-1] * mult;
		//Definir a frequência da nota
		TIM_SetAutoreload(TIM4, ARR-1);
	}
}

/*-----------------------------------------------------------*/
/* Example task to present characteres in ther display. */
extern uint8_t history_time = 20;
extern int16_t last_y = 130;
static void prvLcdTask( void *pvParameters )
{
	lcd_init( );

	uint8_t x_off = 35;
	uint8_t y_off = 15;


	for( ;; )
	{

		Ler_Acelerometro();

		XYcolor_CDs cd1;
		XYcolor_CDs cd2;
		XYcolor_CDs cd3;


//		xSemaphoreTake( xDisplayMutex, ( TickType_t) portMAX_DELAY );
		if (uxSemaphoreGetCount( xEtapaDoJogo ) == 0) // Menu
		{
			if (uxSemaphoreGetCount( xDisplayMutex ) == 1)
			{
				xSemaphoreTake(xDisplayMutex, ( TickType_t) portMAX_DELAY );
				Clear_Screen();
			}

			if ( uxSemaphoreGetCount( xWinLose) != 0 )
			{
				xSemaphoreTake(xWinLose, ( TickType_t) portMAX_DELAY );
			}

//			xSemaphoreTake( xDisplayMutex, ( TickType_t) portMAX_DELAY );
			Menu();
			atualizar_mouse();
//			xSemaphoreGive(xDisplayMutex );

		}
		if (uxSemaphoreGetCount( xEtapaDoJogo ) == 1) // História
		{
			if (uxSemaphoreGetCount( xDisplayMutex ) == 1)
			{
				xSemaphoreTake(xDisplayMutex, ( TickType_t) portMAX_DELAY );
				Clear_Screen();
				history_time = 30;
				last_y = 130;
			}

			History_Text();

			lcd_draw_string(14, 150, (unsigned char*) "Press SW5 to skip", RGB_Convert(255, 80, 10), 1);
			// Print_Story();

		}

		if (uxSemaphoreGetCount( xEtapaDoJogo ) == 2) // Settings
		{
			if (uxSemaphoreGetCount( xDisplayMutex ) == 1)
			{
				xSemaphoreTake(xDisplayMutex, ( TickType_t) portMAX_DELAY );;
				Clear_Screen();
			}

			Back_to_menu();
			Mute();

			atualizar_mouse();

		}

		if (uxSemaphoreGetCount( xEtapaDoJogo ) == 3) // Jogo
		{
			if (uxSemaphoreGetCount( xDisplayMutex ) == 1)
			{
				xSemaphoreTake(xDisplayMutex, ( TickType_t) portMAX_DELAY );
				Clear_Screen();

				// Reset CDs locations

				cd1.x = rand()%(128*2-30-x_off)+20;
				cd1.y = rand()%(160*2-20-y_off)+10;
				cd1.cor = RGB_Convert(255, rand()%255, 0);

				cd2.x = rand()%(128*2-30-x_off)+20;
				cd2.y = rand()%(160*2-20-y_off)+10;
				cd2.cor = RGB_Convert(0, 255, rand()%255);

				cd3.x = rand()%(128*2-30-x_off)+20;
				cd3.y = rand()%(160*2-20-y_off)+10;
				cd3.cor = RGB_Convert(rand()%255, 0, 255);
			}

			CDs_perdidos(cd1, cd2, cd3);
			Time_Clock();
			atualizar_player();
			atualizar_paredes();
		}
		if (uxSemaphoreGetCount( xEtapaDoJogo ) == 4) // Fim
		{
			if (uxSemaphoreGetCount( xDisplayMutex ) == 1)
			{
				xSemaphoreTake(xDisplayMutex, ( TickType_t) portMAX_DELAY );
				Clear_Screen();
			}

			Win_Lose();

			if (uxSemaphoreGetCount( xClockMutex ) == 1)
			{
				xSemaphoreTake( xClockMutex, ( TickType_t) portMAX_DELAY );
				out_end_flag = 1;
			}

		}

        //vTaskDelay( ( TickType_t ) 50 / portTICK_RATE_MS);
	}
}
/*-----------------------------------------------------------*/



/*-----------------------------------------------------------*/
void Timer3_Interrupt_func( void ) // Music Metronome, bmps to Hz
{
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);


	// Seminima a "X" bpm -> X/60 (bps = Hz)
	// Resolução = 8 (1 seminima = 8 fusas)
	// Freq = 8 * X/60 = res * bps
	// Bpms possíveis: 60 67 75 82 90 97 105 112 120 127 135 142 150 157 165 172 180 187 195 202
	// Timer freqs:     8  9 10 11 12 13  14  15  16  17  18  19  20  21  22  23  24  25  26  27
	uint8_t bpms = 120;//60;//150;
	uint8_t freq_fusas = (uint8_t) (8*bpms/60);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	TIM_TimeBaseStructure.TIM_Period = 10000-1;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_Prescaler = (6400/freq_fusas)-1; //
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	TIM_Cmd(TIM3, ENABLE);


	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE );
}

void Timer4_func( void )// Freq for Buzzer
{
	// Buzzer pin
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// Prioridade
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);



	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period = 18512-1;// <- para 64MHz //(5846*4)-1; <- para 72MHz //auto-reload 0 at´e 65535
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_Prescaler = 7-1;
	// Com CK_INIT = 64MHz
	//64000000/((18511+1)*(6+1) = 493,888 Hz ~= 493.883 Hz
	// A = 439.899 ~= 440Hz (dif=0.0165, percent_err=0.0037%):
	// Opção 1: prescaler = 7-1;  Afinado em B4: 493.883Hz  [C4 - B7]
	// Opção 2: prescaler = 14-1; Afinado em B5: 246.94Hz   [C3 - B6]

	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

	// Saída para o PB9 -> Buzzer
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Toggle;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC4Init(TIM4, &TIM_OCInitStructure); //PB1
	TIM_Cmd(TIM4, DISABLE); //Ligar o buzzer apenas quando desejado

}


void External_Interrupt_func()
{
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);
	EXTI_InitTypeDef EXTI_InitStructure;
	EXTI_InitStructure.EXTI_Line = EXTI_Line1;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
}



void Timer2_Interrupt_func( void ) //1Hz
{
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	TIM_TimeBaseStructure.TIM_Period = 64000-1;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_Prescaler = 1000-1; // 1Hz
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	TIM_Cmd(TIM2, ENABLE);

	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE );
}


uint16_t RGB_Convert(uint16_t r, uint16_t g, uint16_t b)
{
	r = (uint16_t) r*(pow(2,5)-1)/255;
	g = (uint16_t) g*(pow(2,6)-1)/255;
	b = (uint16_t) b*(pow(2,5)-1)/255;
	return (uint16_t) ((r << 11) | (g << 5) | (b));
}



//static void prvUSART2Interrupt ( void )
//{
//	NVIC_InitTypeDef NVIC_InitStructure;
//
//	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE );
//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
//
//	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//
//	NVIC_Init(&NVIC_InitStructure);
//}


//static void prvSetupRCC( void ) //com HSE
//{
//    /* RCC configuration - 72 MHz */
//    ErrorStatus HSEStartUpStatus;
//
//    RCC_DeInit();
//    /*Enable the HSE*/
//    RCC_HSEConfig(RCC_HSE_ON);
//    /* Wait untill HSE is ready or time out */
//    HSEStartUpStatus = RCC_WaitForHSEStartUp();
//    if(HSEStartUpStatus == SUCCESS)
//    {
//        /* Enable The Prefetch Buffer */
//        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
//        /* 72 MHZ - 2 wait states */
//        FLASH_SetLatency(FLASH_Latency_2);
//
//        /* No division HCLK = SYSCLK */
//        RCC_HCLKConfig(RCC_SYSCLK_Div1);
//        /* PCLK1 = HCLK/2 (36MHz) */
//        RCC_PCLK1Config(RCC_HCLK_Div2);
//        /* PCLK2 = HCLK (72MHz)*/
//        RCC_PCLK2Config(RCC_HCLK_Div1);
//
//        /* Use PLL with HSE=12MHz */
//        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_6);
//        /* Enable the PLL */
//        RCC_PLLCmd(ENABLE);
//        /* Wait for PLL ready */
//        while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET );
//
//        /* Select the PLL as system clock source */
//        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
//        /* Wait until PLL is used as system clock */
//        while( RCC_GetSYSCLKSource() != 0x08 );
//    }
//    else
//    {
//        while(1);
//    }
//}
///*-----------------------------------------------------------*/


static void prvSetupRCC( void ) // RCC COM HSI
{

    RCC_DeInit();
    /*Enable the HSI*/
    RCC_HSICmd(ENABLE);
    /* Wait untill HSI is ready or time out */
    while(RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET){};

    //SET HSI AS SYSCLK SRC. CONFIGURE HCLK, PCLK1 & PCLK2
    RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div2);
    RCC_PCLK2Config(RCC_HCLK_Div1);

    //000 Zero wait state, if 0  MHz < SYSCLK <= 24 MHz
    //001 One wait state, if  24 MHz < SYSCLK <= 48 MHz
    //010 Two wait states, if 48 MHz < SYSCLK <= 72 MHz */
    FLASH_SetLatency(FLASH_Latency_2);

    //DISABLE PLL
    RCC_PLLCmd(DISABLE);

    //CHANGE PLL SRC AND MULTIPLIER
    RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_16);

    //ENABLE PLL
    //WAIT FOR IT TO BE READY
    //SET SYSCLK SRC AS PLLCLK
    RCC_PLLCmd(ENABLE);
    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET){};
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

    FLASH_SetLatency(FLASH_Latency_2);

    //SET HCLK = SYSCLK = 64MHZ
    RCC_HCLKConfig(RCC_SYSCLK_Div1);

    //SET PCLK1 = HCLK/2 = 32MHZ
    RCC_PCLK1Config(RCC_HCLK_Div2);

    //SET PCLK2 = HCLK = 64MHZ
    RCC_PCLK2Config(RCC_HCLK_Div1);

}
/*-----------------------------------------------------------*/


static void prvSetupGPIO( void )
{
    /* GPIO configuration - GREEN LED*/
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable GPIOB clock */
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB , ENABLE );

    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOB, &GPIO_InitStructure);


    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA , ENABLE ); //Botão A1 (SW5)

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_Init(GPIOA, &GPIO_InitStructure);

}
/*-----------------------------------------------------------*/



//void prvSetupUSART2( void )
//{
//USART_InitTypeDef USART_InitStructure;
//GPIO_InitTypeDef GPIO_InitStructure;
//
//    /* USART2 is configured as follow:
//        - BaudRate = 115200 baud
//        - Word Length = 8 Bits
//        - 1 Stop Bit
//        - No parity
//        - Hardware flow control disabled (RTS and CTS signals)
//        - Receive and transmit enabled */
//
//    /* Enable GPIOA clock */
//    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA , ENABLE );
//
//    /* USART Periph clock enable */
//    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
//
//    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_2;
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOA, &GPIO_InitStructure);
//
//    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_3;
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOA, &GPIO_InitStructure);
//
//
//    USART_InitStructure.USART_BaudRate = 115200;
//    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
//    USART_InitStructure.USART_StopBits = USART_StopBits_1;
//    USART_InitStructure.USART_Parity = USART_Parity_No;
//    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
//    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
//
//    /* Configure the USART2 */
//    USART_Init(USART2, &USART_InitStructure);
//    /* Enable the USART2 */
//    USART_Cmd(USART2, ENABLE);
// }
//
///*-----------------------------------------------------------*/



static void prvSendMessageUSART2(char *message)
{
uint16_t cont_aux=0;

    while(cont_aux != strlen(message))
    {
        USART_SendData(USART2, (uint8_t) message[cont_aux]);
        while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
        {
        }
        cont_aux++;
    }
}
/*-----------------------------------------------------------*/



// SPI Functions

void SPI2_Init()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;

    //PB13 CLK e PB15 MOSI (output)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15 ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//PB14 MISO (input)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);


	//PD2 CS
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_WriteBit(GPIOD, GPIO_Pin_2, 1); //Cs a 1


    // Step 1: Initialize SPI2
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

    // Initialization struct
    SPI_InitTypeDef SPI_InitStruct;
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStruct.SPI_CRCPolynomial = 7;
    SPI_Init(SPI2, &SPI_InitStruct);
    SPI_Cmd(SPI2, ENABLE);
//    GPIO_WriteBit(GPIOD, GPIO_Pin_2, 1); //Cs a 1
}


// Escreve a data na adress
void SPI_Write(uint8_t adress, uint8_t send_data)
{
	while ( SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData( SPI2, adress );
	SPI_I2S_ClearFlag(SPI2, SPI_I2S_FLAG_TXE);
	while ( SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
    SPI_I2S_ReceiveData( SPI2 );
    SPI_I2S_ClearFlag(SPI2, SPI_I2S_FLAG_RXNE);

	while ( SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData( SPI2, send_data );
	SPI_I2S_ClearFlag(SPI2, SPI_I2S_FLAG_TXE);
	while ( SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
	SPI_I2S_ReceiveData( SPI2 );
    SPI_I2S_ClearFlag(SPI2, SPI_I2S_FLAG_RXNE);
}

// Retorna a data que está na address
uint16_t SPI_Read(uint8_t address)
{
	uint16_t receive_data = 0;
	address |= 0b10000000;// Para ler o oitavo bit tem de ser 1

	GPIO_WriteBit(GPIOD, GPIO_Pin_2, 0); // Cs a 0

	while ( SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData( SPI2, address );
	SPI_I2S_ClearFlag(SPI2, SPI_I2S_FLAG_TXE);
	while ( SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
    SPI_I2S_ReceiveData( SPI2 );
    SPI_I2S_ClearFlag(SPI2, SPI_I2S_FLAG_RXNE);


	while ( SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData( SPI2, 0x00 );
	SPI_I2S_ClearFlag(SPI2, SPI_I2S_FLAG_TXE);
	while ( SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
    receive_data = SPI_I2S_ReceiveData( SPI2 );
    SPI_I2S_ClearFlag(SPI2, SPI_I2S_FLAG_RXNE);

    GPIO_WriteBit(GPIOD, GPIO_Pin_2, 1); // Cs a 1

	return receive_data; // Devolver a data recebida
}

void CONFIG_SREG()
{
	// SREG1
		GPIO_WriteBit(GPIOD, GPIO_Pin_2, 0); // colocar pino em baixo
		SPI_Write(0x20, 0b11000111);	// Endereço do registo CTRL_REG1
		// Sensor ativo - 11; Dividir por 8 - 11 (2560Hz); ST desligado - 0; Zen,Yen,Xen (eixos) ativos - 111;
		GPIO_WriteBit(GPIOD, GPIO_Pin_2, 1); // colocar pino alto

	//SREG2
		GPIO_WriteBit(GPIOD, GPIO_Pin_2, 0); // colocar pino em baixo
		SPI_Write(0x21, 0b01000000);	// Endereço do registo CTRL_REG2
		// +/-2g; BDU=1; BLE=0; ROOT=0; IEN=0; DRDY=0; SIM=0 (4 wire); DAS=0; (12 bit right shift)
		GPIO_WriteBit(GPIOD, GPIO_Pin_2, 1); // colocar pino alto

 	//SREG3
		GPIO_WriteBit(GPIOD, GPIO_Pin_2, 0); // colocar pino em baixo
		SPI_Write(0x21, 0b10000000);	// Endereço do registo CTRL_REG3
		GPIO_WriteBit(GPIOD, GPIO_Pin_2, 1); // colocar pino alto
}


void Ler_Acelerometro( void )
{

	uint8_t x_h = 0, x_l=0, y_h=0, y_l=0;//, z_h=0, z_l=0;
	int16_t x, y;//, z;

	x_l=SPI_Read(0x28);
	x_h=SPI_Read(0x29);

	y_l=SPI_Read(0x2A);
	y_h=SPI_Read(0x2B);

//	z_l=SPI_Read(0x2C);
//	z_h=SPI_Read(0x2D);

	x = (x_h<<8) | x_l;
	y = (y_h<<8) | y_l;
	//z = (z_h<<8) | z_l;


	XY_obj_vals Acel_vals;
	Acel_vals.x = x;
	Acel_vals.y = y;
	static BaseType_t pxHigherPriorityTaskWoken;
	xQueueReset(xAcelQueue);//Clear xAcelQueue
	xQueueSendToBack(xAcelQueue, &Acel_vals, &pxHigherPriorityTaskWoken);
}


// Sprites
void mouse(uint8_t x, uint8_t y, uint16_t color)
{
	lcd_draw_pixel(x, y, color);     lcd_draw_pixel(x, y+1, color);   lcd_draw_pixel(x+1, y+1, color);
	lcd_draw_pixel(x, y+2, color);   lcd_draw_pixel(x+1, y+2, color); lcd_draw_pixel(x+2, y+2, color);
	lcd_draw_pixel(x, y+3, color);   lcd_draw_pixel(x+1, y+3, color); lcd_draw_pixel(x+2, y+3, color);
	lcd_draw_pixel(x+3, y+3, color); lcd_draw_pixel(x, y+4, color);   lcd_draw_pixel(x+1, y+4, color);
	lcd_draw_pixel(x+2, y+4, color); lcd_draw_pixel(x+3, y+4, color); lcd_draw_pixel(x+4, y+4, color);
	lcd_draw_pixel(x, y+5, color);   lcd_draw_pixel(x+1, y+5, color); lcd_draw_pixel(x+2, y+5, color);
	lcd_draw_pixel(x+3, y+5, color); lcd_draw_pixel(x+4, y+5, color); lcd_draw_pixel(x+5, y+5, color);
	lcd_draw_pixel(x, y+6, color);   lcd_draw_pixel(x+1, y+6, color); lcd_draw_pixel(x+2, y+6, color);
	lcd_draw_pixel(x+3, y+6, color); lcd_draw_pixel(x+4, y+6, color); lcd_draw_pixel(x+5, y+6, color);
	lcd_draw_pixel(x+6, y+6, color); lcd_draw_pixel(x, y+7, color);   lcd_draw_pixel(x+1, y+7, color);
	lcd_draw_pixel(x+2, y+7, color); lcd_draw_pixel(x+3, y+7, color); lcd_draw_pixel(x, y+8, color);
	lcd_draw_pixel(x+1, y+8, color); lcd_draw_pixel(x+3, y+8, color); lcd_draw_pixel(x+4, y+8, color);
	lcd_draw_pixel(x, y+9, color);   lcd_draw_pixel(x+3, y+9, color); lcd_draw_pixel(x+4, y+9, color);
	lcd_draw_pixel(x+4, y+10, color);lcd_draw_pixel(x+5, y+10, color);																														  lcd_draw_pixel(x+4, y+11, color);lcd_draw_pixel(x+5, y+11, color);
}

void dvd(uint8_t x, uint8_t y, uint16_t color)
{
    int i = 0;
    for(i = 0; i < 30; i++){
        if(i == 13) i = 20;
        lcd_draw_pixel(x + i, y, color);
    }
    for(i = 0; i < 32; i++){
        if(i == 13) i = 20;
        lcd_draw_pixel(x + i, y+1, color);
    }
    for(i = 0; i < 33; i++){
        if(i == 14) i = 19;
        lcd_draw_pixel(x + i, y+2, color);
    }
    for(i = 6; i < 33; i++){
        if(i != 10){
            if(i == 14) i = 19;
            if(i == 22) i = 27;
            lcd_draw_pixel(x + i, y+3, color);
        }
    }
    for(i = 0; i < 34; i++){
        if(i == 15) i = 18;
        if(i == 3)  i = 7;
        if(i == 25) i = 29;
        lcd_draw_pixel(x + i, y+4, color);
    }
    for(i = 0; i < 34; i++){
        if(i != 11 || i != 21){
            if(i == 15) i = 18;
            if(i == 3)  i = 8;
            if(i == 25) i = 30;
            lcd_draw_pixel(x + i, y+5, color);
        }
    }
    for(i = 0; i < 34; i++){
        if(i != 11 || i != 21){
            if(i == 16) i = 17;
            if(i == 3)  i = 8;
            if(i == 25) i = 30;
            lcd_draw_pixel(x + i, y+6, color);
        }
    }
    for(i = 0; i < 34; i++){
        if(i != 16){
            if(i == 11) i = 13;
            if(i == 3)  i = 7;
            if(i == 20) i = 22;
            if(i == 25) i = 29;
            lcd_draw_pixel(x + i, y+7, color);
        }
    }
    for(i = 0; i < 33; i++){
        if(i == 10) i = 13;
        if(i == 3)  i = 6;
        if(i == 20) i = 22;
        if(i == 25) i = 28;
        lcd_draw_pixel(x + i, y+8, color);
    }
    for(i = 0; i < 33; i++){
        if(i == 10) i = 14;
        if(i == 19) i = 22;
        lcd_draw_pixel(x + i, y+9, color);
    }
    for(i = 0; i < 32; i++){
        if(i == 9)  i = 14;
        if(i == 19) i = 22;
        lcd_draw_pixel(x + i, y+10, color);
    }
    for(i = 0; i < 30; i++){
        if(i == 7)  i = 15;
        if(i == 18) i = 22;
        lcd_draw_pixel(x + i, y+11, color);
    }
    for(i = 15; i < 18; i++){
        lcd_draw_pixel(x + i, y+12, color);
    }

    lcd_draw_pixel(x + 16, y+13, color);

}

void CD(uint8_t x, uint8_t y, uint16_t color)
{
	int i = 0;
	for(i = 8; i < 26; i++){
			lcd_draw_pixel(x + i, y+14, color);
		}
		for(i = 3; i < 31; i++){
			lcd_draw_pixel(x + i, y+15, color);
		}
		for(i = 0; i < 34; i++){
			lcd_draw_pixel(x + i, y+16, color);
		}
		for(i = -1; i < 34; i++){
			if(i == 12)
				i = 21;
			lcd_draw_pixel(x + i, y+16, color);
		}
		for(i = -2; i < 36; i++){
			if(i == 10)
				i = 23;
			lcd_draw_pixel(x + i, y+17, color);
		}
		for(i = -1; i < 34; i++){
			if(i == 12)
				i = 21;
			lcd_draw_pixel(x + i, y+18, color);
		}
		for(i = 0; i < 34; i++){
			lcd_draw_pixel(x + i, y+19, color);
		}
		for(i = 3; i < 31; i++){
			lcd_draw_pixel(x + i, y+20, color);
		}
		for(i = 8; i < 26; i++){
			lcd_draw_pixel(x + i, y+21, color);
		}
}



// Atualizar Display
void atualizar_mouse( void )
{
	// Receive Acelerometer Values
	XY_obj_vals Acel_vals;
	if( xAcelQueue != 0 ) xQueuePeek( xAcelQueue, &Acel_vals, ( TickType_t ) portMAX_DELAY );
	int16_t acel_x = Acel_vals.x;
	int16_t acel_y = Acel_vals.y;


	// Receive Player Display Position Values
	XY_obj_vals Display_Pos_vals;
	if( xPlayerDisplayPosQueue != 0 ) xQueuePeek( xPlayerDisplayPosQueue, &Display_Pos_vals, ( TickType_t ) portMAX_DELAY );
	int16_t x_disp = Display_Pos_vals.x;
	int16_t y_disp = Display_Pos_vals.y;
	int16_t x_disp_lim;
	int16_t y_disp_lim;

	mouse(x_disp, y_disp, 0x0000); //clear mouse

	int8_t x_add = 0;
	int8_t y_add = 0;
	uint8_t limit = 20;
	uint8_t x_off = 7;
	uint8_t y_off = 12;
	if (acel_x >  limit)    x_add+=3;
	if (acel_x < -limit)    x_add-=3;
	if (acel_x >  limit*4)  x_add+=4;
	if (acel_x < -limit*4)  x_add-=4;
	if (acel_x >  limit*8)  x_add+=6;
	if (acel_x < -limit*8)  x_add-=6;

	if (acel_y >  limit)    y_add+=3;
	if (acel_y < -limit)    y_add-=3;
	if (acel_y >  limit*4)  y_add+=4;
	if (acel_y < -limit*4)  y_add-=4;
	if (acel_y >  limit*8)  y_add+=6;
	if (acel_y < -limit*8)  y_add-=6;

	// Display Mouse Logic
	x_disp += x_add;
	y_disp += y_add;
	x_disp_lim = x_disp;
	y_disp_lim = y_disp;

	if (x_disp < 0)      x_disp_lim = 0;
	if (x_disp > 128-x_off)  x_disp_lim = 128-x_off;
	if (y_disp < 0)      y_disp_lim = 0;
	if (y_disp > 160-y_off) y_disp_lim = 160-y_off;

	mouse(x_disp_lim, y_disp_lim, 0xFFFF); //print mouse

	Display_Pos_vals.x = x_disp_lim;
	Display_Pos_vals.y = y_disp_lim;

	static BaseType_t pxHigherPriorityTaskWoken;
	xQueueReset(xPlayerDisplayPosQueue);//Clear xPlayerDisplayPosQueue
	xQueueSendToBack(xPlayerDisplayPosQueue, &Display_Pos_vals, &pxHigherPriorityTaskWoken);


}

void atualizar_player( void )
{
	// Receive Acelerometer Values
	XY_obj_vals Acel_vals;
	if( xAcelQueue != 0 ) xQueuePeek( xAcelQueue, &Acel_vals, ( TickType_t ) portMAX_DELAY );
	int16_t acel_x = Acel_vals.x;
	int16_t acel_y = Acel_vals.y;


	// Receive Player Display Position Values
	XY_obj_vals Display_Pos_vals;
	if( xPlayerDisplayPosQueue != 0 ) xQueuePeek( xPlayerDisplayPosQueue, &Display_Pos_vals, ( TickType_t ) portMAX_DELAY );
	int16_t x_disp = Display_Pos_vals.x;
	int16_t y_disp = Display_Pos_vals.y;
	int16_t x_disp_lim;
	int16_t y_disp_lim;


	// Receive Player Map Position Values
	XY_obj_vals Map_Pos_vals;
	if( xPlayerMapPosQueue != 0 ) xQueuePeek( xPlayerMapPosQueue, &Map_Pos_vals, ( TickType_t ) portMAX_DELAY );
	int16_t x_map = Map_Pos_vals.x;
	int16_t y_map = Map_Pos_vals.y;
	int16_t x_map_lim;
	int16_t y_map_lim;


	dvd(x_disp, y_disp, 0x0000); //clear dvd

	int8_t x_add = 0;
	int8_t y_add = 0;
	uint8_t limit = 20;
	uint8_t x_off = 34;
	uint8_t y_off = 14;
	uint8_t wall_size = 10;
	if (acel_x >  limit)    x_add+=3;
	if (acel_x < -limit)    x_add-=3;
	if (acel_x >  limit*4)  x_add+=4;
	if (acel_x < -limit*4)  x_add-=4;
	if (acel_x >  limit*8)  x_add+=6;
	if (acel_x < -limit*8)  x_add-=6;

	if (acel_y >  limit)    y_add+=3;
	if (acel_y < -limit)    y_add-=3;
	if (acel_y >  limit*4)  y_add+=4;
	if (acel_y < -limit*4)  y_add-=4;
	if (acel_y >  limit*8)  y_add+=6;
	if (acel_y < -limit*8)  y_add-=6;

	// Display Player Logic
	x_disp += x_add;
	y_disp += y_add;
	x_disp_lim = x_disp;
	y_disp_lim = y_disp;

	if (x_disp < 0+wall_size) x_disp_lim = 0+wall_size;
	if (x_disp > 128-x_off-wall_size) x_disp_lim = 128-x_off-wall_size;
	if (y_disp < 0+wall_size) y_disp_lim = 0+wall_size;
	if (y_disp > 160-y_off-wall_size) y_disp_lim = 160-y_off-wall_size;


	// Map Logic
	x_map += x_add;
	y_map += y_add;
	x_map_lim = x_map;
	y_map_lim = y_map;

	if (x_map < 0+wall_size) x_map_lim = 0+wall_size;
	if (x_map > 128*2-x_off-wall_size) x_map_lim = 128*2-x_off-wall_size;
	if (y_map < 0+wall_size) y_map_lim = 0+wall_size;
	if (y_map > 160*2-y_off-wall_size) y_map_lim = 160*2-y_off-wall_size;

	Map_Pos_vals.x = x_map_lim;
	Map_Pos_vals.y = y_map_lim;


	// Center Player
	uint8_t margem_x = 128/2;
	uint8_t margem_y = 160/2;
	uint16_t map_x_size = 128*2;
	uint16_t map_y_size = 160*2;
	if (x_map_lim > margem_x- x_off/2 && x_map_lim < map_x_size-margem_x- x_off/2)  x_disp_lim = margem_x - x_off/2;
	if (y_map_lim > margem_y- y_off/2 && y_map_lim < map_y_size-margem_y- y_off/2)  y_disp_lim = margem_y - y_off/2;



	Display_Pos_vals.x = x_disp_lim;
	Display_Pos_vals.y = y_disp_lim;

	dvd(x_disp_lim, y_disp_lim, 0xFFFF); //print dvd

	static BaseType_t pxHigherPriorityTaskWoken;
	xQueueReset(xPlayerDisplayPosQueue);//Clear xPlayerDisplayPosQueue
	xQueueSendToBack(xPlayerDisplayPosQueue, &Display_Pos_vals, &pxHigherPriorityTaskWoken);


//	static BaseType_t pxHigherPriorityTaskWoken;
	xQueueReset(xPlayerMapPosQueue);//Clear xPlayerDisplayPosQueue
	xQueueSendToBack(xPlayerMapPosQueue, &Map_Pos_vals, &pxHigherPriorityTaskWoken);

//	char msg[20];
//	memset(msg, 0, sizeof(msg));
//	sprintf(msg, "disp: %d %d  ", x_disp_lim, y_disp_lim);
//	lcd_draw_string(30, 10, msg, 0xFFFF, 1);
//
//	memset(msg, 0, sizeof(msg));
//	sprintf(msg, "map:  %d %d   ", x_map_lim, y_map_lim);
//	lcd_draw_string(30, 20, msg, 0xFFFF, 1);

}

uint8_t x_last_thick = 0;
uint8_t y_last_thick = 0;
void atualizar_paredes( void )
{
	uint8_t max_wall_thickness = 10;
	uint16_t wall_color = RGB_Convert(50, 50, 80);

	uint8_t x_off = 34;
	uint8_t y_off = 14;

	int16_t test = 0;

	XY_obj_vals Map_Display_vals;
	if( xPlayerDisplayPosQueue != 0 ) xQueuePeek( xPlayerDisplayPosQueue, &Map_Display_vals, ( TickType_t ) portMAX_DELAY );
	int16_t x_disp = Map_Display_vals.x;
	int16_t y_disp = Map_Display_vals.y;

	// Receive Player Map Position Values
	XY_obj_vals Map_Pos_vals;
	if( xPlayerMapPosQueue != 0 ) xQueuePeek( xPlayerMapPosQueue, &Map_Pos_vals, ( TickType_t ) portMAX_DELAY );
	int16_t x_map = Map_Pos_vals.x;
	int16_t y_map = Map_Pos_vals.y;

	int16_t x_visible_wall_thickness = 0;
	uint8_t x_side_flag;
	int16_t y_visible_wall_thickness = 0;
	uint8_t y_side_flag;



	if (x_map < 128) {x_visible_wall_thickness = x_map - (128/2) + max_wall_thickness; x_side_flag = 0; }
	else { x_visible_wall_thickness = 128*2 - (x_map + max_wall_thickness + (128/2) + x_off/2); x_side_flag = 1; }

	if (y_map < 160) {y_visible_wall_thickness = y_map - (160/2) + max_wall_thickness; y_side_flag = 0; }
	else { y_visible_wall_thickness = 160*2 - (y_map + max_wall_thickness + (160/2) + y_off/2); y_side_flag = 1; }


	if (x_visible_wall_thickness < 0) x_visible_wall_thickness = max_wall_thickness;
	else if (x_visible_wall_thickness > max_wall_thickness) x_visible_wall_thickness = 0;
	else x_visible_wall_thickness = max_wall_thickness - x_visible_wall_thickness;

	if (y_visible_wall_thickness < 0) y_visible_wall_thickness = max_wall_thickness;
	else if (y_visible_wall_thickness > max_wall_thickness) y_visible_wall_thickness = 0;
	else y_visible_wall_thickness = max_wall_thickness - y_visible_wall_thickness;



	if (!x_side_flag)
	{

		if (x_last_thick != x_visible_wall_thickness)
			lcd_draw_fillrect(0, 0, x_last_thick, 160, 0x0000);

		if (x_visible_wall_thickness <= max_wall_thickness )
		{
			lcd_draw_fillrect(0, 0, x_visible_wall_thickness, 160, wall_color);//parede da esquerda - 0

			for (int i=0; i<4; i++) { //Draw Wall Lines
				test = 40*(i+1);
				if (y_map > 160/2 - y_off/2 && y_map < 160*2 - (160/2 + y_off/2))
					test += y_disp - y_map;
				if (test < 0) test   += 160 * (int)((160+abs(test))/160);
				if (test > 160) test -= 160 * (int)(test/160);

				if ( test > 0 && test < 160) lcd_draw_line(0, test, max_wall_thickness, test, 0x0000);
			}
		}
	}
	if (x_side_flag)
	{
		if (x_last_thick != x_visible_wall_thickness)
			lcd_draw_fillrect(128-x_last_thick, 0, x_last_thick, 160, 0x0000);
		if (x_visible_wall_thickness <= max_wall_thickness )
		{
			lcd_draw_fillrect(128-x_visible_wall_thickness, 0, x_visible_wall_thickness, 160, wall_color);//parede da direta - 1

			for (int i=0; i<4; i++) { //Draw Wall Lines
				test = 40*(i+1);
				if (y_map > 160/2 - y_off/2 && y_map < 160*2 - (160/2 + y_off/2))
					test += y_disp - y_map;
				if (test < 0) test   += 160 * (int)((160+abs(test))/160);
				if (test > 160) test -= 160 * (int)(test/160);

				if ( test > 0 && test < 160) lcd_draw_line(128-max_wall_thickness, test, 128, test, 0x0000);
			}
		}
	}



	if (!y_side_flag)
	{
		if (y_last_thick != y_visible_wall_thickness)
			lcd_draw_fillrect(0, 0, 128, y_last_thick, 0x0000);
		if (y_visible_wall_thickness <= max_wall_thickness )
		{
			lcd_draw_fillrect(0, 0, 128, y_visible_wall_thickness, wall_color);//parede de cima - 2

			for (int i=0; i<4; i++) { //Draw Wall Lines
				test = 32*(i+1);
				if (x_map > 128/2 - x_off/2 && x_map < 128*2 - (128/2 + x_off/2))
					test += x_disp - x_map;
				if (test < 0) test   += 128 * (int)((128+abs(test))/128);
				if (test > 128) test -= 128 * (int)(test/128);

				if ( test > 0 && test < 128) lcd_draw_line(test, 0, test, max_wall_thickness, 0x0000);
			}
		}
	}
	if (y_side_flag)
	{
		if (y_last_thick != y_visible_wall_thickness)
			lcd_draw_fillrect(0, 160-y_last_thick, 128, y_last_thick, 0x0000);
		if (y_visible_wall_thickness <= max_wall_thickness )
		{
			lcd_draw_fillrect(0, 160-y_visible_wall_thickness, 128, y_visible_wall_thickness, wall_color);//parede de baixo - 3

			for (int i=0; i<4; i++) { //Draw Wall Lines
				test = 32*(i+1);
				if (x_map > 128/2 - x_off/2 && x_map < 128*2 - (128/2 - x_off/2))
					test += x_disp - x_map;
				if (test < 0) test   += 128 * (int)((128+abs(test))/128);
				if (test > 128) test -= 128 * (int)(test/128);

				if ( test > 0 && test < 128) lcd_draw_line(test, 160-max_wall_thickness, test, 160, 0x0000);
			}
		}
	}


	x_last_thick = x_visible_wall_thickness;
	y_last_thick = y_visible_wall_thickness;

}



extern int8_t clock_time = 15; // Usar interrupção a 1Hz
void Time_Clock( void )
{
	unsigned char msg[3];
	uint16_t clock_color = RGB_Convert(255,0,0);
	uint8_t offset = 0;

	if (uxSemaphoreGetCount( xClockMutex ) == 1)
	{
		xSemaphoreTake( xClockMutex, ( TickType_t) portMAX_DELAY );

		sprintf(msg, "%d", clock_time);
		if (clock_time < 10) offset = 12;
		lcd_draw_string(94 + offset, 12, msg, 0x0000, 2);

		clock_color = RGB_Convert(255,0,255);//RGB_Convert(rand()%255, rand()%255, rand()%255);

		clock_time--;
	}

	if (clock_time < 10) offset = 12;
	else offset = 0;
	sprintf(msg, "%d", clock_time);
	lcd_draw_string(94 + offset, 12, msg, clock_color, 2);

	if (clock_time == 0)
	{
		xSemaphoreGive(xEtapaDoJogo);
		xSemaphoreGive(xDisplayMutex);

		// Reset time
		clock_time = 15;
	} //o jogo expirou

}


void Menu( void )
{
	// Options:
	uint16_t color = RGB_Convert(90, 240, 10);
	uint8_t opt[4] = {10, 90, 70, 20};

	lcd_draw_string( 10, 20, "DVD", RGB_Convert(90, 240, 100), 2);
	lcd_draw_string( 10, 40, "Dungeon", RGB_Convert(90, 240, 100), 2);

	lcd_draw_rect(opt[0], opt[1], opt[2], opt[3], color);
	lcd_draw_string( opt[0] + 5, opt[1] + (opt[3]/2), "Start", 0xFFFF, 1);

	lcd_draw_rect(opt[0], opt[1]+30, opt[2], opt[3], color);
	lcd_draw_string( opt[0] + 5, opt[1]+30 + (opt[3]/2), "Settings", 0xFFFF, 1);

}


void Clear_Screen( void )
{
	lcd_draw_fillrect(0, 0, 128, 160, 0x0000); // Clear Screen
}

void Back_to_menu( void )
{
	uint8_t back[4] = {22, 140, 80, 12};
	// Back to menu
	uint16_t color = RGB_Convert(255, 10, 10);

	lcd_draw_rect(back[0], back[1], back[2], back[3], color);
	lcd_draw_string( back[0]+5, back[1]+3, "Back to Menu", color, 1);
}


void Mute( void )
{
	uint16_t color_m;
	uint8_t mus[4] = {68, 98, 30, 12};

	lcd_draw_string( mus[0]-40, mus[1]+3, "Music:", 0xFFFF, 1);

	if (uxSemaphoreGetCount(xMusicOnOffMutex) == 1)
	{
		color_m = RGB_Convert(0, 255, 0);
		lcd_draw_string( mus[0]+5, mus[1]+3, "ON ", color_m, 1);
	}
	if (uxSemaphoreGetCount(xMusicOnOffMutex) == 0)
	{
		color_m = RGB_Convert(255, 0, 0);
		lcd_draw_string( mus[0]+5, mus[1]+3, "OFF", color_m, 1);
	}

	lcd_draw_rect(mus[0], mus[1], mus[2], mus[3], color_m);
}

extern XYcolor_CDs last_cd1;
extern XYcolor_CDs last_cd2;
extern XYcolor_CDs last_cd3;
extern uint8_t flag_cor = 1;
void CDs_perdidos( XYcolor_CDs cd1, XYcolor_CDs cd2, XYcolor_CDs cd3 )
{
	uint8_t flag_cd1 = 0;
	uint8_t flag_cd2 = 0;
	uint8_t flag_cd3 = 0;

	uint8_t x_off = 35+14;
	uint8_t y_off = 15+14;
	// Receive Player Display Position Values
	XY_obj_vals Display_Pos_vals;
	if( xPlayerDisplayPosQueue != 0 ) xQueuePeek( xPlayerDisplayPosQueue, &Display_Pos_vals, ( TickType_t ) portMAX_DELAY );
	int16_t x_disp = Display_Pos_vals.x;
	int16_t y_disp = Display_Pos_vals.y;

	XY_obj_vals Map_Pos_vals;
	if( xPlayerMapPosQueue != 0 ) xQueuePeek( xPlayerMapPosQueue, &Map_Pos_vals, ( TickType_t ) portMAX_DELAY );
	int16_t x_map = Map_Pos_vals.x;
	int16_t y_map = Map_Pos_vals.y;

	cd1.x = cd1.x-x_map+x_disp;
	cd1.y = cd1.y-y_map+y_disp;
	cd2.x = cd2.x-x_map+x_disp;
	cd2.y = cd2.y-y_map+y_disp;
	cd3.x = cd3.x-x_map+x_disp;
	cd3.y = cd3.y-y_map+y_disp;

	if (flag_cor)
	{
		last_cd1.cor = cd1.cor;
		last_cd2.cor = cd2.cor;
		last_cd3.cor = cd3.cor;
		flag_cor = 0;
	}

	if (flag_cd1 == 1) {CD(cd1.x, cd1.y, 0x0000); flag_cd1 = 0;}
	if (cd1.x > 0-x_off && cd1.x<128+x_off && cd1.y > 0-y_off && cd1.y<160+y_off)
	{
		CD(last_cd1.x, last_cd1.y, 0x0000);
		CD(cd1.x, cd1.y, last_cd1.cor);
		flag_cd1 = 1;
	}

	if (flag_cd2 == 1) {CD(cd2.x, cd2.y, 0x0000); flag_cd2 = 0;}
	if (cd2.x > 0-x_off && cd2.x<128+x_off && cd2.y > 0-y_off && cd2.y<160+y_off)
	{
		CD(last_cd2.x, last_cd2.y, 0x0000);
		CD(cd2.x, cd2.y, last_cd2.cor);
		flag_cd2 = 1;
	}


	if (flag_cd3 == 1) {CD(cd3.x, cd3.y, 0x0000); flag_cd3 = 0;}
	if (cd3.x > 0-x_off && cd3.x<128+x_off && cd3.y > 0-y_off && cd1.y<160+y_off)
	{
		CD(last_cd3.x, last_cd3.y, 0x0000);
		CD(cd3.x, cd3.y, last_cd3.cor);
		flag_cd3 = 1;
	}


	last_cd1.x = cd1.x;
	last_cd1.y = cd1.y;
	last_cd2.x = cd2.x;
	last_cd2.y = cd2.y;
	last_cd3.x = cd3.x;
	last_cd3.y = cd3.y;

}

uint16_t cnt = 0;
uint8_t last_x = 0;//128/2-50;

//extern uint8_t history_time = 28;
//extern int16_t last_y = 130;

void History_Text( void )
{
	char msgtotal[27][21] = {"Diogo Vasco Dante,",
							"um explorador",
							"de renome mundial",
							"encontra-se numa",
							"aventura perigosa,",
							"se nao a mais",
							"periogosa",
							"da sua vida.",
							" ",
							"Na esperanca",
							"de libertar",
							"o messias,",
							"preso nas masmorras",
							"do seu arquinimigo",
							"Bruno Reiquejavique.",
							" ",
							"Diogo Vasco Dante",
							"tem 15 segundos",
							"para achar os",
							"3 artefactos ancioes",
							"FDADM",
							"(Formato Digital",
							"de Armazenamento",
							"de Dados Multimedia)",
							"e libertar",
							"o messias,",
							"salvando a galaxia.",
	};



	if (uxSemaphoreGetCount( xClockMutex ) == 1)
	{
		xSemaphoreTake( xClockMutex, ( TickType_t) portMAX_DELAY );
		history_time--;
	}

	uint8_t esp = 20;
	uint8_t off = 5;//10;

	last_y -= off;

	for (int i=0;i<27;i++)
	{
		if (last_y+esp*i+off < 130 && last_y+off > (-esp)*i)
			lcd_draw_fillrect(last_x, last_y+esp*i+off, 128, 10, 0x0000);
//			lcd_draw_string(last_x, last_y+esp*i+off, msgtotal[i], 0x0000, 1);
		if (last_y+esp*i < 130 && last_y > (-esp)*i)
			lcd_draw_string(last_x, last_y+esp*i, msgtotal[i], 0xFF00, 1);
	}


	if (history_time == 0)
	{
		xSemaphoreGive(xEtapaDoJogo);
		xSemaphoreGive(xEtapaDoJogo);
		xSemaphoreGive(xDisplayMutex);
		history_time = 30;
		last_y = 130;
	}

}

void Win_Lose( void )
{
	lcd_draw_string(4, 150, (unsigned char*) "Press SW5 to restart", RGB_Convert(255, 80, 10), 1);

	if ( uxSemaphoreGetCount( xWinLose ) == 0 )
	{
		lcd_draw_string(18, 20, (unsigned char*) "YOU LOST", RGB_Convert(255, 80, 100), 2);
		dvd(128/2-34/2, 160/2, RGB_Convert(255, 100, 80));
	}

	else
	{
		lcd_draw_string(22, 20, (unsigned char*) "YOU WON", RGB_Convert(80, 255, 100), 2);

		dvd(128/2-34/2, 160/2+2, RGB_Convert(100, 255, 80));
		CD(128/2-34/2, 160/2+4, RGB_Convert(100, 255, 80));
		CD(128/2-34*3/2-5, 160/2-22, RGB_Convert(255, 80, 100));
		CD(128/2+34/2+3, 160/2-22, RGB_Convert(80, 100, 255));
	}
}


