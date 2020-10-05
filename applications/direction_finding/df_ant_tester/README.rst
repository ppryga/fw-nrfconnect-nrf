.. _bluetooth-df-ant-tester:

Direction Finding: antenna tester
#####################################

The DF Antenna Tester sample provides a reference implementation of an antenna functionality tester.

Overview
********

The tester application runs number of tests on selected antennas and provides results by UART.

Requirements
************

A Nordic board with Bluetooth LE 5.1 support:

   * nRF52833 Development Kit board (PCA10100)

Building and running
********************

.. code-block:: console
	mkdir build
	cd build
	cmake -DBOARD=nrf52833_pca10100 ..
	make
	make flash