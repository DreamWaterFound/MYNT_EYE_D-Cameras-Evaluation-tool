# Evalution tool for MYNT_EYE_D Cameras

A Evalution tool for MYNT EYE stereo cameras. I designed it for D1000-50.

## Feature

- [ ] Display stereo images and point cloud in one window (depended on Pangolin and OpenCV)
- [ ] Give an intuitive presentation for camera's rotation (based on IMU data)
- [ ] Drawing IMU raw data (include temperature) over time
- [ ] Time stamp drawing of three sensors ( left and right cameras, and IMU)
- [ ] Drawing exposure time over time

## Dependencies

- CMake
- Pangolin (included)
- Gflags (included)
- GLog (included)
- Eigen 3
- OpenCV 3
- Indicators (included)
- MYNT EYE D SDK

## Kown Bugs

- When you pressed "Ctrl+C" before the camera initializated, a segment fault will occured.


