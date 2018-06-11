/*	Arduino timed lock box.
*
*	by William Taylor https://github.com/WMTaylor3/Timed-Lock-Box
*
*/


////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////Declarations and Variables/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


//Additional Libraries
#include <EEPROM.h>
#include <DS3231.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Servo.h>
#include <String.h>
#include <Arduino.h> 

//Definitions for hardware buttons.
#define checkButton 12	// Main button for checking remaining time
#define upButton 9		// Button for cycling up menus
#define downButton 10	// Button for cycling down menus
#define nextButton 11	// Button for confirming selection and moving to next item

//Variable to represent LCD connection.
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

//Variables relating to timekeeping aspect of the program.
DS3231 clock;
int endDate[6];		// Array containing end date
int remaining[6];	// Array containing remaining time
					/* Array is as follows:
					* 0 = Year
					* 1 = Month
					* 2 = Date
					* 3 = Hour
					* 4 = Minute
					* 5 = Second
					*/
bool h12, PM, centuryTick;	// h12 and PM are used in 12 hour time keeping. They are not used in this project but are required by various functions in the library, likewise with centuryTick

//Variable representing the state of box
bool locked;


////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////Default functions Setup and Loop//////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


/*
*	Setup function of the program. Runs once when the device is booted.
*	No inputs or outputs.
*/
void setup()
{
	//Set pinModes for the above definitions.
	pinMode(checkButton, INPUT);
	pinMode(upButton, INPUT);
	pinMode(downButton, INPUT);
	pinMode(nextButton, INPUT);

	//Perform initial LCD setup.
	lcd.begin(16, 2);	//LCD is 16 spcaes wide by two lines tall.
	lcd.off();	//Turn LCD off until it is needed.

	//Perform initial setup of real time clock.
	clock.setClockMode(false);

	//Determine whether the device is locked or unlocked by reading value from the EEPROM. If it is locked, read the unlock date from the EEPROM and store it in the above global variables.
	locked = EEPROM.read(0);
	if (locked) {
		ReadDateTimeFromEEPROM();
	}
}

/*
*	Main function of the program. Runs as a loop until device is powered down.
*	No inputs or outputs.
*/
void loop()
{
	//Do nothing until the user presses the check screen button, bringing it to +5 volts.
	if (digitalRead(checkButton) == HIGH)
	{
		//Activate the LCD.
		lcd.on();
		//Check if the user is wanting to enter the backdoor code by pressing the appropriate button combination. If so, call the backdoor function.
		if ((digitalRead(upButton) == HIGH) && (digitalRead(downButton) == LOW) && (digitalRead(nextButton) == HIGH))
		{
			BackDoor();
		}
		//If the device is currently unlocked, prompt the user to enter the end date.
		if (!locked)
		{
			SelectDateTime();
		}
		//Calculate the time remaining until the box is due to unlock, display that time. Then turn off the LCD.
		CalculateRemainingTime();
		BlankDisplay();
		lcd.off();
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

/*
*	Function to prompt the user to input date for the box to unlock.
*	Takes no input parameters, returns no value.
*	Operates by calling the SetUnit function for each of the date units (Day, Month, Year) etc in turn.
*	Essentially this function serves simply to call another function with the appropritate input values.
*/
void SelectDateTime() {
	setEndDateToNow();	//Set the end date/time to this exact moment, this will be used as a starting point to have each unit (day, month, year etc) increased or decreased from.
	SetUnit(0, "Year", 9999, 0);	//Set the year unit. Maximum possible year is 9999AD and minimum possible year is 0AD.
	delay(500);
	SetUnit(1, "Month", 12, 1);		//Set the month unit. Will only accept valid months (i.e. between Jan and Dec).
	delay(500);
	SetUnit(2, "Day", DaysInMonth(endDate[1]), 1);	//Set the day unit. Will only accept a maximum of the number of days in that month, determined using the DaysInMonth function.
	delay(500);
	SetUnit(3, "Hour", 23, 0);		//Sets the hours unit. Will only accept 0000 hours to 2300. Values between 2300 to 2359 are handled by the minutes value.
	delay(500);
	SetUnit(4, "Minute", 59, 0);	//Sets the minutes unit. Will only accept between 0 and 59. As above, further values are handled using the seconds value.
	delay(500);
	SetUnit(5, "Second", 59, 0);	//Sets the seconds value. Any value accepted between 0 and 59.
	BlankDisplay();
	lcd.setCursor(0, 0);	//Blanks the display.
	StoreDateTimeToEEPROM();	//Stores the new date and time to the EEPROM.
	Lock(true);		//Lock the unit.
}

/*
*	Function to set the end date to this exact moment.
*	Called only once, from the above function. Placed in its own function for the sake of modularity.
*	Takes no input parameters, returns no output.
*/
void setEndDateToNow()
{
	endDate[0] = clock.getYear() + 2000;
	endDate[1] = clock.getMonth(centuryTick);
	endDate[2] = clock.getDate();
	endDate[3] = clock.getHour(h12, PM);
	endDate[4] = clock.getMinute();
	endDate[5] = clock.getSecond();
}

/*
*	Function for setting the date and time on the unit.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////Pick up from here.
*/
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

/*
*	Function for storing the time values to the Arduinos EEPROM where they will survive power cycles.
*	Takes no input parameters.
*/
void StoreDateTimeToEEPROM()
{
	EEPROM.write(10, endDate[0] - 2000); //2000 value subtracted so that a stored value of, for example, 2012 becomes 12. Purpose is to save space in the EEPROM.
	EEPROM.write(20, endDate[1]);
	EEPROM.write(30, endDate[2]);
	EEPROM.write(40, endDate[3]);
	EEPROM.write(50, endDate[4]);
	EEPROM.write(60, endDate[5]);
}

/*
*	Reads new values from EEPROM and into the global variables.
*	Takes no input parameters.
*	Is called once upon the device powering up.
*/
void ReadDateTimeFromEEPROM()
{
	endDate[0] = EEPROM.read(10) + 2000; //2000 value added so that a stored value of, for example, 12 becomes 2012. Purpose is to save space in the EEPROM.
	endDate[1] = EEPROM.read(20);
	endDate[2] = EEPROM.read(30);
	endDate[3] = EEPROM.read(40);
	endDate[4] = EEPROM.read(50);
	endDate[5] = EEPROM.read(60);
}

/*
*	Determines whether the month in question is a 31, 30, 29 or 28 day month.
*	Takes in the month value (i.e 1 = Jan, 2 = Feb etc). Return the number of days in that month.
*/
int DaysInMonth(byte month)
{
	//If the month in question always has 31 days, return 31.
	if ((month == 1) || (month == 3) || (month == 5) || (month == 7) || (month == 8) || (month == 10) || (month == 12)) { return 31; }
	else
		//If the month in question always has 30 days, return 30.
		if ((month == 4) || (month == 6) || (month == 9) || (month == 11)) { return 30; }
		else
			//If the month in question is Feb and it is a leap year, return 29.
			if (((month % 4 == 0) && (month % 100 != 0)) || month % 400 == 0) { return 29; }
			//If the month in question is Feb and it is not a leap year, return 28.
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
	Servo servo;

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