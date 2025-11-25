# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-11-25

### Added

- Initial release of Smart Door Lock System
- PIR motion detection functionality
- 4x4 keypad PIN authentication
- Servo motor door control mechanism
- RGB LED status indicators (Blue/Yellow/Green/Red)
- 16x2 LCD display user interface
- Auto-idle timeout functionality
- PIN verification system with retry capability
- State machine implementation (Idle/Active states)
- Professional documentation and circuit schema

### Features

- **Motion Detection**: Automatic system activation using PIR sensor
- **PIN Authentication**: Secure 4-digit PIN entry via keypad
- **Visual Feedback**: RGB LED status indicators and LCD display
- **Servo Control**: Automated door lock/unlock mechanism
- **Auto-Sleep**: Energy-efficient idle mode after timeout
- **Error Handling**: Invalid PIN attempts with retry functionality

### Technical Implementation

- Arduino-compatible codebase (tested on Uno/Nano)
- Modular function design for easy maintenance
- Efficient memory usage and power management
- Robust error handling and state validation
- Clear pin configuration and component interfacing

[unreleased]: https://github.com/douglascabrera/domotic-door-lock/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/douglascabrera/domotic-door-lock/releases/tag/v1.0.0
