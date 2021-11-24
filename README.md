# esp8266-zabbix
Simple examples for creating zabbix-agent on ESP-devices

# These components were used for development:

1) `NodeMCU v3`
2) `Wemos D1 Mini`
3) `DHT11`
4) `DS18B20` (Tested only with D2 pin, I recommend to use it. GPIO16 (D0) can't be used - checked by me twice)
5) `BMP280`
6) `AHT10` (Only with GPIO4/D2 and GPIO5/D1 - view comments section for pinout)

# Sources
1) Async web-server was taken [here](https://randomnerdtutorials.com/esp8266-dht11dht22-temperature-and-humidity-web-server-with-arduino-ide/)
2) Zabbix Agent by [Azq2](https://github.com/Azq2)
3) AHTxx Library by [enjoyneering](https://github.com/enjoyneering/AHTxx)

# Known issues
1) Because of using millis() / 1000 as uptime, zabbix value can't be more than 41 days 17 hours (int32 overflow)

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

# bmp280.ino
Simple script, including:
1) Reading temperature & pressure from BMP280 sensor (maybe should work for BMP180 too, idk)
2) Providing simple web-server for displaying them (routes are: "/", "/temperature", "/pressure")
3) Zabbix-agent on port 10050 with this items:
    - agent.ping
    - agent.hostname
    - agent.uptime
    - agent.version
    - read.temperature
    - read.pressure

# ds18b20-discover.ino (legacy)
Simple script for detecting 64-bit addresses of DS18B20 sensors. Flash it & view address in console
Update: if you're using only one sensor - no need to do this, view `ds18b20.ino` comments section

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

# aht10.ino
Simple script, including:
1) Reading temperature & humidity from AHT10 sensor (should work for AHT15/AHT20 too)
2) Providing simple web-server for displaying them (routes are: "/", "/temperature", "/humidity")
3) Zabbix-agent on port 10050 with this items:
    - agent.ping
    - agent.hostname
    - agent.uptime
    - agent.version
    - read.temperature
    - read.humidity

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

# esp8266-bmp280-zabbix-agent.xml
* Pattern for Zabbix (BMP280 sensor). Tested on 5.4. 
* Included items:
    - agent.ping
    - agent.hostname
    - agent.uptime
    - agent.version
    - read.temperature
    - read.pressure
* Triggers:
    - ESP is unavailable
    - ESP was restarted
    - Hostname was changed
    - Agent Version was changed
* Simple graphs:
    - Temperature
    - Pressure
    
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

# esp8266-aht-zabbix-agent.xml
* Pattern for Zabbix (AHT10 sensor). Tested on 5.4. 
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
