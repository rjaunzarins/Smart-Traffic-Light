# D Flip-Flop Controlled Smart Traffic-Light System

CPSC5491 - Internet of Things  
Laurentian University - Fall 2026  

**Group 11:** Rhy Jaunzarins and Kaylee Joyce

## Overview
This project implements a three-way intersection smart traffic-light 
system using Autodesk Tinkercad Circuits with an Arduino Uno and two 
independent hardware finite-state machines (FSMs). The intersection
is developed around the actual intersection at University Road and
Ramsey Lake Road, the entrance to Laurentian University.

The Arduino is used to process sensor data from three ultrasonic
sensors located on each traffic light facing its approaching
vehicles. Once processed, the data is used to make traffic-light
timing decisions, which is used to inform the hardware through several
control signals, along with a clock pulse to transition the hardware 
to the next traffic-signal phase. 

The hardware FSMs use D flip-flops
and combinational logic to store the current state, determine the
next state, and the associated traffic-light signal output. 

The PCF8574 I2C port expander reads the state of the primary
hardware FSM, while the Arduino software tracks the expected
hardware state. If the software predicted state and the actual
hardware state do not match, the Arduino signals the hardware to 
enter the flashing-red fallback mode.

The system includes adaptive traffic timing, pedestrian crossing
signals, hardware fault detection, and software state verification.

## Repository Structure

- `src/SmartTrafficLight/` - Arduino source code (.ino)
- `docs/CPSC5491 - G11 - Term Project.pdf` - Final project report
- `docs/Traffic_Light_Schematic.pdf` - Complete circuit schematic
- `docs/Final-Presentation.pdf` - Final project presentation

## Simulation

The project was developed and tested using Autodesk Tinkercad Circuits.
[View and test the complete Tinkercad simulation](https://www.tinkercad.com/things/io6dZTbmpuM-d-flip-flop-controlled-smart-traffic-light-system)

The Arduino source code requires an Arduino-compatible environment.
The complete hardware circuit is available through the Tinkercad
simulation software and the full schematic is included in the repo.

## Testing

Testing was completed on FSM transitions, vehicle detections, adaptive
timing, startup operation, and hardware and software fault detection.

All 12 adaptive timing scenarios passed. All FSM state transitions 
were tested and verified, including from the unused state passed. All 
hardware fault-injection tests properly triggered the fallback flashing
-red mode and there were no false activations during testing. The 
startup operation also worked correctly throughout testing.
