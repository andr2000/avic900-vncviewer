#!/system/bin/sh

if [ ! -f /cache/usb_tether_prevconfig ] ; then
	echo '/cache/usb_tether_prevconfig not found. Is tethering really active?' >&2
	exit 1
fi

if [ -f /cache/usb_tether_dnsmasq.pid ] ; then
	kill "$(cat /cache/usb_tether_dnsmasq.pid)"
	rm /cache/usb_tether_dnsmasq.pid
fi

echo 0 > /proc/sys/net/ipv4/ip_forward
iptables -t nat -D POSTROUTING 1
ip link set usb0 down
ip addr flush dev usb0
ip rule del from all lookup main

setprop sys.usb.config "$(cat /cache/usb_tether_prevconfig)"
rm /cache/usb_tether_prevconfig
while [ "$(getprop sys.usb.state)" = 'rndis' ] ; do sleep 1 ; done
