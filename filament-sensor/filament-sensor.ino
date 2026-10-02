#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <vector>

using std::vector;

LiquidCrystal_I2C lcd(0x27,16,2);
const int PinCLK = 2;
const int PinDT = 3;
const int PinSW = 4;

volatile bool TurnDetected = false;
volatile bool rotationdirection = false;

const char chars[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', ' ',
                  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
                  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '!', '?', '+', '-', '*', '/', '(', ')', '@'};

void isr(){
  if (digitalRead(PinCLK)){
    rotationdirection = digitalRead(PinDT);
  } else {
    rotationdirection = !digitalRead(PinDT);
  }
  TurnDetected = true;
}

String stringInput(){
  String result = "";
  int index = 0;
  String displayString;
  while(true){
    if (TurnDetected){
      if (!rotationdirection && index < 71){
        index++;
        delay(10);
      } else if (rotationdirection && index > 0){
        index--;
        delay(10);
      }
      TurnDetected = false;
    }
    if (result.length() < 16){
      displayString = result;
    } else {
      displayString = result.substring(result.length() - 15);
    }
    lcd.setCursor(0, 1);
    lcd.print("To save select @");
    lcd.setCursor(0, 0);
    lcd.print(displayString);
    lcd.print(chars[index]);
    if (digitalRead(PinSW) && chars[index] != '@'){
      result += chars[index];
      delay(250);
    } else if (digitalRead(PinSW) && chars[index] == '@'){
      delay(250);
      return result;
    }
  }
}

int intInput(String infotext, bool percentage=false, int startvalue=0){
  lcd.clear();
  delay(200);
  int result = startvalue;
  lcd.setCursor(0, 0);
  lcd.print(infotext);
  while(true){
    if (TurnDetected){
      if (rotationdirection){
        result++;
      } else if (!rotationdirection && result > 0){
        result--;
      }
    TurnDetected = false;
    }
    if (percentage && result > 100){
      result = 100;
    }
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(result);
    if (!digitalRead(PinSW) && ((percentage && result <= 100) || !percentage)){
      lcd.setCursor(0, 0);
      return result;
    }
  }
}

class MenuPoint{
  private:
    int currentIndex = 0;
    int iterations = 0;
    int scrollPause;
    unsigned long selectTime = 0;
    String fullText;
    String shownText;
    bool shiftRight = true;
    bool selected = false;
  public:
    MenuPoint(String text="Menupoint", bool s=false, int pause=5000){
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

    /*
     * Sets the display text to the portion of the full text
     * starting at the startindex (the character at startIndex
     * is the leftmost character on the display).
    */
    void shiftText(int startIndex){
      if (startIndex > fullText.length() - 1){ // bind the Index to the lenght of the String
        startIndex = fullText.length() - 1;
      }
      shownText = fullText.substring(startIndex);
    }

    /* Virtual method available to add functionality in childclasses */
    virtual void update(){}
    virtual void onPress(){}

    /*
     * Function for updating a menupoint. If enough time for a scroll
     * has passed, it will scroll the Text on the next call.
     * Non-blocking - should be called in a loop.
     */
    void show(int row){
      lcd.setCursor(0, row);
      if (selected && fullText.length() > 16 && iterations * (long)750 + scrollPause < millis() - selectTime){
        shiftText(currentIndex);
        iterations++;
        if (currentIndex == fullText.length() - 16){
          shiftRight = false;
          iterations = 0; // this is so the Scrolling stops at the end (see down below)
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
      lcd.print(shownText);
    }
    /*void debug(){
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
    }*/
};

template <typename T>
class Menu{
  private:
    vector<vector<T>> menus;
    int currentLocation[2] = {0, 0};
  public:
    Menu(){
      menus.push_back({T("Add Filament", true)});
      menus.push_back({T("Change Name"), T("Change Percentage"), T("Change Base Value"), T("Swap Filaments"), T("Return")});
    }

    bool isPressed(){
      return !digitalRead(PinSW);
    }

    void addFilament(){
      T temp = menus.at(0).at(menus.at(0).size() - 1);
      menus.at(0).pop_back();
      menus.at(0).push_back(MenuPoint(stringInput()));
      menus.at(0).push_back(temp);
    }

    void resetSelectedPoints(){
      for (vector<T>& submenu : menus){
        for (T& menuPoint : submenu){
          menuPoint.setSelected(false);
        }
      }
    }

    /* Registers any movement of the rotary encoder
       and acts accordingly*/
    void registerInputs(){
      // rotation inputs
      if (TurnDetected){
        if (rotationdirection && currentLocation[1] < menus.at(currentLocation[0]).size() - 1){
          currentLocation[1]++;
        } else if (!rotationdirection && currentLocation[1] > 0){
          currentLocation[1]--;
        }
      }
      // press inputs
      if (isPressed()){
        delay(200);
        for (vector<T>& submenu : menus){
          for (T& menuPoint : submenu){
            menuPoint.onPress();
          }
        }
      }
      // update which menupoint is selected
      resetSelectedPoints();
      menus.at(currentLocation[0]).at(currentLocation[1]).setSelected(true);
    }

    /* Renders the correct menupoints on the display*/
    void show(){
      menus.at(currentLocation[0]).at(currentLocation[1]).show(0);
      if (menus.at(currentLocation[0]).size() - 1 >= currentLocation[1] + 1){
        menus.at(currentLocation[0]).at(currentLocation[1] + 1).show(1);
      }
    }

    /* Function to be called in the loop. Updates the whole Menu*/
    void update(){
      registerInputs();
      for (vector<T>& submenu : menus){
        for (T& menuPoint : submenu){
          menuPoint.update();
        }
      }
      show();
    }
};

void setup() {
  // put your setup code here, to run once:
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(2), isr, CHANGE);
}

void loop() {
  // put your main code here, to run repeatedly:
  menu.update();
}
