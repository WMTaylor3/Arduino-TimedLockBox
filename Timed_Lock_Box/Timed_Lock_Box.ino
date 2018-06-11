//Library includes
#include <EEPROM.h>
#include <DS3231.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Servo.h>
#include <String.h>
#include <Arduino.h> 

// Variables for servo
Servo servo;      // Servo used for locking the box

				  // Variables for user buttons
int checkButton = 12;   // Main button for checking remaining time
int upButton = 9;		// Button for cycling up menus
int downButton = 10;	// Button for cycling down menus
int nextButton = 11;	// Button for confirming selection and moving to next item

						// Variables for LCD
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// Variables for Real Time Clock
DS3231 clock;
int endDate[6];		// Array containing end date
int remaining[6];	// Array containing remaining time
					/* Arrays as follows:
					0 = Year
					1 = Month
					2 = Date
					3 = Hour
					4 = Minute
					5 = Second
					*/
bool h12, PM, centuryTick;	// h12 and PM are used in 12 hour time keeping. They are not used in this project but are required by various functions in the library, likewise with centuryTick

							//State of box
bool locked;

void StoreDateTimeToEEPROM()
{
	EEPROM.write(10, endDate[0] - 2000);
	EEPROM.write(20, endDate[1]);
	EEPROM.write(30, endDate[2]);
	EEPROM.write(40, endDate[3]);
	EEPROM.write(50, endDate[4]);
	EEPROM.write(60, endDate[5]);
}

void ReadDateTimeFromEEPROM()
{
	endDate[0] = EEPROM.read(10) + 2000;
	endDate[1] = EEPROM.read(20);
	endDate[2] = EEPROM.read(30);
	endDate[3] = EEPROM.read(40);
	endDate[4] = EEPROM.read(50);
	endDate[5] = EEPROM.read(60);
}

int DaysInMonth(byte month) {		// The following block determines whether it is a 30, 31, 28 or 29 day month
	if ((month == 1) || (month == 3) || (month == 5) || (month == 7) || (month == 8) || (month == 10) || (month == 12)) { return 31; }
	else
		if ((month == 4) || (month == 6) || (month == 9) || (month == 11)) { return 30; }
		else
			if (((month % 4 == 0) && (month % 100 != 0)) || month % 400 == 0) { return 29; }
			else { return 28; }
}

void CalculateRemainingTime()
{
	remaining[5] = endDate[5] - clock.getSecond();				// Calculates remaining seconds
	remaining[4] = endDate[4] - clock.getMinute();				// Calculates remaining minutes
	if (remaining[5] < 0)
	{
		remaining[4]--;
		remaining[5] += 60;
	}

	remaining[3] = endDate[3] - clock.getHour(h12, PM);			// Calculates remaining hours
	if (remaining[4] < 0)
	{
		remaining[3]--;
		remaining[4] += 60;
	}

	remaining[2] = endDate[2] - clock.getDate();				// Calculates remaining days
	if (remaining[3] < 0)
	{
		remaining[2]--;
		remaining[3] += 24;
	}

	remaining[1] = endDate[1] - clock.getMonth(centuryTick);	// Calculates remaining months
	if (remaining[2] < 0)
	{
		remaining[1]--;
		remaining[2] += DaysInMonth(clock.getMonth(centuryTick));
	}

	remaining[0] = endDate[0] - (clock.getYear() + 2000);		// Calculates remaining years
	if (remaining[1] < 0)
	{
		remaining[0]--;
		remaining[1] += 12;
	}

	if (remaining[0] < 0)
	{
		ZeroHour();
	}
	else
	{
		PrintRemainingTime();
	}
}

void PrintRemainingTime()
{
	BlankDisplay();

	lcd.setCursor(0, 0);
	lcd.print(String(remaining[0]) + " Years");
	lcd.setCursor(0, 1);
	lcd.print(String(remaining[1]) + " Months");
	delay(3000);
	BlankDisplay();

	lcd.setCursor(0, 0);
	lcd.print(String(remaining[2]) + " Days");
	lcd.setCursor(0, 1);
	lcd.print(String(remaining[3]) + " Hours");
	delay(3000);
	BlankDisplay();

	lcd.setCursor(0, 0);
	lcd.print(String(remaining[4]) + " Minutes");
	lcd.setCursor(0, 1);
	lcd.print(String(remaining[5]) + " Seconds");
	delay(4000);
	BlankDisplay();
}

void ZeroHour()
{
	lcd.setCursor(0, 0);
	lcd.print("Date Reached!");
	lcd.setCursor(0, 1);
	lcd.print("Welcome!");
	delay(1500);
	Lock(false);
}

void BackDoor()
{
	String enteredCode = "";
	int count = 0;
	lcd.setCursor(0, 0);
	lcd.print("Enter Code:");
	delay(1000);
	while (count < 10)
	{
		lcd.setCursor(count, 1);
		if ((digitalRead(upButton) == HIGH) && (digitalRead(downButton) == LOW) && (digitalRead(nextButton) == LOW))
		{
			enteredCode += 'U';
			lcd.print('*');
			count++;
			delay(500);
		}
		if ((digitalRead(upButton) == LOW) && (digitalRead(downButton) == HIGH) && (digitalRead(nextButton) == LOW))
		{
			enteredCode += 'D';
			lcd.print('*');
			count++;
			delay(500);
		}
		if ((digitalRead(upButton) == LOW) && (digitalRead(downButton) == LOW) && (digitalRead(nextButton) == HIGH))
		{
			enteredCode += 'N';
			lcd.print('*');
			count++;
			delay(500);
		}
	}
	BlankDisplay();
	lcd.setCursor(0, 0);
	if (enteredCode == "UUNDNNDUND")
	{
		lcd.print("Accepted");
		lcd.setCursor(0, 1);
		lcd.print("Welcome William");
		delay(3000);
		Lock(false);
	}
	else
	{
		lcd.print("Access Denied");
		delay(3000);
	}
}

void BlankDisplay()
{
	lcd.setCursor(0, 0);
	lcd.print("                ");
	lcd.setCursor(0, 1);
	lcd.print("                ");
}

void SelectDateTime() {
	setEndDateToNow();
	SetUnit(0, "Year", 9999, 0);
	delay(500);
	SetUnit(1, "Month", 12, 1);
	delay(500);
	SetUnit(2, "Day", DaysInMonth(endDate[1]), 1);
	delay(500);
	SetUnit(3, "Hour", 23, 0);
	delay(500);
	SetUnit(4, "Minute", 60, 0);
	delay(500);
	SetUnit(5, "Second", 60, 0);
	BlankDisplay();
	lcd.setCursor(0, 0);
	StoreDateTimeToEEPROM();
	Lock(true);
}

void SetUnit(int selection, String unitName, int maximum, int minimum)
{
	BlankDisplay();
	lcd.setCursor(0, 0);
	lcd.print(String("Set " + unitName + ":"));
	lcd.setCursor(0, 1);
	lcd.print(String(endDate[selection]));
	while (true) {
		if ((digitalRead(upButton) == HIGH) && (digitalRead(downButton) == LOW) && (digitalRead(nextButton) == LOW))
		{
			endDate[selection]++;
			if (endDate[selection] > maximum)
			{
				endDate[selection] = minimum;
			}
			BlankDisplay();
			lcd.setCursor(0, 0);
			lcd.print(String("Set " + unitName + ":"));
			lcd.setCursor(0, 1);
			lcd.print(String(endDate[selection]));
			delay(150);
		}
		else if ((digitalRead(upButton) == LOW) && (digitalRead(downButton) == HIGH) && (digitalRead(nextButton) == LOW))
		{
			endDate[selection]--;
			if (endDate[selection] < minimum)
			{
				endDate[selection] = maximum;
			}
			BlankDisplay();
			lcd.setCursor(0, 0);
			lcd.print(String("Set " + unitName + ":"));
			lcd.setCursor(0, 1);
			lcd.print(String(endDate[selection]));
			delay(150);
		}
		else if ((digitalRead(upButton) == LOW) && (digitalRead(downButton) == LOW) && (digitalRead(nextButton) == HIGH))
		{
			break;
		}
	}
}

void setEndDateToNow()
{
	endDate[0] = clock.getYear() + 2000;
	endDate[1] = clock.getMonth(centuryTick);
	endDate[2] = clock.getDate();
	endDate[3] = clock.getHour(h12, PM);
	endDate[4] = clock.getMinute();
	endDate[5] = clock.getSecond();
}

void WriteCurrentDateTime(byte year, byte month, byte date, byte hour, byte minute, byte second)
{
	clock.setYear(year - 2000);
	clock.setMonth(month);
	clock.setDate(date);
	clock.setHour(hour);
	clock.setMinute(minute);
	clock.setSecond(second);
}

void Lock(bool lock)
{
	BlankDisplay();
	lcd.setCursor(0, 0);
	servo.attach(8);
	delay(200);
	if (lock)
	{
		lcd.print("Locking...");
		servo.write(0);
		locked = true;
		EEPROM.write(0, 1);
	}
	else
	{
		lcd.print("Unlocking...");
		servo.write(110);
		locked = false;
		EEPROM.write(0, 0);
	}
	delay(3000);
	servo.detach();
}

void setup()
{
	// User input
	pinMode(checkButton, INPUT);
	pinMode(upButton, INPUT);
	pinMode(downButton, INPUT);
	pinMode(nextButton, INPUT);

	// LCD
	lcd.begin(16, 2);
	lcd.off();

	// Real Time Clock
	clock.setClockMode(false);
	//WriteCurrentDateTime(2018, 3, 1, 19, 13, 0);      // Sets current date and time using values provided to function

	// Read from EEPROM
	locked = EEPROM.read(0); // Whether device is locked or not.
	if (locked) {
		ReadDateTimeFromEEPROM();
	}
}

void loop()
{
	if (digitalRead(checkButton) == HIGH)
	{
		lcd.on();
		if ((digitalRead(upButton) == HIGH) && (digitalRead(downButton) == LOW) && (digitalRead(nextButton) == HIGH))
		{
			BackDoor();
		}
		if (!locked)
		{
			SelectDateTime();
		}
		CalculateRemainingTime();
		BlankDisplay();
		lcd.off();
	}
}
