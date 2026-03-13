/**
 * test_phase2 - Phase 2 显示引擎测试
 * 
 * 测试所有显示引擎组件的编译和功能
 * 
 * @author AI Assistant
 * @version 1.0.0
 */

#include <LEDMatrix.h>
#include <FontRenderer.h>
#include <DisplayEngine.h>

// 测试状态
enum TestState {
    TEST_INIT,
    TEST_FIRE,
    TEST_MATRIX,
    TEST_RIPPLE,
    TEST_LIFE,
    TEST_SAND,
    TEST_PONG,
    TEST_WEATHER,
    TEST_CLOCK,
    TEST_DATE,
    TEST_TEMP,
    TEST_CARD,
    TEST_TRANSITION,
    TEST_MANAGER,
    TEST_COMPLETE
};

TestState currentState = TEST_INIT;
unsigned long stateStartTime = 0;
const unsigned long STATE_DURATION = 3000;  // 每个状态3秒

// 测试对象
DisplayManager* displayManager = nullptr;
Card* currentCard = nullptr;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("Phase 2: Display Engine Test");
    Serial.println("========================================\n");
    
    // 初始化显示管理器
    displayManager = &DisplayManager::getInstance();
    displayManager->begin();
    
    Serial.println("DisplayManager initialized successfully!");
    Serial.println("Starting display engine tests...\n");
    
    stateStartTime = millis();
    currentState = TEST_FIRE;
}

void loop() {
    // 检查是否需要切换测试状态
    if (millis() - stateStartTime >= STATE_DURATION && currentState != TEST_COMPLETE) {
        currentState = (TestState)(currentState + 1);
        stateStartTime = millis();
        
        // 清理旧卡片
        if (currentCard) {
            delete currentCard;
            currentCard = nullptr;
        }
        
        // 创建新测试卡片
        createTestCard();
        
        Serial.print("Switched to test state: ");
        Serial.println(getStateName(currentState));
    }
    
    // 更新和渲染
    if (displayManager) {
        displayManager->update();
        displayManager->render();
    }
    
    // 打印FPS（每秒一次）
    static unsigned long lastFPSPrint = 0;
    if (millis() - lastFPSPrint >= 1000) {
        lastFPSPrint = millis();
        if (displayManager) {
            Serial.print("FPS: ");
            Serial.println(displayManager->getFPS());
        }
    }
    
    // 测试完成
    if (currentState == TEST_COMPLETE) {
        Serial.println("\n========================================");
        Serial.println("All tests completed successfully!");
        Serial.println("========================================");
        delay(5000);
        currentState = TEST_FIRE;  // 循环测试
        stateStartTime = millis();
        createTestCard();
    }
}

void createTestCard() {
    if (!displayManager) return;
    
    // 清理旧卡片
    while (displayManager->getCardCount() > 0) {
        displayManager->removeCard(0);
    }
    
    currentCard = new Card(getStateName(currentState));
    
    // 根据状态创建不同的背景和前景
    switch (currentState) {
        case TEST_FIRE:
            currentCard->setBackground(new FireBackground());
            currentCard->addForeground(new ClockForeground());
            break;
            
        case TEST_MATRIX:
            currentCard->setBackground(new MatrixRainBackground());
            currentCard->addForeground(new DateForeground());
            break;
            
        case TEST_RIPPLE:
            currentCard->setBackground(new WaterRippleBackground());
            currentCard->addForeground(new TempForeground());
            break;
            
        case TEST_LIFE:
            currentCard->setBackground(new GameOfLifeBackground());
            {
                ClockForeground* clock = new ClockForeground();
                clock->setFormat(ClockForeground::Format::HH_MM_SS);
                currentCard->addForeground(clock);
            }
            break;
            
        case TEST_SAND:
            currentCard->setBackground(new SandBackground());
            {
                TempForeground* temp = new TempForeground();
                temp->setFormat(TempForeground::Format::TEMP_HUMID);
                currentCard->addForeground(temp);
            }
            break;
            
        case TEST_PONG:
            currentCard->setBackground(new PongBackground());
            currentCard->addForeground(new ClockForeground());
            break;
            
        case TEST_WEATHER:
            {
                WeatherBackground* weather = new WeatherBackground();
                weather->setWeather(WeatherBackground::WeatherType::RAINY);
                currentCard->setBackground(weather);
                currentCard->addForeground(new DateForeground());
            }
            break;
            
        case TEST_CLOCK:
            {
                MatrixRainBackground* matrix = new MatrixRainBackground();
                matrix->setForegroundArea(10, 2, 22, 7);
                currentCard->setBackground(matrix);
                ClockForeground* clock = new ClockForeground();
                clock->setPosition(Position::CENTER);
                currentCard->addForeground(clock);
            }
            break;
            
        case TEST_DATE:
            currentCard->setBackground(new WaterRippleBackground());
            {
                DateForeground* date = new DateForeground();
                date->setFormat(DateForeground::Format::MM_DD_WEEK);
                date->setPosition(Position::BOTTOM_CENTER);
                currentCard->addForeground(date);
            }
            break;
            
        case TEST_TEMP:
            currentCard->setBackground(new FireBackground());
            {
                TempForeground* temp = new TempForeground();
                temp->setTemperature(28.5f);
                temp->setHumidity(65.0f);
                temp->setPosition(Position::TOP_RIGHT);
                currentCard->addForeground(temp);
            }
            break;
            
        case TEST_CARD:
            {
                // 测试多前景自动切换
                currentCard->setBackground(new MatrixRainBackground());
                currentCard->setAutoSwitch(true);
                
                ClockForeground* clock = new ClockForeground();
                clock->setDisplayDuration(2);
                currentCard->addForeground(clock);
                
                DateForeground* date = new DateForeground();
                date->setDisplayDuration(2);
                currentCard->addForeground(date);
                
                TempForeground* temp = new TempForeground();
                temp->setDisplayDuration(2);
                currentCard->addForeground(temp);
            }
            break;
            
        case TEST_TRANSITION:
            // 测试转场效果
            currentCard->setBackground(new FireBackground());
            displayManager->setTransitionType(TransitionType::FADE);
            displayManager->setTransitionDuration(1000);
            currentCard->addForeground(new ClockForeground());
            break;
            
        case TEST_MANAGER:
            // 测试管理器功能
            {
                // 创建多个卡片并启用自动切换
                for (int i = 0; i < 3; i++) {
                    Card* card = new Card("Auto Card");
                    switch (i % 3) {
                        case 0: card->setBackground(new MatrixRainBackground()); break;
                        case 1: card->setBackground(new FireBackground()); break;
                        case 2: card->setBackground(new WaterRippleBackground()); break;
                    }
                    card->addForeground(new ClockForeground());
                    displayManager->addCard(card);
                }
                displayManager->setAutoSwitch(true);
                displayManager->setSwitchInterval(3);
                currentCard = nullptr;  // 卡片已添加到管理器
            }
            break;
            
        default:
            break;
    }
    
    // 添加当前卡片到管理器（如果不是TEST_MANAGER）
    if (currentCard && currentState != TEST_MANAGER) {
        displayManager->addCard(currentCard);
    }
}

const char* getStateName(TestState state) {
    switch (state) {
        case TEST_INIT:      return "INIT";
        case TEST_FIRE:      return "FIRE";
        case TEST_MATRIX:    return "MATRIX";
        case TEST_RIPPLE:    return "RIPPLE";
        case TEST_LIFE:      return "LIFE";
        case TEST_SAND:      return "SAND";
        case TEST_PONG:      return "PONG";
        case TEST_WEATHER:   return "WEATHER";
        case TEST_CLOCK:     return "CLOCK";
        case TEST_DATE:      return "DATE";
        case TEST_TEMP:      return "TEMP";
        case TEST_CARD:      return "CARD";
        case TEST_TRANSITION:return "TRANSITION";
        case TEST_MANAGER:   return "MANAGER";
        case TEST_COMPLETE:  return "COMPLETE";
        default:             return "UNKNOWN";
    }
}
