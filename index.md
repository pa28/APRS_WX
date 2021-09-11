---
show_downloads: ["yes"]
---

## [influxDB](www.influxdata.com) and [Grafana](https://grafana.com/)

InfluxDB and Grafana are great tools for storing and displaying (respectively) time-series data.
There are many great tutorials available around the web. If you are using a Raspberry Pi as your
server you coudl start here:
[https://simonhearne.com/2020/pi-influx-grafana/](https://simonhearne.com/2020/pi-influx-grafana/).

## Data Collection Daemons

This is where I will collect the list of my data collection daemons. There are two categoreis:
    * those aimed at Amateur Radio Operators.
    * those aimed at a general audience.

### Amateur Radio

#### APRS Weather Aggregator.

This is a daemon which connects to an [APRS_IS](http://www.aprs-is.net/) protocol backbone, registers
a data acquisition filter<sup>[1](#aprs_filter)</sup> to request packets within a specified radius of
a location (usually the home QTH). The daemon decodes weather packets and aggregates the data weighting
each packet according to the distance from the indicated location. Each time a new weather packet
arrives, a new aggregation is computed and sent as a measurement to the configured influxDB.

##### Availability

The daemon is available as a compiled binary for _armhf_ and _amd64_ architectures in _Debian_ packages
from my [Repository](https://pa28.github.io/Repository). Once the repository is configured on your
system the daemon may be installed with: `sudo apt install aprs_wx`

This is a [systemd](https://en.wikipedia.org/wiki/Systemd) service which you may:
    * start: `sudo systemctl start aprs_wx`
    * stop: `sudo systemctl stop aprs_wx`
    * derermine status: `sudo systemctl status aprs_wx`
    * enable to start at boot: `sudo systemctl enable aprs_wx`
    * disable from start at boot: `sudo systemctl disable aprs_wx`

Operation is controlled by a configuration file located at `/usr/local/share/APRS_WX/config.txt`.
A shadow configuration file located at `/usr/local/share/APRS_WX/config.pkg` is also installed
and upgraded as the configuration options change.

##### Configuration options:

<dl>
<dt><strong>callsign</strong>:</dt>
<dd>Specifies the amatuer radio callsign of the person responsible for running the daemon.
This is used to authenticate to the APRS_IS server. The default value begins with _N0CALL_
which indicates the configuration has not been customized.</dd><br/>
<dt><strong>passcode</strong>:</dt>
<dd>The passcode is associated with the callsign and is sued to authenticate the daemon.
If you do not know your passcode, or how to generate it contact the software provider.</dd><br/>
<dt><strong>latitude</strong>:</dt>
<dd>The latitude of the point of interest (home QTH) in decimal degrees, North positive.</dd><br/>
<dt><strong>longitude</strong>:</dt>
<dd>The longitude of the point of interest (home QTH) in decimal degrees, East positive.</dd><br/>
<dt><strong>radius</strong>:</dt>
<dd>The radius of the area of interest in kilometers.</dd><br/>
<dt><strong>influxTLS</strong>:</dt>
<dd>Set to 1 to use a TLS connection, 0 to use an unencrypted connection.</dd><br/>
<dt><strong>influxHost</strong>:</dt>
<dd>The host name of the systme running the influxDB server.</dd><br/>
<dt><strong>influxPort</strong>:</dt>
<dd>The port used to connect to the influx server.</dd><br/>
<dt><strong>influxDb</strong>:</dt>
<dd>The name of the database measurements will be sent to.</dd><br/>
</dl>

##### Command Line Options

The binary, usually at `/usr/local/bin/aprs_wx` may be run manually for testing purposes by:
```shell script
sudo aprs_wx --config /usr/local/share/APRS_WX/config.txt
```
