/*
 *
 8051 GLCD 128x64 Text character font
 http://www.electronicWings.com
 *
 *  Modificado para adicionar a tela de início "Stock Car", aceleração e explosão no local.
 */

#include <reg51.h>          // Inclui o arquivo de cabeçalho reg51
#include <intrins.h>        // Inclui o arquivo de cabeçalho intrínseco
#include "Font_Header.h"
#include <stdlib.h>         // Para rand()
#include <stdio.h>          // Para a função sprintf

#define Data_Port P3        // Define a porta de dados para o GLCD
#define ROAD_WIDTH 60
#define NO_OBSTACLE 255
#define INITIAL_DELAY 300
#define MIN_DELAY 50
#define SPEED_INCREASE 40

sbit RS = P2^0;              // Define os pinos de bits de controle
sbit RW = P2^1;
sbit E = P2^2;
sbit CS1 = P2^3;
sbit CS2 = P2^4;
sbit RST = P2^5;
sbit RIGHT = P1^0;
sbit LEFT = P1^1;

// --- Protótipos de Funções ---
// <<< MUDANÇA: O protótipo agora aceita coordenadas
void GLCD_DisplayExplosion(unsigned char col, unsigned char page);
void GLCD_DisplayGameOver(unsigned int points);

void delay(unsigned int k)
{
	unsigned int i,j;
	for (i=0;i<k;i++)
		for (j=0;j<112;j++);
}

void GLCD_Command(char Command)
{
	Data_Port = Command;
	RS = 0;
	RW = 0;
	E = 1;
	_nop_ ();
	E = 0;
	_nop_ ();
}

void GLCD_Data(char Data)
{
	Data_Port = Data;
	RS = 1;
	RW = 0;
	E = 1;
	_nop_ ();
	E = 0;
	_nop_ ();
}

void GLCD_Init()
{
	CS1 = 1; CS2 = 1;
	RST = 1;
	delay(20);
	GLCD_Command(0x3E);
	GLCD_Command(0x40);
	GLCD_Command(0xB8);
	GLCD_Command(0xC0);
	GLCD_Command(0x3F);
}

void GLCD_ClearAll()
{
	int i,j;
	CS1 = 1; CS2 = 1;
	for(i=0;i<8;i++)
	{
		GLCD_Command((0xB8)+i);
		for(j=0;j<64;j++)
		{
			GLCD_Data(0);
		}
	}
	GLCD_Command(0x40);
	GLCD_Command(0xB8);
}

void GLCD_DisplayChar(char c, char col_start, unsigned char page)
{
	unsigned char col;
	unsigned int i = 0;

	for (i = 0; i < 5; i++)
	{
		col = col_start + i;
		if (col >= 64)
		{
			CS1 = 0; CS2 = 1;
			GLCD_Command(0xB8 + page);
			GLCD_Command(0x40 + (col - 64));
		}
		else
		{
			CS1 = 1; CS2 = 0;
			GLCD_Command(0xB8 + page);
			GLCD_Command(0x40 + col);
		}
		GLCD_Data(font[c - 32][i]);
	}
	GLCD_Data(0x00);
}

void GLCD_DisplayString(char* str, char col_start, unsigned char page)
{
    while (*str)
    {
        GLCD_DisplayChar(*str, col_start, page);
        col_start += 6;
        str++;
    }
}

void GLCD_DisplayCharCenter(char c, char col_start)
{
    unsigned char page = 6;
	unsigned char col;
	unsigned int i = 0;

    for (i = 0; i < 5; i++)
    {
		col = col_start+i;
		if (col >= 64)
		{
			CS1 = 0; CS2 = 1;
			GLCD_Command(0xB8 + page);
			GLCD_Command(0x40 + (col - 64));
		}
		else
		{
			CS1 = 1; CS2 = 0;
			GLCD_Command(0xB8 + page);
			GLCD_Command(0x40 + col);
		}
		GLCD_Data(font[c - 32][i]);
    }
    GLCD_Data(0x00);
}

void GLCD_DisplayPoints(unsigned int points, char col_start, unsigned char page)
{
	char buffer[4];
	sprintf(buffer, "%03d", points);
	GLCD_DisplayString(buffer, col_start, page);
}

void GLCD_DisplayRoad(char c, unsigned char col_start, unsigned char page)
{
	GLCD_DisplayChar(c, col_start, page);
	GLCD_DisplayChar(c, col_start + ROAD_WIDTH, page);
}

// <<< MUDANÇA: Função de explosão agora recebe coordenadas e não limpa a tela.
void GLCD_DisplayExplosion(unsigned char col, unsigned char page)
{
	// Não limpa a tela para manter o cenário visível.
	// Apenas desenha "BOOM!" sobre a posição da colisão.
	// Ajustamos a posição inicial para tentar centralizar a palavra na colisão.
	if (col > 20) {
		GLCD_DisplayString("BOOM!", col - 15, page);
	} else {
		GLCD_DisplayString("BOOM!", col, page);
	}
	delay(1500); // Mostra a explosão por 1.5 segundos
}

void GLCD_DisplayGameOver(unsigned int points){
	char points_str[12];
	sprintf(points_str, "Points: %03d", points);

	GLCD_ClearAll(); // A tela de Game Over limpa o display
	GLCD_DisplayString("Game Over", 35, 3);
	GLCD_DisplayString(points_str, 29, 4);
}

void Start_Screen()
{
	GLCD_ClearAll();
	GLCD_DisplayString("Stock Car", 34, 3);
	GLCD_DisplayString("Pressione para", 20, 5);
	GLCD_DisplayString("iniciar", 40, 6);

	while(RIGHT == 1 && LEFT == 1);

	GLCD_ClearAll();
	delay(100);
}

void main()
{
    unsigned char position_car = 62;
    unsigned char position_road = 32;
    unsigned char page = 0;
    unsigned char road_positions[8];
    unsigned char obstacles[8];
    unsigned char new_pos;
    unsigned char obstacle_position;
    unsigned char obstacle_chance;
    int move = 0;
    unsigned int move_contador = 0;
	unsigned int points = 0;
    unsigned int game_delay = INITIAL_DELAY;

    GLCD_Init();

	Start_Screen();

    for (page = 0; page < 8; page++) {
        road_positions[page] = 32;
        obstacles[page] = NO_OBSTACLE;
    }

    srand(1);

    GLCD_ClearAll();
    GLCD_DisplayCharCenter('A', position_car);

    while (1)
    {
        obstacle_position = rand() % (ROAD_WIDTH - 5);
        obstacle_chance = rand() % 15;

        if (!RIGHT)
            position_car += 5;

        if (!LEFT)
            position_car -= 5;

        if (position_car > 127 - 5)
            position_car = 127 - 5;
		if (position_car < 5)
			position_car = 0;

        new_pos = road_positions[0];
        if (move_contador == 0) {
            move = rand() % 3;
            move_contador = rand() % 5 + 3;
        }

        if (move == 0 && new_pos > 5)
            new_pos -= 2;
        else if (move == 2 && new_pos < (128 - ROAD_WIDTH - 5))
            new_pos += 2;

        move_contador--;

        for (page = 7; page > 0; page--) {
            road_positions[page] = road_positions[page - 1];
            obstacles[page] = obstacles[page - 1];
        }

        if (position_car < road_positions[6] + 5)
            position_car = road_positions[6] + 5;

        if (position_car > road_positions[6] + ROAD_WIDTH - 5)
            position_car = road_positions[6] + ROAD_WIDTH - 5;

        road_positions[0] = new_pos;

        if (obstacle_chance < 4)
            obstacles[0] = new_pos + 5 + obstacle_position;
        else
            obstacles[0] = NO_OBSTACLE;

        GLCD_ClearAll();

        for (page = 0; page < 8; page++) {
            GLCD_DisplayRoad('|', road_positions[page], page);
            if (obstacles[page] != NO_OBSTACLE)
                GLCD_DisplayChar('X', obstacles[page], page);
        }

        GLCD_DisplayCharCenter('A', position_car);

        // --- MUDANÇA: Passa as coordenadas da colisão para a função de explosão ---
        if (obstacles[6] != NO_OBSTACLE &&
            position_car >= obstacles[6] &&
            position_car <= obstacles[6] + 4)
        {
			// Chama a explosão na posição do carro (position_car) e na página da colisão (6)
			GLCD_DisplayExplosion(position_car, 6);
			GLCD_DisplayGameOver(points);
            while (1);
        }

		GLCD_DisplayPoints(points, 0 , 0);
		points++;

		if (points > 0 && (points % 25 == 0))
		{
			if (game_delay > MIN_DELAY)
			{
				game_delay -= SPEED_INCREASE;
			}
		}

		delay(game_delay);
    }
}