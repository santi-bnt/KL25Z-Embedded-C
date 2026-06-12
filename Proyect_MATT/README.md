# MATT

**MATT** is an embedded robotics project designed to assist with writing and erasing tasks on a classroom whiteboard. The project combines mechanical design, embedded control, wireless communication, a web interface, and computer vision to create a modular prototype capable of interacting with a vertical surface.

The system was developed as part of an engineering project focused on the integration of microcontrollers, actuators, communication protocols, and intelligent sensing in a real physical prototype.

---

## Project Overview

MATT is a whiteboard assistant robot that can receive commands from a user interface and translate them into physical actions such as movement, writing, and erasing. The project was built around the idea of connecting a traditional educational tool, the whiteboard, with embedded systems and IoT technologies.

The main goal of the project is not only to move a robot, but to coordinate multiple subsystems working together:

- Embedded control using a microcontroller
- Wireless communication through an ESP32
- Motor and servo actuation
- Web-based user interaction
- Computer vision using a Raspberry Pi and camera
- Power management for logic and actuators

---

## Problem Statement

Whiteboards are still widely used in classrooms, laboratories, and presentations. However, writing, erasing, and repeating board tasks are manual and repetitive activities. MATT proposes a robotic assistant capable of automating basic whiteboard interactions while serving as a practical example of embedded systems integration.

The main challenge was to design a system that could operate on a vertical surface while coordinating hardware, firmware, communication, and user interaction in a reliable way.

---

## Main Objectives

- Develop a functional embedded robotic prototype for whiteboard interaction.
- Control motors and servos from a microcontroller.
- Receive commands through a web interface.
- Establish communication between the ESP32 and the FRDM-KL25Z using UART.
- Integrate a Raspberry Pi camera for whiteboard detection.
- Test the system through basic movement, writing, and erasing routines.
- Apply concepts from embedded systems, digital systems, microcontrollers, and real-time control.

---

## General Architecture

The project is divided into several main blocks:

```text
User
  |
  v
Web Interface
  |
  v
ESP32
  |
  | UART Communication
  v
FRDM-KL25Z Microcontroller
  |
  |-- Motor Drivers
  |-- DC Motors
  |-- Servos
  |-- GPIO / PWM / Timers
  |
  v
Robot Mechanism

Raspberry Pi + Camera
  |
  v
Whiteboard Detection / Computer Vision
```

---

## Hardware Components

### Main Control

**FRDM-KL25Z**

- Main embedded controller.
- Handles low-level control logic.
- Generates PWM signals.
- Controls GPIOs.
- Communicates with the ESP32 through UART.

### Wireless Communication

**ESP32**

- Receives commands from the web interface.
- Works as a bridge between the user and the KL25Z.
- Sends instructions to the microcontroller using serial communication.

### Vision System

**Raspberry Pi**

- Used for image processing and camera integration.
- Detects the whiteboard area.
- Can identify board corners and define the working region.

**Camera Module**

- Captures the whiteboard.
- Provides image data for computer vision processing.

### Actuation

**DC Motors**

- Used for robot movement.
- Controlled through motor drivers.
- Speed can be adjusted through PWM.

**Servos**

- Used for mechanical actions such as lowering or lifting the marker and eraser mechanism.

### Power System

- Battery-powered prototype.
- Separate voltage regulation is recommended for:
  - Logic devices such as ESP32 and KL25Z.
  - Actuators such as motors and servos.
- Common ground is required between all electronic modules.

---

## Software Components

### Web Interface

The web interface allows the user to interact with the robot without directly connecting to the microcontroller. It can be expanded to include buttons, predefined routines, writing commands, erasing actions, or calibration options.

### ESP32 Firmware

The ESP32 is responsible for receiving the commands sent from the interface and forwarding them to the FRDM-KL25Z through UART communication.

Main responsibilities:

- Establish wireless communication.
- Receive user commands.
- Format and send instructions through UART.
- Act as a communication bridge between the interface and the embedded controller.

### FRDM-KL25Z Firmware

The KL25Z performs the low-level control of the robot.

Main responsibilities:

- Configure GPIOs.
- Generate PWM signals.
- Control motor direction and speed.
- Control servos.
- Manage basic system states.
- Interpret commands received through UART.

### Raspberry Pi Vision Software

The Raspberry Pi handles the computer vision part of the project.

Possible tasks:

- Capture video from the camera.
- Detect the whiteboard limits.
- Identify the corners of the board.
- Define a valid working area for the robot.
- Support future positioning or calibration features.

---

## Embedded Systems Concepts Applied

### GPIO

GPIO pins are used to control digital signals, such as motor driver inputs, enable pins, and actuator control lines.

### PWM

PWM signals are used to control motor speed and servo positioning. This allows the system to adjust movement intensity and actuator behavior.

### UART

UART communication is used between the ESP32 and the FRDM-KL25Z. This allows the wireless interface and the embedded control layer to remain separated while still exchanging commands.

### Timers

Timers can be used to define movement durations, generate periodic actions, or control execution timing without relying only on blocking delays.

### Interruptions

Interruptions can be used to respond to external events, safety signals, buttons, or sensors without continuously checking them in the main program.

---

## State Machine

The system can be represented as a state machine to organize its behavior.

```text
Initialization
      |
      v
Idle / Waiting for Command
      |
      v
Command Reception
      |
      |-- Movement State
      |-- Writing State
      |-- Erasing State
      |-- Calibration State
      |-- Error / Safety State
      |
      v
Return to Idle
```

| State | Description |
|---|---|
| Initialization | Configures GPIOs, PWM, UART, timers, and actuators. |
| Idle | Waits for a new instruction from the user interface. |
| Command Reception | Reads and interprets the command received from the ESP32. |
| Movement | Activates motors according to the requested action. |
| Writing | Controls the marker mechanism and executes a writing routine. |
| Erasing | Activates the eraser mechanism and performs an erasing routine. |
| Calibration | Adjusts movement, position, or board reference values. |
| Error / Safety | Stops actuators and waits for a reset or safe command. |

---

## IoT and Computer Vision Integration

MATT includes an additional layer of IoT and computer vision.

### IoT Component

The ESP32 allows the robot to receive instructions wirelessly from a web interface. This makes the system easier to control and more flexible than a fully wired setup.

Benefits:

- Remote interaction from a browser.
- Separation between user interface and low-level control.
- Easier expansion for future cloud or mobile features.
- More realistic embedded system architecture.

### Computer Vision Component

The Raspberry Pi and camera are used to detect the whiteboard area. This gives the robot a visual reference of the surface where it is working.

Possible vision features:

- Whiteboard corner detection.
- Perspective correction.
- Work area definition.
- Visual calibration.
- Future robot localization.

---

## Integration Strategy

The project was developed using a modular integration strategy.

1. Test each hardware component individually.
2. Validate motor and servo control.
3. Test UART communication between ESP32 and KL25Z.
4. Connect the web interface with the ESP32.
5. Integrate the Raspberry Pi camera system.
6. Combine all modules into the final prototype.
7. Test the complete system with basic routines.

This approach helps reduce errors and makes debugging easier because each subsystem can be tested separately before full integration.

---

## Main Challenges

Some of the main challenges found during development were:

- Calibrating motor movement on a physical surface.
- Managing power for motors and servos.
- Keeping stable communication between ESP32 and KL25Z.
- Coordinating multiple platforms in the same project.
- Making the mechanical system behave as expected in real conditions.
- Integrating embedded control with a web interface and camera system.

---

## Solutions Applied

To solve the main integration challenges, the project used the following strategies:

- Modular testing before full system integration.
- Common ground between all electronic modules.
- Separate power regulation for logic and actuators.
- PWM adjustment for motor behavior.
- UART communication for a simple and reliable data exchange.
- State machine logic to organize the system behavior.
- Progressive validation of movement, communication, and actuation.

---

## Validation Plan

The prototype can be validated through several test cases.

| Test Case | Expected Result |
|---|---|
| Power-on test | All modules initialize correctly. |
| UART test | ESP32 sends data and KL25Z receives it correctly. |
| Motor test | Motors respond to control signals. |
| Servo test | Marker and eraser mechanisms move correctly. |
| Web interface test | User commands are received by the system. |
| Vision test | Camera detects the whiteboard area. |
| Integration test | The complete system performs a basic routine. |

---

## Future Improvements

Possible improvements for future versions include:

- Closed-loop motor control using encoders.
- Better position estimation on the whiteboard.
- More accurate writing trajectories.
- Full integration of computer vision with movement correction.
- Mobile app or improved web dashboard.
- Battery monitoring system.
- Emergency stop system.
- Mechanical redesign for better stability on vertical surfaces.

---

## Learning Outcomes

This project helped connect concepts from different engineering courses into one complete system.

Main learning outcomes:

- Embedded programming on a real microcontroller.
- Peripheral configuration and control.
- Communication between microcontrollers.
- Integration of software and hardware.
- Real-world debugging.
- Importance of modular design.
- Relationship between digital systems, computer architecture, and systems on chip.

MATT represents the transition from isolated digital logic and programming exercises to a complete embedded system with a physical purpose.

---

## Team

Developed by:

- Santiago Benavent
- [Add teammate name]
- [Add teammate name]
- [Add teammate name]

Course:

- Systems on Chip / Microcontrollers and RTOS

Institution:

- Tecnológico de Monterrey

---

## Project Status

This project is a functional educational prototype under development. The current version focuses on embedded control, communication, mechanical actuation, and system integration. Future versions can improve movement accuracy, vision-based calibration, and autonomous whiteboard interaction.
