#puts "argc = $argc"
#puts "argv = $argv"
#puts "arg 1 is [lindex $argv 0]"

if { $argc != 1 } {
    puts "not enough arguments to xmd script"
    exit
}

set offset 0
set file [lindex $argv 0]

connect arm hw

rst
after 1000
stop
dow -data build-$file/lk.bin $offset
rwr pc $offset
con

