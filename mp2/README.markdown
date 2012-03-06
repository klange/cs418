# CS 418 MP2

## Quaternion-based flight simulator.
Rotation (pitch/roll/yaw) is done with a quick quaternion implementation combined with a camera object that keeps track of its own position and direction. The quaternions make it simpler to perform rotations about particular axes, especially those corresponding to relative axes of the plane. The background scenery is the provided sample from mountains.c.

Controls are the same as the `mountain` demo that this is based on, but with the following additions:

* `w`   Speed up (forward)
* `s`   Slow down (reverse)
* `a`   Rudder left (immediate yaw left)
* `d`   Rudder right (immediate yaw right)
* `r`   Move up
* `e`   Move down

* up      Tilt the plane "up" (relative to the current orientation)
* down    Tilt the plane "down" (similarly)
* left    Rotate left
* right   Rotate right


## Installation ##
Should build fine with a quick `make` on Linux. Run with `./mp2`.
