/**
 * DisplayEngine - 显示引擎总括头文件
 * 
 * 包含所有显示引擎组件
 * 
 * @author AI Assistant
 * @version 1.0.0
 */

#ifndef DISPLAY_ENGINE_H
#define DISPLAY_ENGINE_H

// 核心组件
#include "Core/DisplayManager.h"
#include "Core/Card.h"
#include "Core/Transition.h"

// 背景基类
#include "Backgrounds/Background.h"

// 背景效果
#include "Backgrounds/FireBackground.h"
#include "Backgrounds/MatrixRainBackground.h"
#include "Backgrounds/WaterRippleBackground.h"
#include "Backgrounds/GameOfLifeBackground.h"
#include "Backgrounds/SandBackground.h"
#include "Backgrounds/PongBackground.h"
#include "Backgrounds/WeatherBackground.h"

// 前景基类
#include "Foregrounds/Foreground.h"

// 前景效果
#include "Foregrounds/ClockForeground.h"
#include "Foregrounds/DateForeground.h"
#include "Foregrounds/TempForeground.h"

#endif // DISPLAY_ENGINE_H
