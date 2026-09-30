#puts "argc = $argc"
#puts "argv = $argv"
#puts "arg 1 is [lindex $argv 0]"

if { $argc != 1 } {
    puts "not enough arguments to xmd script"
    exit
}

set offset 0
set file [lindex $argv 0]

# build directory, found the same way make places it
if {[info exists env(BUILDROOT)]} {set buildroot $env(BUILDROOT)} else {set buildroot .}
if {[info exists env(BUILDDIR_SUFFIX)]} {set suffix $env(BUILDDIR_SUFFIX)} else {set suffix ""}

connect arm hw

rst
after 1000
stop
dow -data $buildroot/build-$file$suffix/lk.bin $offset
rwr pc $offset
con

