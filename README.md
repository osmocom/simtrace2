SIMtrace v2.0
=============

This is the repository for the next-generation SIMtrace devices,
providing abilities to trace the communication between (U)SIM card and
phone, remote (U)SIM card forward, (U)SIM man-in-the-middle, and more.

This is under heavy development, and right now it is not surprising if
things still break on a daily basis.

NOTE: Nothing in this repository applies to the SIMtrace v1.x hardware
or its associated firmware.  SIMtrace v1.x is based on a different CPU /
microcontroller architecture and uses a completely different software
stack and host software.

Supported Hardware
------------------

At this point, the primary development target is still the OWHW + sysmoQMOD
device, but we expect to add support for a SAM3 based SIMtrace hardware
board soon.

The goal is to support the following devices:

* Osmocom SIMtrace 1.x with SAM3 controller
** this is open hardware and schematics / PCB design is published
* sysmocom sysmoQMOD (with 4 Modems,  4 SIM slots and 2 SAM3)
** this is a proprietary device, publicly available from sysmocom
* sysmocom OWHW (with 2 Modems and 1 SAM3 onboard)
** this is not publicly available hardware, but still supported

This Repository
---------------

This repository contains several directory

* firmware - the firmware to run on the actual devices
* hardware - some information related to the hardware
* host - Programs to use on the USB host to interface with the hardware
