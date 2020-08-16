# E-Bike Logger
Summary: This device logs measured battery voltage and current, calculates amp-hours and watt-hours, and stores the data on an SD card in CSV format.

Soon after I got my Ancheer 26-inch folding electric bicycle, I built a data logger that directly measures battery voltage and current, and calculates wattage. It also records calculated energy consumption in terms of ampere-hours and watt-hours. It records this data on a microSD card every second. The datafile can be opened in a spreadsheet application like Excel to analyze and graph the data.

The prototype hardware used Dupont jumpers to connect the components temporarily housed in a dollar store kitchen container. This worked well on several rides so the next step was to build it on a stripboard and put it in a real project enclosure. The project has gone through several evolutionary improvements and is now well developed.

The project is based on an ESP32 dev kit, specifically the TTGO T8 V1.7 with onboard LiPo charger and microSD card socket. Other ESP32 boards could be used but few are available with the integrated microSD socket. Stand-alone sockets are available that can easily be connected to the ESP32.

E-bike battery current is measured with an ACS712 Hall-effect sensor rated at 20 amperes. Battery voltage is measured with one of the built-in analog to digital converters provided in the ESP32 and scaled with a simple voltage divider. Measured and calculated values are displayed on an LCD1602 backlit display and simultaneously logged to the SD card. At the end of your ride, you remove the card and plug it into a PC to read the comma-separated value (CSV) data. Future development will do the logging on a smartphone allowing the data to be combined with GPS tracking and video recording.

The data is timestamped with two numbers. One number is simply the time in seconds since the start of the log. The other number is the year, month, date, hour, minute, and second obtained from a network time protocol (NTP) source provided Wi-Fi is available when you turn the device on.

A key component is the dc-dc switch-mode converter that supplies 5 V from the e-bike battery. It is difficult to find one that will accept an input voltage of greater than the 42 volts of the open-circuit 36-V battery. I used one rated at 10-Amps that is overkill for the 300 mA needed by the ESP32 but provides plenty of power for a cell phone charger port. It is based on the LT3800 synchronous step down controller. The device is well characterized at https://lygte-info.dk/review/Converter%20DC-DC%2010A%206.5-60V%20to%201.25-30V%20UK.html. There are now sealed modules made for automotive applications that are smaller and less expensive.

The ESP32 has built-in touch sensors. I connected one to the body of the SPDT switch. Touching the switch handle for a fraction of a second changes the display. There are four display screens:

The default dashboard display shows voltage, current, watts, amp-hours, and watt-hours.
A bar graph of the battery current from 0 to 16 Amps.Each block is a 1-ampere division. Each block is subdivided into five columns each representing 200 mA.
A bar graph of battery wattage from 0 to 400 watts. Each block is a 25-watt division. Each block is subdivided into five columns each representing 5 watts.
The clock display shows the current time using a 24-hour format and the hours:minutes:seconds since the beginning of the log.
The code and schematic for the project are on GitHub at https://github.com/W4KRL/EBikeLogger.

**Here are some important details:**

* The software is written in the Arduino dialect of C++. You may install the latest version of the Arduino IDE from https://www.arduino.cc/en/Main/Software. Select the correct version for your computer. Do not use the Web version.

* Install the ESP32 core with the instructions at https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/

* Add these libraries using the Arduino IDE:

  * LiquidCrystal_I2C by Frank de Branbander https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library
  * ezTime by Rop Gonggrijp – https://github.com/ropg/ezTime
  
* Add your WiFi SSID and password at the places indicated (approximately lines 40 and 41). Add your timezone using the Olson format at line 42. Use this reference https://gist.github.com/ykessler/3349954

* Save the file and upload it to the ESP32.

**PARTS LIST**

1. TTGO T8 V1.7 WiFi Bluetooth module with ESP32 with 16MB flash – See Aliexpress.com. You can use any ESP32 devkit with the addition of an SD card reader module.
50V (or higher) to 5V Buck (Step-down) converter 1Amp or higher. Search Aliexpress for “DC 7-50V to 5V Step Down Converter”. The ones made for automobiles should work well. https://www.aliexpress.com/item/32916697168.html
1. LCD 1602 display with I2C interface
2. ACS712-20A – 20-amp version http://www.aliexpress.com/item/1103203995.html
120 k-ohm resistor
39 k-ohm resistor
10 k-ohm resistor
Deans Connector male
Deans connector female
USB Type-A connector. I used a socket taken from a dollar store phone charger.
Single Pole, Double Throw (SPDT) switch
Connectors and terminals as needed. Use #18 AWG wire or larger for the power circuit.
Stripboard 93×55 mm prototype board https://www.taydaelectronics.com/small-stripboard-93x55mm-copper.html
Project enclosure 71x106x39 mm https://www.taydaelectronics.com/project-plastic-box-03.html
Micro SD card 1GB
