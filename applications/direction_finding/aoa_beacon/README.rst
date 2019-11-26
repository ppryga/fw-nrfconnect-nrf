.. _bluetooth-AoA_beacon-sample:

Bluetooth: AoA Beacon
#################

Overview
********

A simple application demonstrating the BLE Broadcaster role functionality by
advertising an Eddystone URL with AoA CTE transmitting. DF configuration in bluetooth/controller/ll_sw/df_config.h. Using defined MAC address.



Requirements
************

* A Nordic board with BLE 5.1 support

Building and Running
********************

mkdir build
cd build
cmake -DBOARD=nrf52811_pca10068 ..
make
make flash
