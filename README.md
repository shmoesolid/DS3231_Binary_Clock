# DS3231_Binary_Clock

Binary Clock v0.2 originally written by Gabriel J. Pensky.
( Web: https://www.instructables.com/id/Beautiful-Arduino-Binary-Clock/ )

Converted by Shane B. Koehler to use DS3231 with Rinky-Dink Electronics (Henning Karlsen) library v1.01
( Web: http://www.rinkydinkelectronics.com/library.php?id=73 )

ChangeLog:

	v0.2.s.2
 	  -changed the way the LEDs are lit for a stable constant refresh rate, this
  	   fixes the LED flicker/dimming so they all remain constant bright. It
  	   also lowers voltage use from previous and should increase LED lifespan.
  	   (default is 1ms loop iteration)

	v0.2.s.1
 	  -intial conversion

Don't forget to include the DS3231 zip library in Arduino IDE under menu "Sketch/Include Library/Add .ZIP Library..."

Manually set time in setup() method.

Does not use DLS or 12 hour format currently, but has easy hour/minute change buttons.

24h easy mode: 
	
	if hours over 12, subtract by 2, if first digit is a 1, remove it, if first digit is a 2, make it a 1
		
	ie: 15 - 2 = 13, first digit 1 so remove, hour is 3
	ie: 19 - 2 = 17, first digit 1 so remove, hour is 7
	ie: 22 - 2 = 20, first digit 2 so make 1, hour is 10
	ie: 23 - 2 = 21, first digit 2 so make 1, hour is 11
