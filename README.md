# CWOP/APRS Weather Aggregator

The primary tool provided is an [Automatic Packet Reporting System](http://www.aprs.org/apr) (APRS) or
[Citizen Weather Observation Program](http://wxqa.com/) (CWOP) 
weather aggregation system. APRS_WX is a systemd daemon that connects to an [APRS_IS](http://www.aprs-is.net/) protocol
server, configure the  feed to send packets within a specified radius of a given location. The weather reports are
aggregated using a weight based on the distance from the location which decays to zero at or before the specified radius.
The resulting information is pushed to an [InfluxDB](www.influxdata.com) database.

## Table of Contents
1. [Prebuilt Packages](#debian-packages)
1. [Build](#building)
1. [Requirements](#requirements)
    1. [Get the Source](#get-the-software)
    1. [Compile](#compile)
    1. [Install](#install)
1. [Configure](#configure)
1. [Running](#running-the-daemon)

## Debian Packages

Debian packages are available for 32 bit ARM architecture which runs on Raspberry Pi, and 64 bit Intel
architecture which runs on Intel and AMD processors. Configure the
[repository](https://pa28.github.io/Repository) then
```
sudo apt update
sudo apt install aprs_wx
```

## Building

### Requirements:
* git
* cmake
* A C++ compiler
* libcurlpp-dev - C++ wrapper around libCURL
* libcurl4-gnutls-dev, libcurl4-nss-dev or libcurl4-openssl-dev - cURL libraries based on various crypto systems.

``` shell script
apt install git cmake g++ libcurl4-openssl-dev libcurlpp-dev
```

### Get the software
``` shell script
git clone git@github.com:pa28/APRS_WX.git
```

### Compile
``` shell script
cd APRS_WX
mkdir cmake-build-release
cd cmake-build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

### Install
``` shell script
make install
```

### Make Debian packages
``` shell script
make package
```

## Configure
The configuration file will be located at `/usr/local/share/APRS_WX/config.txt` or
equivalent location. The package default configuration file will also be installed
in the same directory under the name `config.pkg` and updated if aditional optiions
are added.

Default config.txt:
``` text
#
# Configuration file for APRS_WX Weather Agregator
#
# Set to the callsign, required for validation, SSID optional.
callsign N0CALL-9
# Set the passcode associated with the callsign, required for validation. If you do not know your passcode, or
# how to generate it contact the software provider.
passcode 12345
# QTH Latitude South negative
latitude 12.3456
# QTH Longitude West negative
longitude -123.4567
# APR_IS filter radius in km
radius 50
#
# InfluxDB parameters
#
# To use https set this to 1
influxTLS 0
# The database server host name, assumes influx is an alias for the database server.
influxHost influx
# The database connection port
influxPort 8086
# The database name to store measurements.
influxDb aprs_wx
```

## Running the Daemon
### Start
``` shell script
sudo systemctl start aprs_wx
sudo systemctl status aprs_wx
```

### Enable start on boot
``` shell script
sudo systemctl enable aprs_wx
```