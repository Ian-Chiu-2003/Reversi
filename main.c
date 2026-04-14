/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "gpio.h"
#include <stdbool.h>
#include <stdio.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "st7735.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define CELL_SIZE 16  // ?? 16x16 pixels (128 / 8 = 16)
#define COLOR_LIGHT_BROWN  ST7735_COLOR565(205, 133, 63)
/*new*/

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

void ST7735_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define CELL_SIZE 16  // ???? 16x16 pixels
#define BOARD_ORIGIN_X 0
#define BOARD_ORIGIN_Y 0
#define COLOR_BROWN 0xA145  // ??? RGB565
#define COLOR_LIGHT_GRAY ST7735_COLOR565(200, 200, 200)

int cursor_row = 3, cursor_col = 3; // ???????

enum CellState {
    EMPTY = 0,
    BLACK = 1,
    WHITE = 2
};
uint8_t board[8][8];

int dr[8] = {-1,-1,-1, 0, 1, 1, 1, 0}; // row??,?-??-?-??-?-??-?-??
int dc[8] = {-1, 0, 1, 1, 1, 0,-1,-1};


enum CellState current_player = BLACK;
uint8_t board[8][8]={0};  // ????

void draw_initial_pieces(void);
void draw_board();
void draw_circle_piece(int row, int col, enum CellState state);
void try_place_and_flip(int row, int col, enum CellState player);
void refresh_board_display();
void show_piece_count_icon(void);
bool is_board_full(void);
void show_final_winner(void);

#define ROW1_PIN    GPIO_PIN_8
#define ROW2_PIN    GPIO_PIN_9
#define ROW3_PIN    GPIO_PIN_10
#define ROW4_PIN    GPIO_PIN_11
#define ROW_PORT    GPIOA

#define COL1_PIN    GPIO_PIN_3
#define COL2_PIN    GPIO_PIN_4
#define COL3_PIN    GPIO_PIN_5
#define COL4_PIN    GPIO_PIN_6
#define COL_PORT    GPIOB
void move_cursor(int d_row, int d_col) {
    cursor_row += d_row;
    cursor_col += d_col;
    if (cursor_row < 0) cursor_row = 0;
    if (cursor_row > 7) cursor_row = 7;
    if (cursor_col < 0) cursor_col = 0;
    if (cursor_col > 7) cursor_col = 7;
}

void scan_keypad_and_control(void)
{
    // ???? ROW ????
    HAL_GPIO_WritePin(ROW_PORT, ROW1_PIN | ROW2_PIN | ROW3_PIN | ROW4_PIN, GPIO_PIN_SET);

    // ? ROW ??
    // ?(S2 = ROW1 + COL2)
    HAL_GPIO_WritePin(ROW_PORT, ROW1_PIN, GPIO_PIN_RESET);
    if (HAL_GPIO_ReadPin(COL_PORT, COL2_PIN) == GPIO_PIN_RESET) { // S2: ?
        move_cursor(-1, 0);
        HAL_Delay(120);
    }
    HAL_GPIO_WritePin(ROW_PORT, ROW1_PIN, GPIO_PIN_SET);

    // ?(S5 = ROW2 + COL1),?(S7 = ROW2 + COL3)
    HAL_GPIO_WritePin(ROW_PORT, ROW2_PIN, GPIO_PIN_RESET);
    if (HAL_GPIO_ReadPin(COL_PORT, COL1_PIN) == GPIO_PIN_RESET) { // S5: ?
        move_cursor(0, -1);
        HAL_Delay(400);
    }
    if (HAL_GPIO_ReadPin(COL_PORT, COL3_PIN) == GPIO_PIN_RESET) { // S7: ?
        move_cursor(0, 1);
        HAL_Delay(400);
    }
    HAL_GPIO_WritePin(ROW_PORT, ROW2_PIN, GPIO_PIN_SET);

    // ?(S10 = ROW3 + COL2),???(S9 = ROW3 + COL1),???(S11 = ROW3 + COL3)
    HAL_GPIO_WritePin(ROW_PORT, ROW3_PIN, GPIO_PIN_RESET);
    if (HAL_GPIO_ReadPin(COL_PORT, COL2_PIN) == GPIO_PIN_RESET) { // S10: ?
        move_cursor(1, 0);
        HAL_Delay(400);
    }
    if (HAL_GPIO_ReadPin(COL_PORT, COL1_PIN) == GPIO_PIN_RESET) { // S9: ???
        try_place_and_flip(cursor_row, cursor_col, BLACK);
        HAL_Delay(200);
    }
    if (HAL_GPIO_ReadPin(COL_PORT, COL3_PIN) == GPIO_PIN_RESET) { // S11: ???
        try_place_and_flip(cursor_row, cursor_col, WHITE);
        HAL_Delay(200);
    }
    HAL_GPIO_WritePin(ROW_PORT, ROW3_PIN, GPIO_PIN_SET);

    // ?? ROW4 ?????
}


// ???????/????
bool need_redraw_board = true;
void draw_cursor(bool blink_on, int row, int col)
{
    int x = BOARD_ORIGIN_X + col * CELL_SIZE;
    int y = BOARD_ORIGIN_Y + row * CELL_SIZE;
    ST7735_DrawFilledRectangle(x+2, y+2, CELL_SIZE-4, CELL_SIZE-4, COLOR_LIGHT_BROWN);
    draw_circle_piece(row, col, board[row][col]);
    if (blink_on) {
        ST7735_DrawRectangle(x+2, y+2, CELL_SIZE-4, CELL_SIZE-4, ST7735_YELLOW);
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
       // ??????
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_SPI1_Init();


  ST7735_Init();                    // ????? // ????
  draw_board();  
  draw_initial_pieces();  

bool blink_on = true;
  uint32_t last_blink = HAL_GetTick();

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
 

  /* USER CODE BEGIN Init */
/*HAL_Init();
SystemClock_Config();*/
  /* USER CODE END Init */

  /* Configure the system clock */


  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */

  /* USER CODE BEGIN 2 */
/*ST7735_Init();*/
  /*MX_GPIO_Init();
  MX_SPI1_Init();*/
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */


    // ??????


    // ????

bool need_redraw_board = true;
int last_cursor_row = 3, last_cursor_col = 3;


while (1)
{
    scan_keypad_and_control();

    if (need_redraw_board) {
draw_board();
for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
            draw_circle_piece(i, j, board[i][j]);
   
    need_redraw_board = 0;
show_piece_count_icon();  // ? ?????????
}

    static uint32_t last_blink = 0;
    static bool blink_on = false;
    static int last_cursor_row = -1, last_cursor_col = -1;

    if (HAL_GetTick() - last_blink > 300) {
        // ????????????
        if (last_cursor_row != cursor_row || last_cursor_col != cursor_col) {
            draw_cursor(false, last_cursor_row, last_cursor_col); // ?????
            last_cursor_row = cursor_row;
            last_cursor_col = cursor_col;
        }
        blink_on = !blink_on;
        draw_cursor(blink_on, cursor_row, cursor_col);
        last_blink = HAL_GetTick();
    }
    HAL_Delay(10);
}

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

/*ST7735_Init();
ST7735_FillScreen(ST7735_BLUE);
HAL_Delay(100);
ST7735_FillScreen(ST7735_RED);
HAL_Delay(100);
ST7735_FillScreen(ST7735_YELLOW);
HAL_Delay(100);
ST7735_FillScreen(ST7735_GREEN);
HAL_Delay(100);
ST7735_FillScreen(ST7735_BLACK);
HAL_Delay(100);*/
  }
  /* USER CODE END 3 */


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void ST7735_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    for (uint16_t i = 0; i < width; i++) {
        for (uint16_t j = 0; j < height; j++) {
            ST7735_DrawPixel(x + i, y + j, color);
        }
    }
}



void draw_circle_piece(int row, int col, enum CellState state)
{
    if (state == EMPTY) return; // ????

    // ??????
    uint16_t color = (state == BLACK) ? ST7735_BLACK : ST7735_WHITE;

    // ?????????
    uint16_t center_x = BOARD_ORIGIN_X + col * CELL_SIZE + CELL_SIZE / 2;
    uint16_t center_y = BOARD_ORIGIN_Y + row * CELL_SIZE + CELL_SIZE / 2;

    // ?????????????
    uint16_t radius = CELL_SIZE / 2 - 2;

    ST7735_DrawFilledCircle(center_x, center_y, radius, color);
}





void draw_board(void)
{
    // 1. ????
    ST7735_FillScreen(COLOR_LIGHT_BROWN);

    // 2. ???(9?,??8???9??)
    for (int i = 0; i <= 8; i++) {
        int x = BOARD_ORIGIN_X + i * CELL_SIZE;
        ST7735_DrawRectangle(x, BOARD_ORIGIN_Y, 1, 8 * CELL_SIZE, ST7735_BLACK);
    }

    // 3. ???(9?)
    for (int i = 0; i <= 8; i++) {
        int y = BOARD_ORIGIN_Y + i * CELL_SIZE;
        ST7735_DrawRectangle(BOARD_ORIGIN_X, y, 8 * CELL_SIZE, 1, ST7735_BLACK);
    }
}


void draw_initial_pieces(void)
{
    // ?????????
    board[3][3] = WHITE;
    board[3][4] = BLACK;
    board[4][3] = BLACK;
    board[4][4] = WHITE;

    // ?????
    draw_circle_piece(3, 3, WHITE);
    draw_circle_piece(3, 4, BLACK);
    draw_circle_piece(4, 3, BLACK);
    draw_circle_piece(4, 4, WHITE);
}

/*
int dr[8] = {-1,-1,-1, 0, 1, 1, 1, 0}; // ???
int dc[8] = {-1, 0, 1, 1, 1, 0,-1,-1};
*/
int is_valid_move(int row, int col, enum CellState player) {
    if (board[row][col] != EMPTY) return 0;
    enum CellState opponent = (player == BLACK) ? WHITE : BLACK;
    for (int dir = 0; dir < 8; dir++) {
        int r = row + dr[dir];
        int c = col + dc[dir];
        int count = 0;
        while (r >= 0 && r < 8 && c >= 0 && c < 8 && board[r][c] == opponent) {
            r += dr[dir];
            c += dc[dir];
            count++;
        }
        if (count > 0 && r >= 0 && r < 8 && c >= 0 && c < 8 && board[r][c] == player)
            return 1;
    }
    return 0;
}

void place_piece(int row, int col, enum CellState player) {
    board[row][col] = player;
    enum CellState opponent = (player == BLACK) ? WHITE : BLACK;

    // ?????????
    for (int dir = 0; dir < 8; dir++) {
        int r = row + dr[dir];
        int c = col + dc[dir];
        int count = 0;

        // ?????????,????????????
        while (r >= 0 && r < 8 && c >= 0 && c < 8 && board[r][c] == opponent) {
            r += dr[dir];
            c += dc[dir];
            count++;
        }
        // ??????????????????,????
        if (count > 0 && r >= 0 && r < 8 && c >= 0 && c < 8 && board[r][c] == player) {
            // ??????????????????
            int rr = row + dr[dir];
            int cc = col + dc[dir];
            while (rr != r || cc != c) {
                board[rr][cc] = player;
                rr += dr[dir];
                cc += dc[dir];
            }
        }
    }
}



void try_place_and_flip(int row, int col, enum CellState player) {
    if (is_valid_move(row, col, player)) {
        place_piece(row, col, player);  // ???????????
        refresh_board_display();        // <-- ????
show_piece_count_icon();
			if (is_board_full()) {
    show_final_winner();
    while(1); // ??,??????
}
    }
}

void show_piece_count_icon(void) {
    int black_count = 0, white_count = 0;
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j) {
            if (board[i][j] == BLACK) black_count++;
            if (board[i][j] == WHITE) white_count++;
        }

    // ???????
    ST7735_DrawFilledRectangle(0, 8 * CELL_SIZE + 2, 128, 18, COLOR_LIGHT_BROWN);

    // ?????
    int black_icon_x = 10;
    int icon_y = 8 * CELL_SIZE + 10;
    int radius = 7;
    ST7735_DrawFilledCircle(black_icon_x, icon_y, radius, ST7735_BLACK);

    // ?????
    int white_icon_x = 70;
    ST7735_DrawFilledCircle(white_icon_x, icon_y, radius, ST7735_WHITE);

    // ??????
    char bstr[8];
    sprintf(bstr, ":%d", black_count);
    ST7735_DrawString(black_icon_x + radius + 2, icon_y - 9, bstr, ST7735_BLACK, COLOR_LIGHT_BROWN, &Font_11x18);

    // ??????
    char wstr[8];
    sprintf(wstr, ":%d", white_count);
    ST7735_DrawString(white_icon_x + radius + 2, icon_y - 9, wstr, ST7735_BLACK, COLOR_LIGHT_BROWN, &Font_11x18);
}


void refresh_board_display(void) {
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            draw_circle_piece(i, j, board[i][j]);
        }
    }
show_piece_count_icon();
	}

	bool is_board_full(void) {
    int count = 0;
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
            if (board[i][j] != EMPTY)
                count++;
    return count == 64;
}

void show_final_winner(void) {
    int black_count = 0, white_count = 0;
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j) {
            if (board[i][j] == BLACK) black_count++;
            if (board[i][j] == WHITE) white_count++;
        }

    // ?????
    ST7735_FillScreen(ST7735_BLUE);

    char msg[32];
    if (black_count > white_count)
        sprintf(msg, "Black Wins");
    else if (white_count > black_count)
        sprintf(msg, "White Wins");
    else
        sprintf(msg, "Draw!");

    // ????
    ST7735_DrawString(10, 70, msg, ST7735_WHITE, ST7735_BLUE, &Font_11x18);
}


/*void draw_cursor() {
    int x = BOARD_ORIGIN_X + cursor_col * CELL_SIZE;
    int y = BOARD_ORIGIN_Y + cursor_row * CELL_SIZE;
    // ???
    ST7735_DrawRectangle(x+1, y+1, CELL_SIZE-2, CELL_SIZE-2, ST7735_RED);
}*/
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
ST7735_DrawFilledCircle(64, 64, 10, ST7735_RED);
  }
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
