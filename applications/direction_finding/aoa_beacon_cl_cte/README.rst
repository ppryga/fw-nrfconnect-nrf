.. _bluetooth-aoa-beacon-conectionless-cte:

Bluetooth: AoA Beacon Connectionless CTE
########################################

Overview
********

The example provides a reference implementation of an angle of arrival beacon.
The beacon uses Constant Tone Extension (CTE) added to regular advertising packets.
It does not allow to establish connection. 
The application uses fixed MAC address, that is expected by angle of arrival locator examples.

Requirements
************

A Nordic board with BLE 5.1 support:
	* nRF52833 Development Kit board (PCA10100)
	* nRF52811 Development Kit board (PCA10068)

Building and Running
********************

.. code-block:: console
	mkdir build
	cd build
	cmake -DBOARD=nrf52811_pca10068 ..
	make
	make flash

KConfig
=======

The KConfi of the example application consists of two parts:
	* general Bluetooth configuration to enable possiblity to advertise
	* direction finding configuration that enables direction finding subsystem and enables transmission of CTE
	
 
Bluetooth initialization
========================

The application uses bluetooth advertising to broadcase CTE.
The stack is configured to use fixed MAC address.
That means, if one uses more than one Beacon in the same environment then they will interfere.
Advertising interval time is configured by :c:macro:`BT_ADV_INTERVAL` macro.
 
Direction Finding Extension
===========================

Direction finding extension is reposible for broadcast of Constan Tone Extension.
It is a functionality implemented in Nordics radio and provided by MCUs that have Bluetooth 5.1 implemented.
To enable DFE functionality one has to enable Direction Finding subsystem in bluetooth controller: ``CONFIG_BT_CTLR_DF_SUBSYSTEM=y``
To enable transmission of CTE at the end of advertising packet one has to set: ``CONFIG_BT_CTLR_DFE_TX=y``

The example runs CTE in connectionless configuration.
That means, the application uses advertising packets to broadcase CTE.
Use of connectionless configuration implies also transmission 250kHz constant tone wave.
That is caused by use of Bluetooth  1 Mbps PHY in connection less transmission.

Radio Configuration
-------------------

To start CTE transmission, radio has to be correctly configured.
There is a number of registers that control DFE in radio. 
In case of Beacon, CTE broadcaster only following registers are in use:
	* CTEINLINECONF controls if CTE inline configuration is enable (not supported by the Bluetooth stack yet)
	* DFEMODE controls mode of the DFE
	* DFECTRL1 provides various configuration for Direction finding
	
Since there is no support for CTE inline functionality in the Bluetooth controller, the functionality is disabled implicitly.
``CTEINLINECONF.CTEINLINECTRLEN is set to zero (Disabled).``
Only one supported mode for direction finding is Angle of Arrival, because of that DFEMODE.DFEOPMODE is set to 3 (Direction finding mode set to AoA)
To set appropriate mode use :c:function:`dfe_set_mode` function.
Also to transmit CTE one needs to set its length.
That is provided to DFECTRL1.NUMBEROF8US.
Valid range of the number of 8us is 2-20, unfortunately we have found an issue and currently max value is limited to 10 (issue is under investigation).
To set CTE length use :c:function:`dfe_set_duration` function.
Radio is configured in :c:function:`dfe_init()`.

