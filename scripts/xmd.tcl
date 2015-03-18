#puts "argc = $argc"
#puts "argv = $argv"
#puts "arg 1 is [lindex $argv 0]"

if { $argc != 1 } {
    puts "not enough arguments to xmd script"
    exit
}

connect arm hw

rst
after 1000
stop
dow -data build-[lindex $argv 0]/lk.bin 0
rwr pc 0
con

