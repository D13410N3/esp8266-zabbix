# esp8266-zabbix
Simple examples for creating zabbix-agent on ESP-devices

# These components were used for development:

1) `NodeMCU v3`
2) `DHT11`
3) `DS18B20`

# Sources
1) Async web-server was taken [here](https://randomnerdtutorials.com/esp8266-dht11dht22-temperature-and-humidity-web-server-with-arduino-ide/)
2) Zabbix Agent by [Azq2](https://github.com/Azq2)

# dht11.ino
Simple script, including:
1) Reading temperature & humidity from DHT11 sensor (should work for DHT22 too)
2) Providing simple web-server for displaying them (routes are: "/", "/temperature", "/humidity")
3) Zabbix-agent on port 10050 with this items:
    - agent.ping
    - agent.hostname
    - agent.uptime
    - agent.version
    - read.temperature
    - read.humidity

# ds18b20-discover.ino
Simple script for detecting 64-bit addresses of DS18B20 sensors. Flash it & view address in console

# ds18b20.ino
Simple script, including:
1) Reading temperature from DS18B20 sensor
2) Providing simple web-server for displaying it (routes are: "/", "/temperature")
3) Zabbix-agent on port 10050 with this items:
    - agent.ping
    - agent.hostname
    - agent.uptime
    - agent.version
    - read.temperature

# esp8266-dht-zabbix-agent.xml

* Pattern for Zabbix (DHT11 sensor). Tested on 5.4. 
* Included items:
    - agent.ping
    - agent.hostname
    - agent.uptime
    - agent.version
    - read.temperature
    - read.humidity
* Triggers:
    - ESP is unavailable
    - ESP was restarted
    - Hostname was changed
    - Agent Version was changed
* Simple graphs:
    - Temperature
    - Humidity
    - Temperature & humidity
    
# esp8266-ds18b20-zabbix-agent.xml

* Pattern for Zabbix (DS18B20 sensor). Tested on 5.4. 
* Included items:
    - agent.ping
    - agent.hostname
    - agent.uptime
    - agent.version
    - read.temperature
* Triggers:
    - ESP is unavailable
    - ESP was restarted
    - Hostname was changed
    - Agent Version was changed
* Simple graphs:
    - Temperature
