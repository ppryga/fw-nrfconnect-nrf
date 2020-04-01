.. _bluetooth-aoa-Locator-conectionless-cte:

Bluetooth: AoA locator with simple geometrical angle calculation.
#################################################################

The Angle of Arrival (AoA) locator that uses only 2 antennas and calculates the AoA using onlu simple geometric calculations.
This implementation is provided mainly for internal testing with really simple algorithm where dataflow is easy to follow.

Overview
********

The locator application uses Constant Tone Extension (CTE) added to regular advertising packets to gather IQ samples.
The calculated AOA and samples are forwarded to UART output.
The application does not allow to establish a connection.
The application uses fixed MAC address to get IQ samples only from a particular beacon.

Requirements
************

A Nordic board with Bluetooth LE 5.1 support:

   * nRF52833 Development Kit board (PCA10100)
   * nRF52811 Development Kit board (PCA10068)

Building and running
********************

.. code-block:: console
	mkdir build
	cd build
	cmake -DBOARD=nrf52833_pca10100 ..
	make
	make flash

KConfig
=======

The application provides the following custom configuration options:

	* ``AOA_LOCATOR_UART_PORT`` defines the name of the UART port use to forward IQ samples.

Note that oversampling is a Nordic radio feature.
Both configurations cannot work at once so make sure to disable one when enabling the other.

prj.conf
========

The application configuration file consists of the following parts:

   * General kernel configuration that sets the stacks and heap.
   * General Bluetooth configuration that enables scanning for incoming advertising.
   * UART configuration that enables driver and setup interrupts.
   * Direction finding configuration that enables: the direction finding subsystem, reception of CTE, regular sampling settings.

To enable oversampling one may use following configuarion of sampling:
CONFIG_BT_CTLR_DFE_SAMPLE_SPACING_250NS=y
CONFIG_BT_CTLR_DFE_SAMPLE_SPACING_REF_250NS=y

More documentation
==================

For a technical documentation about bluetooth and technical implemnetation
of direction finidnig see the `bluetooth-aoa-Locator-conectionless-cte`_ documentation.
