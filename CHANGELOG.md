# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## 0.4.0
### Added
- Added MeasurementToTopicSuffixEmpty, to allow more simplistic message handling.
- Added MeasurementToTopicSuffixTree, to allow hierachical topic handling.
- Added FullMeasurementFormatter, to go with MeasurementToTopicSuffixEmpty

### Fixed 
- [BREAKING] Fixed typo in DefaultMeasurementToTopicSuffix.

## 0.3.0
### Changed
- Moved to stable UPT Core 1.0.0

## 0.2.1
### Changed
- Refactor for compilation with arduino UPT core 0.9

## 0.1.0

### Added
- Add implementation of MQTT client with optional delegation of Wi-Fi setup and monitoring.
- Provide convenience functions to send Measurements.
- Ability to specify a global prefix for the topic.
- Supporting only PlatformIO
