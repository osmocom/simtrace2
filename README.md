SIMtrace v2.0
=============

This is the repository for the next-generation SIMtrace devices,
providing abilities to trace the communication between (U)SIM card and
phone, remote (U)SIM card forward, (U)SIM man-in-the-middle, and more.

NOTE: Nothing in this repository applies to the SIMtrace v1.x hardware
or its associated firmware.  SIMtrace v1.x is based on a different CPU /
microcontroller architecture and uses a completely different software
stack and host software.

Supported Hardware
------------------

* Osmocom [SIMtrace2](https://osmocom.org/projects/simtrace2/wiki) with SAM3 controller
  * this is open hardware and schematics / PCB design is published
  * pre-built hardware available from [sysmocom webshop](https://shop.sysmocom.de/SIMtrace2-Hardware-Kit/simtrace2-kit)
* Osmocom [ngff-cardem](https://osmocom.org/projects/ngff-cardem/wiki) M.2/NGFF modem carrier with SAM3 controller
  * this is open hardware and schematics / PCB design is published
  * pre-built hardware available from [sysmocom webshoo](https://shop.sysmocom.de/M.2-modem-carrier-with-remote-SIM-tracing/ngff-cardem-kit-external)
* sysmocom [sysmoQMOD](https://sysmocom.de/products/lab/sysmoqmod/index.html) (with 4 Modems,  4 SIM slots and 2 SAM3)
  * this is a proprietary device, publicly available from sysmocom
  * hardware evaluation kit available from [sysmocom webshop](https://shop.sysmocom.de/sysmoQMOD-evaluation-kit/sysmoQMOD-evk)
* sysmocom OWHW (with 2 Modems and 1 SAM3 onboard)
  * this is not publicly available hardware, but still supported

This Repository
---------------

This repository contains several directory

* firmware - the firmware to run on the actual devices
* hardware - some information related to the hardware
* host - Programs to use on the USB host to interface with the hardware


The host software includes

* libosmo-simtrace2 - a shared library to talk to devices running the simtrace2 firmware
* simtrace2-list - list any USB-attached devices running simtrace2 firmware
* simtrace2-sniff - interface the 'trace' firmware to obtain card protocol traces
* simtrace2-cardem-pcsc - interface the 'cardem' fimrware to use a SIM in a PC/SC reader
