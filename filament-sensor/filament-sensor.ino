#include <LiquidCrystal_I2C.h>
#include <string.h>
#include <SPI.h>
#include <MFRC522.h>

LiquidCrystal_I2C lcd(0x27,16,2);
void subString(char* destination, const char* source, int start, int length){
  strncpy(destination, source + start, length);
  destination[length] = '\0';
}

class MenuPoint{
  private:
    int currentIndex = 0;
    int iterations = 0;
    int scrollPause;
    unsigned long selectTime = 0;
    String fullText;
    char shownText[17];
    bool shiftRight = true;
    bool selected = false;
  public:
    MenuPoint(String text, bool s, int pause=5000){
      fullText = text;
      shiftText(0);
      selected = s;
      scrollPause = pause;
    }
    void setText(String text){
      fullText = text;
      fullText.trim();
    }
    void setSelected(bool s){
      selected = s;
      selectTime = millis();
    }
    void shiftText(int startIndex){
      if (startIndex > fullText.length() - 1){ // bound the Index to the lenght of the String
        startIndex = fullText.length() - 1;
      }
      subString(shownText, fullText.c_str(), startIndex, 16);
    }
    void show(int row){
      lcd.setCursor(0, row);
      if (selected && fullText.length() > 16 && iterations * (long)750 + scrollPause < millis() - selectTime){
        shiftText(currentIndex);
        iterations++;
        if (currentIndex == fullText.length() - 16){
          shiftRight = false;
          iterations = 0; // this is so the Scrolling stops at the end
          selectTime = millis();
        } else if (currentIndex == 0) {
          shiftRight = true;
          iterations = 0; // do this so the formula "iterations * 750" can't reach the maximum amount for longs so the if-block continues executing
          selectTime = millis();
        }
        shiftRight ? currentIndex++ : currentIndex--;
      } else if (!selected){
        shiftText(0);
      }
      // Serial.println("I should show the Text");
      lcd.print(shownText);
    }
    void debug(){
      Serial.println(currentIndex);
      Serial.println(iterations);
      Serial.println(selectTime);
      Serial.println(fullText);
      Serial.println(fullText.c_str());
      Serial.println(fullText.length());
      Serial.println(shownText);
      Serial.println(shiftRight);
      Serial.println(selected);
      Serial.println("------------------");
    }
};

MenuPoint point("Hello, World! This is a test", true);
unsigned long waitTime = 0;

void setup() {
  // put your setup code here, to run once:
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available()){
    point.setText(Serial.readString());
  }
  point.show(0);
  /*if (millis() - waitTime > 750){
    point.debug();
    waitTime = millis();
  }*/
}
