/*****************************************************************************
* Product: "Fly 'n' Shoot" game example on EFM32-SLSTK3401A board
* Last updated for version 5.8.1
* Last updated on  2016-12-12
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Alternatively, this program may be distributed and modified under the
* terms of Quantum Leaps commercial licenses, which expressly supersede
* the GNU General Public License and are specifically designed for
* licensees interested in retaining the proprietary status of their code.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <www.gnu.org/licenses/>.
*
* Contact information:
* <www.state-machine.com/licensing>
* <info@state-machine.com>
*****************************************************************************/
#ifndef BSP_H
#define BSP_H

#define BSP_TICKS_PER_SEC    100U
#define BSP_SCREEN_WIDTH     640U
#define BSP_SCREEN_HEIGHT    480U

#define HID_UP_MASK 	0x00
#define HID_DOWN_MASK 	0xFF
#define HID_LEFT_MASK 	0x00
#define HID_RIGHT_MASK 	0xFF
#define HID_A_MASK 		0x2F
#define HID_B_MASK 		0x4F
#define HID_SELECT_MASK	0x10
#define HID_START_MASK 	0x20


void BSP_init(void);
void BSP_terminate(int16_t result);

void BSP_updateScreen(void);
void BSP_clearFB(void);
void BSP_clearWalls(void);
void BSP_paintString(uint16_t x, uint16_t y, char const *str);
void BSP_paintBitmap(uint16_t x, uint16_t y, uint8_t bmp_id);
void BSP_advanceWalls(uint16_t top, uint16_t bottom);
void BSP_updateScore(uint16_t score);

bool BSP_isThrottle(void); /* is the throttle button depressed? */
bool BSP_doBitmapsOverlap(uint8_t bmp_id1, uint16_t x1, uint16_t y1,
                          uint8_t bmp_id2, uint16_t x2, uint16_t y2);
bool BSP_isWallHit(uint8_t bmp_id, uint16_t x, uint16_t y);

void BSP_displayOn(void);
void BSP_displayOff(void);

void BSP_randomSeed(uint32_t seed); /* random seed */
uint32_t BSP_random(void);          /* pseudo-random generator */

extern QActive *the_Ticker0; /* "Ticker" active object for tick rate 0 */

#endif /* BSP_H */
