## Application and radio protocols and timings

The goal of the application layer protocol is to be :
 * Energy efficient
 * Radio efficient (minimize collisions)
 * "ticked" measures

## Time sync
The whole sensor network is expected to share a common time, in order to
synchronize the measures.

A GPS receiver is implemented on hardware v0_1, and will be used as a primary
time source for the beginning of the deployment. Since GPS receivers are costly
and consume energy, A radio synchronization protocol will be deployed later.

The document "LoRaWAN Application Layer Clock Synchronization Specification v1.0.0"
describes such a time synchronization protocol and can be implemented as is or
probably can be improved (resolution is 1 second, ...). The precision can probably
be improved by using a sub-second resolution, and power efficiency can probably
be improved by requesting time correction at a future know time delta, as a response
to one of our measure packet.

Once a common time is shared across the network, LoRaWAN nodes will start to do
periodic and synchronized measures at a configurable interval, named T_MEASURE.
Sensors requiring a significant time before T_MEASURE will be powered and
initialized in order to have a value valid at T_MEASURE.

The measurement results will be sent at a (possibly other) configurable interval,
named T_RADIO_TX. The values to be transmitted can be a computed mean/min/max/threshold/...
image of the measure.
T_RADIO_TX will be (pseudo-)randomized to avoid radio collisions. Since the
DevAddr is unique in a network, it could be used as a seed. In order to prevent
pathological ("bad luck, always bad") timings, each transmit will be pseudo-random.

Example timings for measure each 2 minutes, radio TX each 6 minutes:
08h12:00 : measure 1
08h14:00 : measure 2
08h16:00 : measure 3
08h18:00 : measure 4
08h18:24 : radio TX mean measure 1-3
08h20:00 : measure 5
08h22:00 : measure 6
08h22:13 : radio TX mean measure 4-6
08h24:00 : measure 7

In order to minimize device consumption, T_RADIO_TX(+radio duration) SHOULD not
overlap with T_MEASURE(+measure duration).

Power source and usage (battery voltage/capacity/solar power/battery temperature)
will be monitored and transmitted, either periodically or when an significant
change is detected (for instance, every 0.1V but at least twice a day).

Downlink messages can be used to configure the various periods, modes, thresholds, ...

## Timings

All time are in unix time (seconds since january 1970) and represents the UTC
time.

Periodic measures are done at T_MEASURE interval, and send at T_RADIO_TX
interval.

The measure will be done at MEASURE_TS, when unix_tx % T_MEASURE == 0.
Since all measures are synchronized in the network, sending the result over
radio at the same time will almost guarantee collisions. We try to avoid
collision using this pseudo-random generated time:

RADIO_TS = MEASURE_TS + (HASH(DevAddr | measure_ts) % T_RADIO_TX)

Since there is no need to 

## Wintering
For this particular application, when the irrigation network is emptied for the
winter period, the nodes are expected to stay in place and conserve as much
power as possible for the next season.

Periodic message MUST still be send for:
  0. (MANDATORY) Configure the end of wintering.
  0. (MANDATORY) Plan an eventual battery servicing, or detect a catastrophic
     failure (fire, landslide, avalanche, ...).
  0. (OPTIONAL) Log the effective climatic conditions (temperatures, solar power,
      ...), for future designs.

# Proposed messages

## Downlink : REQ_Measure
* Type      : confirmed
* Response  : None
* Port      : 0
* Data      : u8 input_type, u8 power, u16 wakeup_duration_ms, u16 period, u16 threshold, u16 radio_div, u8 what
            input_type :
                bit 7 : sensor #
                bit 6..0 : sensor type
                    0: analog16
                    1: i2c
                    2: digital
                    3: pulse
            power:
                0 : none (sensor is self-powered)
                1 : 5v
                2 : v_indus

            wakeup_duration_ms
                0..65534 : power should be on ms before measure valid
                65535 : 

            period:
                in seconds.

            threshold:
                if the measure changed more than threshold since the last radio TX, TX now

            radio_div:
                TX one "what" every radio_div measures

            what : bitfield
                    bit7 : avg
                    bit6 : min
                    bit5 : max
                    bit6..0 : RFU
