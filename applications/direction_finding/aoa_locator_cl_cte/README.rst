.. _bluetooth-aoa-Locator-conectionless-cte:

Bluetooth: AoA Locator Connectionless CTE
########################################

Overview
********

The example provides a reference implementation of an angle of arrival locator.
The locator application uses Constant Tone Extension (CTE) added to regular advertising packets to gather IQ samples.
The samples are forwarded to UART output.
The application does not allow to establish a connection. 
The application also uses fixed MAC address, to get IQ samples only from particular beacon.

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
	cmake -DBOARD=nrf52833_pca10100 ..
	make
	make flash

KConfig
=======

The application provides following custom config entries:
	* AOA_LOCATOR_UART_PORT name of uart potr use to forward IQ samples.
	* AOA_LOCATOR_REGULAR_CTE enables sampling configration that comply with Bluetooth 5.1 specification
	* AOA_LOCATOR_OVERSAMPLING_CTE eanbles oversampling configuration that does not comply with Bluetooth 5.1 specification	

Pay attentio that oversampling is Nordics radio feature.
Also both configuration would not work at once, so you have to remember to disable one to enable other one.


prj.conf
========

The application configuration file consists of following parts:
	* general Kernel configuration that main responsibility is to set stacks and heap
	* general Bluetooth configuration to enable possiblity to scan for incomming advertising
	* UART configuratin, enable driver and setup interrupts.
	* direction finding configuration that enables: direction finding subsystem, reception of CTE, regular sampling settings 

 
Bluetooth initialization
========================

The application uses bluetooth scanning to receive and sample CTE.
The stack is configured to use fixed MAC address.
That is required to avoid corrupted IQ samples gathered when radio receives advertising pacet from a device that does not broadcast CTE.
Use of fixed mac address means, if one uses more than one Beacon in the same environment then they will interfere.
Scanning timing are set to following value:
	* scanning interval: 0x20, that equals to 20[ms]
	* scanning window: 0x20, that equals to 20[ms]
 
Direction Finding Extension
===========================

Direction finding extension is reponsible for receive and sample of Constan Tone Extension.
It is a functionality implemented in Nordics radio and provided by MCUs that have Bluetooth 5.1.
To enable DFE functionality one has to enable Direction Finding subsystem in bluetooth controller: ``CONFIG_BT_CTLR_DF_SUBSYSTEM=y``
To enable receving and sampling of CTE at the end of incomming advertising packet one has to set: ``CONFIG_BT_CTLR_DFE_RX=y``

The example receives and samples CTE in connectionless configuration.
That means, the application uses broadcaster advertising packets that have added CTE after CRC.
Use of connectionless configuration implies also the CTE received has 250kHz constant tone wave.
That is caused by use of Bluetooth 1 Mbps PHY in connection less transmission.

Radio Configuration
-------------------

To start CTE transmission, radio has to be correctly configured.
There is a number of registers that control DFE in radio. 
In case of Locator, CTE receiver, following registers are in use:
	* CTEINLINECONF controls if CTE inline configuration is enable (not supported by the Bluetooth stack yet)
	* DFEMODE controls mode of the DFE
	* DFECTRL1 provides various configuration for Direction finding
	* DFECTRL2 provided possiblity to set offsets for Direction Finding
	
CTE Inline Configuration
~~~~~~~~~~~~~~~~~~~~~~~~
Since there is no support for CTE inline functionality in the Bluetooth controller, the functionality is disabled implicitly.
``CTEINLINECONF.CTEINLINECTRLEN is set to zero (Disabled).``

DFE Mode
~~~~~~~~
Only one supported mode for direction finding is Angle of Arrival, because of that DFEMODE.DFEOPMODE is set to 3 (Direction finding mode set to AoA)
To set appropriate mode use :c:function:`dfe_set_mode` function.

DFE Duration
~~~~~~~~~~~~
To transmit CTE one needs to set its length.
That is provided to DFECTRL1.NUMBEROF8US.
Valid range of the number of 8us is 2-20, unfortunately we have found an issue and currently max value is limited to 10 (issue is under investigation).
To set CTE length use :c:function:`dfe_set_duration` function.

CTE Start point
~~~~~~~~~~~~~~~
CTE may be added to a Bluetooth pacekt in two palces:
	* after CRC
	* during packet payload
The start point of CTE broadcase must be set to the same value in beacon and locator.
Currently Bluetooth implementation supports transmission/receive ofCTE after CRC end.
This value should stay RADIO_DFECTRL1_DFEINEXTENSION_CRC and should not be changed.
To set CTE start point use :c:function:`dfe_set_start_point` function.

Sampling when CRC error
~~~~~~~~~~~~~~~~~~~~~~~
IQ sampling may be processed even if there is a CRC error detected.
In such case IQ samples may also be corrupted.
Because of that this setting is set to false. 
It is storngly advised to not change that setting.
To set sampling on CRC error use :c:function:`dfe_set_sample_on_crc_error` function.

AoA/AoD Trigger source
~~~~~~~~~~~~~~~~~~~~~~
The current implementation of the Direction Finding in Bluetooth stack supports start of AoA procedure by TASKS_DFESTART only.
When it is implemented, it will be possible to use some other singal to start the procedure also.
That meas this value should not be changed.
To set trigger sourcer use :c:function:`dfe_set_trig_dfe_start_task_only` function.

Sampling type
~~~~~~~~~~~~~
The radio is able to provide two kinds of samples:
	* complex samples(rectangural) I/Q
	* complex samples(polar) as magnitude and phase
	
I/Q samples are 12 bit including sign bit. Sign is extended to 16bits.
Polar samples are: 
	* magnitude 13bits unsigned value given as magnitude=K*sqrt(I^2+Q^2), where K≈1.646756 is the Cordic scaling factor.
	* phase 9 bits including sign bit, sign extended to 16bits
The application is based on I/Q (rectangular) complex samples.
The setting shoudl not be changed to provide correct values on the output.
To set the type of provided samples use :c:function: `dfe_set_sampling_type` function.
Allowed values that may be provided to function are: RADIO_DFECTRL1_SAMPLETYPE_IQ,RADIO_DFECTRL1_SAMPLETYPE_MagPhase.

Backoff gain
~~~~~~~~~~~~
The radio is able to change lower gain when start receive CTE. 
The gain is lowered a number of steps (max is 15).
The application does not change gain, so the backoff value is set to zero.
To set backoff gain use :c:function: `dfe_set_backoff_gain` function.

Antenna GPIOS
~~~~~~~~~~~~~
To run sampling there shoudl be attached an antennas matrix to the DK board.
The radio is able to handle up to 8 GPIOS to switch atennas.
Antennas are switched by setting particular GPIOS state to ones and zeros.
That means, radio is able switch up to 2^8 different antennas.

The application is implemented to work with provided by Nordic antennas matrix.
So there are 12 antennas available.
The application uses only first 4 GPIOS (of 8 possible).
There is no restriction which GPIO may not be used for antenna switching.

The example uses following GPIOS to handle the atenna matrix: (P0.03,P0.04,P0.28,P0.29).
To set antenna GPIOS use an array of following structures:
struct dfe_ant_gpio {
	u8_t idx;
	u8_t gpio_num;
};
where:
	* idx is index of PSEL.DFEGPIO
	* gpio_num is a port/pin number of the GPIO to be set: bit 0-4 are pin number (max 32), bit 5 is a port number

Example uses following GPIOS array (all GPIOS from port 0):
const static struct dfe_ant_gpio g_gpio_conf[4] = {
	{0, 3}, {1,4}, {2, 28}, {3,29}
};

To set antenna GPIO patterns use :c:function: `dfe_set_ant_gpios` function.

Antenna Patterns
~~~~~~~~~~~~~~~~
Antenna switch pattern is a binary number that each bit is applied to particular antenna GPIO pin.
E.g. we use 4 GPIOS so our patterns may not be greater than 15.
The pattern 0x3 means antenna GPIOS at index 0,1,2 will be set, and 4th is left unset.

The radio is able to store up to 40 antenna switch patterns.
At least 3 patterns must be provided: 
	* SWITCHPATTERN[0] is used in idle mode
	* SWITCHPATTERN[1] is used in guard and reference period
	* SWITCHPATTERN[2...] are used in switch-sampling period (at leas one must be provided)
If number of switch-sample periods is greater than number of stored switchpatterns, then the RADIO loops back to the pattern used after reference period (SWITCHPATTERN[2]).

Here are patterns that may be used to switch antennas on Nordics provided atenna matrix:
+--------+--------------+
|Antenna | ANT_SEL[3:0] |
+--------+--------------+
| ANT_12 |  0 (0000)    |
| ANT_10 |  1 (0001)    |
| ANT_11 |  2 (0010)    |
| ----   |  3 (0011)    |
+ -------+--------------+
| ANT_3  |  4 (0100)    |
| ANT_1  |  5 (0101)    |
| ANT_2  |  6 (0110)    |
| ----   |  7 (0111)    |
+--------+--------------+
| ANT_6  |  8 (1000)    |
| ANT_4  |  9 (1001)    |
| ANT_5  | 10 (1010)    |
| ----   | 11 (1011)    |
+--------+--------------+
| ANT_9  | 12 (1100)    |
| ANT_7  | 13 (1101)    |
| ANT_8  | 14 (1110)    | 
| ----   | 15 (1111)    |
+--------+--------------+

The application uses two arrays to set antennas:
	* ant_gpio_pattern that holds patterns that enable particular antennas (index of pattern is a number of antenna on the board)
	* antennae_switch_idx that holds indices of antennas to be stored in SWITCHPATTERN register (those indices are corresponding to ant_gpio_pattern indices).
The antennae_switch_idx stores swich-sampling antennas only.
The SWITCHPATTERN[0] is sotred in idle_ant_idx.
The SWITCHPATTERN[1] is sotred in ref_ant_idx.
The sequence the patterns are applied is following: idle_ant_idx, ref_ant_idx, antennae_switch_idx.
To set antenna patterns use :c:functions: `dfe_set_ant_gpio_patterns` function.

Antenna switch spacing
~~~~~~~~~~~~~~~~~~~~~~
After reference period antenna switch period begins.
The duration every switch-sample period has depends on setting provided.
Allowed values are:
	* RADIO_DFECTRL1_TSWITCHSPACING_4us (1UL)
	* RADIO_DFECTRL1_TSWITCHSPACING_2us (2UL)
	* RADIO_DFECTRL1_TSWITCHSPACING_1us (3UL) (This value is out of Bluetooth specification. It is a Nordic extension and was not tested in regard of provided samples and their usability). 
Every switch-sample period is divided into two parts: swich slot, sample slot.
Number of switch-sample periods depends on DFE duration (number of 8us).
E.g. 
Guard period lasts 4[us]
Reference period lasts 8[us]
If DFE duration is 5 -> 5*8[us]=40[us], then 40 - 12 = 28[us].
This is the time for antenna switching.
If we set atenna switch spacing to 2[us] then we have 14 antenna switches.
If we set 11 atennas in SWITCHPATTERN register then after 11th antenna we will have samples from SWITCHPATTERN[2],SWITCHPATTERNS[3],SWITCHPATTERNS[4] (because of loopback).
To set switch spacing use :c:functions: `dfe_set_ant_switch_spacing` function.

Switch spacing offset
~~~~~~~~~~~~~~~~~~~~~
The radio allows to do some fine tunig when the switching of antennas start.
That offset is applied before guart period starts (before first switch from idle state).
The value of the offset is a 12 bit signed number of 16M cycles (number of 62.5[ns]).
The example does not use this setting.
To set switch spacing use :c:functions: `dfe_set_switch_offset` function.

Reference samples spacing and switching period sample spacing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
During reference period samples are gathered accoring to reference samples spacing value.
Reference sample spacing allowed values are:
	* RADIO_DFECTRL1_TSAMPLESPACINGREF_4us (1UL)
	* RADIO_DFECTRL1_TSAMPLESPACINGREF_2us (2UL)
	* RADIO_DFECTRL1_TSAMPLESPACINGREF_1us (3UL)
	* RADIO_DFECTRL1_TSAMPLESPACINGREF_500ns (4UL)
	* RADIO_DFECTRL1_TSAMPLESPACINGREF_250ns (5UL)
	* RADIO_DFECTRL1_TSAMPLESPACINGREF_125ns (6UL)
Switch period sample spacing allowed values are:
	* RADIO_DFECTRL1_TSAMPLESPACING_4us (1UL)
	* RADIO_DFECTRL1_TSAMPLESPACING_2us (2UL)
	* RADIO_DFECTRL1_TSAMPLESPACING_1us (3UL)
	* RADIO_DFECTRL1_TSAMPLESPACING_500ns (4UL)
	* RADIO_DFECTRL1_TSAMPLESPACING_250ns (5UL)
	* RADIO_DFECTRL1_TSAMPLESPACING_125ns (6UL)
	
According to Bluetooth specification there is only one sample spacing allowed 1[us].
However Nordics radio provides also other settings.
Here the most interesting seems to be possiblity to use oversampling (spacing values that are lower than 1[us]).
*This is the only difference between configurations enabled by configs: AOA_LOCATOR_REGULAR_CTE and AOA_LOCATOR_OVERSAMPLING_CTE.*
*In case of AOA_LOCATOR_REGULAR_CTE the 1[us] sample spacing is used. (for both reference and switch periods)*
*In case of AOA_LOCATOR_OVERSAMPLING_CTE the 250[ns] sample spacing is used. (for both reference and switch periods)*
Also pay attention that the radio allows to set different sample spacing for referense and switch periods.
To set sample spacing for reference period use: :c:functions: dfe_set_sampling_spacing_ref function.
To set sample spacing for switching period use: :c:functions: dfe_set_sample_spacing function.

There is one more thing that have to be noted about sampling.
Sampling in reference period starts at the beginning of the period. 
That means last sample in reference period is taken "sample spacing time" before end of the period.
E.g. if reference sample spacing is set to 500[ns], then last sample is taken 500ns before end of reference period (or 7,5[us] after start of the period).
Sampling in switchin period does not start at the beginning of the period.
It starts after a delay that value is a half of the switch spacing time. 
E.g. if swich spacing is 2[us] then first sample arrives after delay of 1[us].
So delay between last reference period sample and first swich period sample is provided by formula: TSAMPLESPACINGREF + 1/2 * TSWITCHSPACING.
So for TSAMPLESPACINGREF=1us,    TSWITCHSPACING=4us, we get  delay 1 + 4/2    = 3us
       TSAMPLESPACINGREF=1us,    TSWITCHSPACING=2us, we get  delay 1 + 2/2    = 2us
       TSAMPLESPACINGREF=0.5us,  TSWITCHSPACING=2us, we get  delay 0.5 + 2/2 = 1.5us
That delay should be taken into accout when evaluation phase and time difference between samples from reference period and switching period.

The radio does not stop sampling in switching slots.
That has a drawback when time between samples is shorter than switch spacing.
In such case samples will be taken during switch period. 
That samples may be corruped because radio may not be in stable state to gather valid values.
Why are samples taken during switch period?
Because radio starts sampling and collects samples until end of DFE(CTE) duration.
E.g switch spacing is 2[us] (so switch slot (SW) is 1[us], sampling slot (SA) is 1[us].
sampling slot is 250[ns], "X" means sample.

+-------|-------+-------|-------+-------|-------+-------|-------+
   SW   |  SA   |   SW  |  SA   |   SW  |  SA   |   SW  |  SA   |
+-------|-------+-------|-------+-------|-------+-------|-------+
         X X X X X X X X X X X X X X X X X X X X X X X X X X X X 
+-------|-------+-------|-------+-------|-------+-------|-------+

After end of first SW slot, sampling starts and continues to end of DFE(CTE) duration.

So sampling in during the switch slot has implications.
Samples must be discarded, but sample does not have provided time when it was taken.
So all of it end in software that must evaluate timings of samples with use of provided settgins (switch and sample spacings), taking into account when sampling starts.

In case of reference period is it easy, every sample is valud.
In case of switching period it is more complicated. 
First of all, the algorithm must check if the spacing between samples is shorter than antennas switch spacing.
If that is ture, then half of samples should be discarded.

Similar compilation applies to mapping of samples to antennas.
That also have to be done by software because radio does not provide such information.
This is based on switch, sample spacing, length of DFE(CTE) and antennas switch pattern.
Length of DFE(CTE) with antenna spacing provides us number of effective antennas used.
Sample spacing and switch spacing allows us to find out which antenna was used to provide particular sample.
Pay attention that first antenna provides us only half of samples taken in single switch-sample period.

Implementation of samples to antennas mapping (including marking "255" discaded samples) may be found in :c:function `df_map_iq_samples_to_antennas`.

Sampling offset
~~~~~~~~~~~~~~~
Similar to switching, sampling start in swtich-sample period offset may be also fine tuned.
Is may be done by setting sample offset. 
The value of the offset is a 12 bit signed number of 16M cycles (number of 62.5[ns]).
So this may delay or make the sampling start faster (e.g. if switches on the antennas board are very slow).
Pay attention that this delay must be added to mapping of samples to time and antennas.
Note - it is not added in our evaluation of samples to antennas mapping!
The application sets this value to 1 (62.5ns) to move start of sampling just a little bit.
Bluetooth 5.1 specification states that sampels should be taken 125ns after start of sampling slot and 125ns before end of sampling slot.
To set sampling spacing use :c:functions: `dfe_set_sample_offset` function.

Radio is configured in :c:function:`dfe_init()`.