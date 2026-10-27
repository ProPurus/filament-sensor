#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <vector>

using std::vector;

LiquidCrystal_I2C lcd(0x27, 16, 2);
const int PinCLK = 18;
const int PinDT = 19;
const int PinSW = 4;

const int rfidResetPin = 22;
const MFRC522 rfidReaders[] = {MFRC522(13, rfidResetPin), MFRC522(14, rfidResetPin), MFRC522(16, rfidResetPin), MFRC522(17, rfidResetPin)};

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
    if (!digitalRead(PinSW) && chars[index] != '@'){
      result += chars[index];
      delay(250);
    } else if (!digitalRead(PinSW) && chars[index] == '@'){
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
  protected:
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
      lcd.print("                ");
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
};


class Filament : public MenuPoint {
  public:
    int filamentNumber;
    int turnsPerLayer;
    int fullSpoolWeight;
    double remainingFilamentPercentage;
    int rotationCount = 0;
    unsigned long lastDetectedAt = 0;
    double spoolRadius = 100;
    double filamentDiameter = 1.75;
    double filamentDensity = 0.00124; // PLA density in g/mm^3

    Filament(
      String text,
      bool selected = false,
      int pause = 5000,
      int number = 0,
      int turns = 28,
      int weight = 1000,
      double remainingPercentage = 100.0,
      double radius = 90
    ) : MenuPoint(text, selected, pause) {
      filamentNumber = number;
      turnsPerLayer = turns;
      fullSpoolWeight = weight;
      remainingFilamentPercentage = remainingPercentage;
      spoolRadius = radius;
    }

    /*
    bool checkRotation(int waitTime) {
      // Checks whether the spool's RFID tag is present at the corresponding reader.
      if (millis() - lastDetectedAt < waitTime) {
        return false;
      }

      if (rfidReaders[filamentNumber - 1].PICC_IsNewCardPresent() &&
          rfidReaders[filamentNumber - 1].PICC_ReadCardSerial()) {
        rfidReaders[filamentNumber - 1].PICC_HaltA();
        lastDetectedAt = millis();
        return true;
      }

      return false;
    }
    */

    /**
     * Calculates the filament mass consumed by one spool rotation
     * and updates the remaining filament percentage.
     *
     * @param calculateOnly If true, returns the fraction of the full spool
     *                      weight consumed without updating the percentage.
     * @return The consumed mass as a fraction of the full spool weight
     *         when calculateOnly is true.
     */
    double consumeOneRotation(bool calculateOnly = false) {
      double currentFilamentRadius = spoolRadius - (int(rotationCount / turnsPerLayer) * filamentDiameter); // Distance from the spool center to the outermost filament layer.
      double filamentLength = 2 * PI * currentFilamentRadius; // Length of filament unwound in one rotation.
      double filamentMass = PI * sq(filamentDiameter / 2) * filamentLength * filamentDensity; // Mass of the filament consumed during one rotation.

      rotationCount++;
      double consumedFraction = filamentMass / fullSpoolWeight;
      
      if (calculateOnly) {
        return consumedFraction;
      }
      remainingFilamentPercentage -= consumedFraction;
      return consumedFraction;
    }

    /*
     * Reconstructs the rotation count based on the remaining filament
     * percentage by simulating rotations from a full spool.
     */
    void calculateRotationCount() {
      double simulatedRemainingPercentage = 100.0;
      rotationCount = 0;
      while (simulatedRemainingPercentage >= remainingFilamentPercentage) {
        simulatedRemainingPercentage -= consumeOneRotation(true);
      }
    }
};


template <typename T>
class Menu{
  private:
    vector<vector<T>> menus;
    int currentLocation[2] = {0, 0};
  public:
    Menu(vector<vector<T>> subMenus){
      for (vector<T> subMenu : subMenus){
        menus.push_back(subMenu);
      }
      // menus.push_back({T("Add Filament", true)});
      // menus.push_back({T("Change Name"), T("Change Percentage"), T("Change Base Value"), T("Swap Filaments"), T("Return")});
    }

    bool isPressed(){
      return !digitalRead(PinSW);
    }

    void addFilament(){
      T temp = menus.at(0).at(menus.at(0).size() - 1);
      menus.at(0).pop_back();
      menus.at(0).push_back(T(stringInput()));
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
        TurnDetected = false;
      }
      // press inputs
      if (isPressed()){
        delay(200);
        menus.at(currentLocation[0]).at(currentLocation[1]).onPress();
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

Menu<MenuPoint> menu({{MenuPoint("Add Filament", true)},
                      {MenuPoint("Change Name"), MenuPoint("Change Percentage"), MenuPoint("Change Base Value"), MenuPoint("Swap Filaments"), MenuPoint("Return")}
                      });

void setup() {
  // put your setup code here, to run once:
  pinMode(PinDT, INPUT_PULLUP);
  pinMode(PinCLK, INPUT_PULLUP);
  pinMode(PinSW, INPUT_PULLUP);
  lcd.init();
  lcd.backlight();
  Serial.begin(921600);
  attachInterrupt(digitalPinToInterrupt(PinCLK), isr, CHANGE);
}

void loop() {
  // put your main code here, to run repeatedly:
  menu.update();
}
