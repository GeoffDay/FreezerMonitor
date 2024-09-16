This project is a monitoring system for freezers in a shed that is uninhabited most of the time. A recent coolroom failure resulted 
in over a thousand dollars of spoiled food. A master unit will read temperature on a couple of LM50CIM temp sensors that will be 
placed in the freezers at the shed, display the temps on an LCD with coloured status lights and transmit them via LoRa modules 
to 2 or 3 monitoring stations at members houses. LoRa has been chosen as it is sending a positive signal which if it stops could 
indicate a power failure at the shed. Using an ethernet system requires a dedicated whatsapp channel or software on a phone to 
indicate failure neither of which I am up to at the moment. There are two freezers and a coolroom, two sensors may be enough since food 
will go from the freezers to the coolroom prior to an event, three sensors would cover all eventualities. 
The slave stations will display temps and status and sound an alarm if freezer temps exceed a nominal value. Lets say 2 degrees c. 
We have two alarm conditions   1: any or all of the sensors exceeding 2 degrees C. 
                               2: power outage. 
Power outage may be indicated using battery backup to send a signal if charging current stops or simply by no signal. This is the 
simplest way and if we retransmit each monitoring stations readings to the other stations we have a more robust system. Each monitoring 
station will send the last recieved temps with a rt1 2 or 3 prepended. If a station stops receiving temp readings from the master then 
after 10 minutes will flag an error and sound an alarm.
