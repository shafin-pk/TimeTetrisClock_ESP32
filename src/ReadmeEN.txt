/*****************************************************************************
* | File      	:   Readme_EN.txt
* | Author      :   
* | Function    :   Help with use
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2022-08-29
* | Info        :   Here is an English version of the documentation for your quick use.
******************************************************************************/
This file is to help you use this routine.
Here is a brief description of the use of this project:

1. Basic information:
This routine is verified using RGB-Matrix-P3-64x32. 
You can view the corresponding test routine in the project;

2. Pin connection:

R1  -> GP13
G1  -> R2(OUT)
B1  -> G2(OUT)
R2  -> R1(OUT)
G2  -> G1(OUT)
B2  -> B1(OUT)
A    ->  GP19
B    ->  GP23
C    ->  GP18
D    ->  GP5
E     ->  GP15
CLK ->  GP14	  
STB/LAT->  GP22
OE   ->  GP0


3. Basic use:
    1): Copy all the files in libraries to libraries in the installation path of Arduino IDE
        
    2): Open the project EzTimeTetrisClockESP32.ino with Arduino IDE
    
    3): Click File -> Additional Development Board Manager Address -> Enter:
"https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json"
-> good
        
    4): Click Tools -> Development Board -> Development Board Manager -> Search for ESP32 -> 
Download version 1.0.6 -> Close -> ESP32 Arduino -> NodeMCU-32S
        
    5): Set the wifi name and password (in line 56), connect the ESP32, select the corresponding port, 
and then click the arrow below the edit to compile and download
    