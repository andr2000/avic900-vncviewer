#!/system/bin/ash -x

prevconfig=$(getprop sys.usb.config)
if [ "${prevconfig}" != "${prevconfig#rndis}" ] ; then
	echo 'Is tethering already active?' >&2
	exit 1
fi

echo "${prevconfig}" > /cache/usb_tether_prevconfig
setprop sys.usb.state 'rndis'
setprop sys.usb.config 'rndis'
until [ "$(getprop sys.usb.state)" = 'rndis' ] ; do sleep 1 ; done

ip rule add from all lookup main
ip addr flush dev usb0
ip addr add 192.168.2.1/24 dev usb0
ip link set usb0 up
iptables -t nat -I POSTROUTING 1 -o rmnet0 -j MASQUERADE
echo 1 > /proc/sys/net/ipv4/ip_forward
cp /sdcard/gscript/dnsmasq /data/local/dnsmasq
/data/local/dnsmasq --pid-file=/cache/usb_tether_dnsmasq.pid --interface=usb0 --bind-interfaces --bogus-priv --filterwin2k --no-resolv --domain-needed --server=8.8.8.8 --server=8.8.4.4 --cache-size=1000 --dhcp-range=192.168.2.2,192.168.2.254,255.255.255.0,192.168.2.255 --dhcp-lease-max=253 --dhcp-authoritative --dhcp-leasefile=/cache/usb_tether_dnsmasq.leases < /dev/null

