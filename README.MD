This project will read temperature on a couple of LM50CIM temp sensors that will monitor the freezers at the shed display them on an LCD
and transmit them via LoRa modules to 2 or 3 monitoring stations at members houses. There are two freezers and a coolroom, two sensors 
may be enough since food will go from the freezers to the coolroom prior to an event, three sensors would cover all eventuallities. 
The monitoring station will display temps and status and sound an alarm if freezer temps exceed a nominal value. Lets say 2degrees c. 
We have two alarm conditions   1: any or all of the sensors exceeding 2 degrees C. 
                               2: power outage. 
Power outage may be indicated using battery backup to send a signal if charging current stops or simply by no signal. This is the 
simplest way and if we retransmit each monitoring stations readings to the other stations we have a more robust system.
