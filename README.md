# CWOP/APRS Weather Aggregator

The primary tool provided is an [Automatic Packet Reporting System](http://www.aprs.org/apr) (APRS) or
[Citizen Weather Observation Program](http://wxqa.com/) (CWOP) 
weather aggregation system. APRS_WX is a systemd daemon that connects to an [APRS_IS](http://www.aprs-is.net/) protocol
server, configure the  feed to send packets within a specified radius of a given location. The weather reports are
aggregated using a weight based on the distance from the location which decays to zero at or before the specified radius.
The resulting information is pushed to an [InfluxDB](www.influxdata.com) database.