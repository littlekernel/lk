# Fake module that just declares deps on all the PCI drivers in the system.
#
MODULES += dev/block/ahci
MODULES += dev/bus/pci
MODULES += dev/net/e1000
MODULES += dev/virtio/9p
MODULES += dev/virtio/block
MODULES += dev/virtio/gpu
MODULES += dev/virtio/net
MODULES += dev/virtio/rng
