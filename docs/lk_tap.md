# Setting up a tun/tap network for QEMU

The `do-qemu*` scripts offer two kinds of networking:

- `-n` - QEMU user mode networking. Needs no setup; the guest sits behind QEMU's NAT and
  can reach out but is not reachable from the host network.
- `-t` - a tap interface named `qemu0`, which puts the guest on a real network segment.
  The interface has to exist before QEMU starts.

`-N` picks the network device model with either one (e.g. `virtio-net-pci`, `e1000e`).
`do-qemuarm`, `do-qemuriscv`, `do-qemux86` and `do-qemum68k` take `-t`; `do-qemumips` does
not.

## Create the tap interface

```bash
sudo ip tuntap add mode tap user $USER name qemu0
sudo ip link set qemu0 up
```

## Bridge it to the host network

To put the guest on the same network as the host, bridge `qemu0` with a physical
interface. The host's address then belongs on the bridge rather than the physical NIC, so
do this from a console, or let your network configuration tool (NetworkManager,
systemd-networkd, netplan) create the bridge instead.

```bash
sudo ip link add br0 type bridge
sudo ip link set qemu0 master br0
sudo ip link set <physical nic> master br0
sudo ip link set br0 up
```

To give the guest a private segment with the host only, skip the physical NIC and assign
an address to `br0` (or directly to `qemu0`) instead.

## Run

```bash
scripts/do-qemuarm -6 -t
scripts/do-qemux86 -6 -t -N virtio-net-pci
```
