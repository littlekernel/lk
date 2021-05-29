# Setting up a tun/tap network for qemu
sudo ip tuntap add mode tap user $USER name qemu0
sudo ip link set qemu0 up

# manually making bridge
sudo brctl addbr br0
sudo brctl addif br0 qemu0
sudo brctl addif br0 <physical nic>
sudo ip link set br0 up

# alternatively using the network config to create the bridge and merge it with particular vlan
