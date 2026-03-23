# OpenThread Simulation on POSIX

This directory contains example platform drivers for simulation on POSIX.

## Build Examples

```bash
$ cd <path-to-openthread>
$ mkdir build && cd build
$ cmake -GNinja -DOT_PLATFORM=simulation ..
$ ninja
```

After a successful build, the `elf` files are found in:

- `<path-to-openthread>/build/examples/apps/cli`
- `<path-to-openthread>/build/examples/apps/ncp`

## Interact

1. Spawn the process:

```bash
$ cd <path-to-openthread>/build/simulation/examples/apps/cli
$ ./ot-cli-ftd 1
```

2. Type `help` for list of commands.

```bash
> help
attachtime
ba
bbr
bufferinfo
ccathreshold
ccm
channel
child
childip
childmax
childrouterlinks
childsupervision
childtimeout
coap
coaps
coex
commissioner
contextreusedelay
counters
csl
dataset
debug
delaytimermin
detach
deviceprops
diag
discover
dns
domainname
eidcache
eui64
extaddr
extpanid
factoryreset
fake
fem
history
ifconfig
instanceid
ipaddr
ipmaddr
joiner
joinerport
keysequence
leaderdata
leaderweight
locate
log
mac
macfilter
mleadvimax
mliid
mlr
mode
multiradio
neighbor
netdata
netstat
networkdiagnostic
networkidtimeout
networkkey
networkname
nexthop
panid
parent
partitionid
ping
platform
pollperiod
preferrouterid
prefix
promiscuous
pskc
radio
radiofilter
rcp
region
releaserouterid
reset
rloc16
route
router
routeradmin
routereligible
routeridrange
scan
service
singleton
sntp
srp
state
targetpower
tcat
tcp
test
thread
timeinqueue
tvcheck
txpower
udp
unsecureport
uptime
vendor
verhoeff
version
exit
nodeidfilter
```
