LOCAL_DIR := $(GET_LOCAL_DIR)

# The MPS2/MPS3 machines are all the same board from the kernel's point of
# view, so there is nothing to configure here. The specific FPGA image is
# selected with MPS2_MACHINE in the project target fragment.
PLATFORM := mps2
