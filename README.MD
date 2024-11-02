This project is a monitoring system for freezers in a shed that is uninhabited most of the time. A recent coolroom failure resulted 
in over a thousand dollars of spoiled food. A master unit will read temperature on a couple of LM50CIM temp sensors that will be 
placed in the freezers at the shed, display the temps on an LCD with coloured status lights and transmit them via LoRa modules 
to 2 or 3 monitoring stations at members houses. LoRa has been chosen as it is sending a positive signal which if it stops could 
indicate a power failure at the shed. Using an ethernet system requires a dedicated whatsapp channel or software on a phone to 
indicate failure neither of which I am up to at the moment. There are two freezers and a coolroom, two sensors may be enough since food 
will go from the freezers to the coolroom prior to an event, three sensors would cover all eventualities. 
The slave stations will display temps and status and sound an alarm if freezer temps exceed a nominal value. Lets say 2 degrees c. 
We have three alarm conditions    1: over temp - any or all of the sensors exceeding 0 degrees C. 
                                  2: disconnected sensor. 
                                  3: power outage.
Power outage could be indicated using battery backup to send a signal if charging current stops. More likely we will assume no signal 
means power failure. Assuming signal levels are better than marginal this should be OK. We will assume that if no signal is recieved 
for 30 mins then there is a power outage and an alarm will sound. At present I am 4km from the shed which is well within advertised range. 
The LoRa needs a clear line of sight between modules to operate correctly, not sure thats going to work from my home to the shed. 
Several others live closer, so they may be better suited to host this device.

