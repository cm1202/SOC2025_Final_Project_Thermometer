
**Introduction**\
This final project implements a digital thermostat system designed to replicate
core HVAC monitoring functions. The system features three primary modes:
real-time temperature display, baseline average recording, and a differential
mode that compares live readings against a stored reference value. The design
utilizes the Nexys 4 DDR FPGA board, using multiple peripherals for user input
and visual output. The control logic is designed as a Finite State Machine
written in C++, creating a interactive menu interface for navigating system
functions. Built upon Chu’s FPGA in C++ framework, the software handles
sensor data acquisition and drive the display outputs. A demonstration of the
final system is on YouTube:

[https://www.youtube.com/shorts/azU49sJCd7U](URL)

**Design**\
The system is primarily controlled through the onboard push buttons. Upon
startup, the board enters the IDLE state, indicated by ’RST’ on the SSEG
display and rightmost four LEDs illuminating. The top button functions as a
global reset, returning the system to this IDLE state from any other mode. The
middle button activates the live temperature mode, where users can view realtime
readings. The unit can be toggled between Celsius and Fahrenheit using
a switch. The down button accesses the difference state. Within this mode,
pressing the right button captures and saves a baseline average temperature.
Then the left button calculates the deviation between the live and stored temperatures.
This deviation is visually color coded using the RGB LEDs: a blue
light (with a negative sign on SSEG display) indicates a temperature drop, a
red light indicates a temperature rise, and a green light indicates no change.
