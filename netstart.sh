#!/bin/bash
ip tuntap add dev tap0 mode tap user oscar
ip tuntap add dev tap1 mode tap user oscar
ip link add br0 type bridge
ip link set br0 up
ip link set tap1 up
ip link set tap1 master br0
ifconfig br0 172.16.2.1 netmask 255.255.255.0 up
ifconfig tap0 up
ip link set tap0 master br0

