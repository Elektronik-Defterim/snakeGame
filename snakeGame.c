#include "stm32f4xx_hal.h"
#include "ili9341.h"
#include <string.h>
#include <stdlib.h>

static char *names[] ={"Background","Color","Level","Start"};
static uint16_t color[]={
		COLOR_BLACK,
		COLOR_NAVY,
		COLOR_DGREEN,
		COLOR_DCYAN,
		COLOR_MAROON,
		COLOR_PURPLE,
		COLOR_OLIVE,
		COLOR_LGRAY,
		COLOR_DGRAY,
		COLOR_BLUE,
		COLOR_BLUE2,
		COLOR_GREEN,
		COLOR_GREEN2,
		COLOR_GREEN3,
		COLOR_CYAN,
		COLOR_RED,
		COLOR_MAGENTA,
		COLOR_YELLOW,
		COLOR_WHITE,
		COLOR_ORANGE,
		COLOR_GREENYELLOW,
		COLOR_BROWN,
		COLOR_NEW,
		COLOR_BLUISH,
		COLOR_SELECT1
};

typedef struct{
	uint8_t backgroundColor;
	uint8_t snakeColor;
	uint8_t snakeSpeed;
}settings;
settings game;

uint16_t colorState[3]={COLOR_SELECT1,COLOR_BLUE2,COLOR_GREEN};
bool startGame = false;
uint16_t size;
extern TIM_HandleTypeDef htim2;
extern RNG_HandleTypeDef hrng;
int16_t direction;
uint16_t snakeCoordinate[660];
uint16_t bait;
bool eatbait = false;
uint16_t colorBait;


void getBaitCoordinate();

void gameDefaultSettings(){
	game.backgroundColor = 0;game.snakeColor = 2;game.snakeSpeed = 1;
	snakeCoordinate[3] = 354; snakeCoordinate[2] = 355; snakeCoordinate[1] = 356; snakeCoordinate[0] = 357;
	for(int i=4;i<size;i++)
		snakeCoordinate[i] = '\0';
	size = 4;
	direction = 1;
	getBaitCoordinate();
}

void drawPart(uint16_t coordinate,uint16_t color){
	uint16_t mx = coordinate%32;
	uint16_t my = coordinate/32;
	ILI9341_Fill_Rect(mx*10+1,my*10+1,mx*10+9,my*10+9,color);
}

bool baitControl(uint16_t controlBait){
	for(int i=0; i<(int)size;i++){
		if(controlBait == snakeCoordinate[i]){
			return false;
		}
	}
	bait = controlBait;
	return true;
}

void getBaitCoordinate(){
	uint16_t baitY = (HAL_RNG_GetRandomNumber(&hrng)%22)+1;
	uint16_t baitX = (HAL_RNG_GetRandomNumber(&hrng)%30)+1;
	int value= (HAL_RNG_GetRandomNumber(&hrng)%25);
	if(color[value] == color[game.backgroundColor]){
		if(game.backgroundColor<=23)
			value +=1;
		else{
			value -=1;
		}
	}
	colorBait = color[value];
	uint16_t sayac = 0;

	bait = (baitY*32)+baitX;
	bool control = baitControl(bait);

	while(!control){
		for(uint16_t i = 1 ;i<31;i++){
			if(((int)baitY+(int)sayac) <= 22){
				if(baitControl(((baitY + sayac)*32)+i)){
					control = true;
					break;}}

			else if(((int)baitY-(int)sayac) >= 1){
				if(baitControl(((baitY + sayac)*32)+i)){
					control = true;
					break;}
			}
		}
		sayac++;
	}
}

bool isDead(){
	if((snakeCoordinate[0]) % 32 != 30 || direction != 1){
		if((snakeCoordinate[0]) % 32 != 1 || direction != -1){
			if((snakeCoordinate[0]+direction) < 736 && (snakeCoordinate[0]+direction)>=32){
				for(int i=2;i<size-1;i++){
					if((snakeCoordinate[0]+direction) == snakeCoordinate[i]){
						return true;
					}
				}
				return false;
			}
		}
	}
	return true;
}

void snakeAction(){
	if(isDead()==false){
		if(bait == (snakeCoordinate[0]+direction)){
			drawPart(bait,color[game.backgroundColor]);
			getBaitCoordinate();
			eatbait = true;
		}
		drawPart(snakeCoordinate[size-1],color[game.backgroundColor]);
		drawPart(snakeCoordinate[0]+direction,color[game.snakeColor]);
		drawPart(bait,colorBait);
		for(int i = size-1;i>=1;i--){
			snakeCoordinate[i] = snakeCoordinate[i-1];
		}
		snakeCoordinate[0] = snakeCoordinate[1]+direction;
		if(eatbait){
			size++;
			snakeCoordinate[size-1] = snakeCoordinate[size-2] - direction;
			eatbait = false;
		}
	}
	else{
		startGame = false;
		HAL_TIM_Base_Stop_IT(&htim2);
	}

}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
 static int counter = 0;
  if(htim->Instance == TIM2){
	  counter++;
	  if(counter == 20){
		  snakeAction();
	 	  counter = 0;
	  }
  }
}

void lcdInit(){
	ILI9341_Init();
	ILI9341_Configuration();
	HAL_Delay(500);
	ILI9341_setRotation(4);
	ILI9341_Fill(COLOR_NEW);
}

void updateMenu(uint8_t state,uint16_t a){
	uint16_t rectStart = 61+(a*45);
	uint16_t yCoordinate = 77+(a*45);
	ILI9341_Fill_Rect(0,rectStart,320,rectStart+44,colorState[state]);
	ILI9341_printText(names[a],20,yCoordinate,COLOR_WHITE,COLOR_SELECT1,2);

	if(a == 0){
		ILI9341_Fill_Rect(240,68,270,98,color[game.backgroundColor]);
	}

	else if(a==1){
		ILI9341_Fill_Rect(240,113,270,143,color[game.snakeColor]);
	}

	else if(a==2){
		char c[3];
		sprintf(c,"%u",game.snakeSpeed);
		ILI9341_printText(c,240,167,COLOR_WHITE,COLOR_SELECT1,2);
	}

}

void setColor(bool increase,uint8_t *features){
	uint8_t value = *features;
	if(increase == true)
	{
		if(color[value+1])
			*features +=1;
		else
			*features = 0;
	}

	else{
		if(color[value-1])
			*features -=1;
		else
			*features=24;
	}

}

void changeGameFeatures(uint8_t parameter,bool increase){
	if(parameter == 0)
		setColor(increase,(uint8_t *)&game.backgroundColor);

	else if(parameter == 1)
		setColor(increase,(uint8_t *)&game.snakeColor);

	else{
		if(increase == true && game.snakeSpeed <10)
			game.snakeSpeed++;

		else if(increase == false && game.snakeSpeed > 1)
			game.snakeSpeed--;
	}
}

void setDirection(uint8_t state){
	if(state !=0 && state!=3){
		if(state == 2){
			if(direction==-32 || direction ==32){
				direction = 1;
			}

			else if(direction == -1 || direction == 1)
				direction = -32;
		}

		 else if(state == 1){
			if(direction == -1 || direction == 1)
				direction = 32;

			else if(direction==-32 || direction == 32)
				direction = -1;
		}

	}
}

static void waitButton(GPIO_TypeDef *Port,uint16_t pin){
	while((HAL_GPIO_ReadPin(Port,pin)));
}

uint8_t buttonControl(){
	uint8_t state = 0;
	if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)){
		state = 1;
		if(startGame)
			setDirection(state);
		waitButton(GPIOA,GPIO_PIN_0);

	}

	else if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_1)){
		state = 2;
		if(startGame)
			setDirection(state);
		waitButton(GPIOA,GPIO_PIN_1);

	}

	else if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_4)){
		state = 3;
		if(startGame)
			setDirection(state);
		waitButton(GPIOA,GPIO_PIN_4);
	}
	return state;
}

void createMenu(){
	ILI9341_Fill_Rect(0,61,320,240,COLOR_SELECT1);

	ILI9341_printText("Background",20,77,COLOR_WHITE,COLOR_SELECT1,2);
	ILI9341_Fill_Rect(240,68,270,98,color[game.backgroundColor]);

	ILI9341_printText("Color",20,122,COLOR_WHITE,COLOR_SELECT1,2);
	ILI9341_Fill_Rect(240,113,270,143,color[game.snakeColor]);

	ILI9341_printText("Level",20,167,COLOR_WHITE,COLOR_SELECT1,2);
	ILI9341_printText("1",240,167,COLOR_WHITE,COLOR_SELECT1,2);

	ILI9341_printText("Start",20,212,COLOR_WHITE,COLOR_SELECT1,2);
	updateMenu(1,0);

}

void menuControl(){
	static int counter = 0;
	static uint8_t buttonOK = 0;
	uint8_t state = buttonControl();
	if(buttonOK == 0 && state!=0){
		updateMenu(0,counter);
		if(state == 1){
			counter++;
			if(counter > 3)
				counter = 0;
			updateMenu(1,counter);
		}

		else if(state == 2){
			counter--;
			if(counter < 0)
					counter = 3;
			updateMenu(1,counter);
		}

		else if(state == 3){
			buttonOK = 1;
			updateMenu(2,counter);
			if(counter == 3){
				startGame = true;
				buttonOK = 0;
				counter = 0;
				state = 0;
			}
		}
	}

	else if(buttonOK == 1 && state!=0){
		if(state == 1){
			changeGameFeatures(counter,true);
			updateMenu(2,counter);
		}

		else if(state == 2){
			changeGameFeatures(counter,false);
			updateMenu(2,counter);
		}

		else {
			buttonOK = 0;
			updateMenu(1,counter);
		}
	}
}

void setConfigurationGame(){
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_5,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6,GPIO_PIN_RESET);
	ILI9341_Fill(COLOR_BLACK);
	ILI9341_Fill_Rect(0,0,320,60,COLOR_NEW);
	ILI9341_printText("Snake Game",70,18,COLOR_WHITE,COLOR_NEW,3);
	createMenu();
	while(1){
		if(!startGame)
			menuControl();
		else break;
	}
}

void deadAnimation(){
	int sayac = 0;
	while(sayac <= 3){

		for(int i = size-1;i>=0;i--){
			drawPart(snakeCoordinate[i],color[game.snakeColor]);
		}
		HAL_Delay(500);
		for(int i = size-1;i>=0;i--){
			drawPart(snakeCoordinate[i],color[game.backgroundColor]);
		}
		HAL_Delay(500);
		sayac++;
	}

	char c[12];
	sprintf(c,"SKORE : %d",((size-3)*10));
	for(int i = 0;i<strlen(c);i++){
		uint16_t x = 36+(24*i);
		ILI9341_drawChar(x,111,c[i],COLOR_BLUE2,color[game.backgroundColor],3);
		HAL_Delay(200);
	}
	HAL_Delay(2000);

}

void start(){
	ILI9341_Fill(color[game.backgroundColor]);
	ILI9341_Fill_Rect(7,7,313,10,COLOR_MAGENTA);
	ILI9341_Fill_Rect(310,10,313,233,COLOR_MAGENTA);
	ILI9341_Fill_Rect(7,10,10,233,COLOR_MAGENTA);
	ILI9341_Fill_Rect(10,230,313,233,COLOR_MAGENTA);
	HAL_TIM_Base_Start_IT(&htim2);
	while(startGame){
		buttonControl();
		HAL_Delay(40);
	}
	deadAnimation();
}

void animation(){
	ILI9341_Fill(COLOR_BLACK);
	char c[]="Elektronik";
	int endChar = 9;
	uint16_t coordinateX = 0;

	while(coordinateX <= 234){
		for(int j = 1;j<=9;j++){
			coordinateX +=2;
			for(int y = 9 ; y >=endChar ; y--){
				if(coordinateX <= 234){
					ILI9341_printText(&c[y],coordinateX-((9-y)*18),87,COLOR_WHITE,COLOR_BLACK,3);
				}
			}
			HAL_Delay(20);

			for(int y = 9 ; y >=endChar && coordinateX<234; y--){
				ILI9341_printText(&c[y],coordinateX-((9-y)*18),87,COLOR_BLACK,COLOR_BLACK,3);
			}

		}
		if(endChar>0)
			endChar--;
	}

	HAL_Delay(300);
	ILI9341_printText("Defterim",80,133,COLOR_WHITE,COLOR_NEW,3);
	HAL_Delay(1000);
}

extern void snakeGame(){
	lcdInit();
	animation();
	while(1){
	gameDefaultSettings();
	setConfigurationGame();
	start();
	}
}
