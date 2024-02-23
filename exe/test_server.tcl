#!/usr/bin/bltwish30

package require BLT
#package require Img
package require Tk

set auto_path [linsert $auto_path 0 $blt_library]

puts "Starting the Test server in TCL"


# Echo_Server --
#	Open the server listening socket
#	and enter the Tcl event loop
#
# Arguments:
#	port	The server's port number
option add *Graph.Tile			bgTexture
option add *Label.Tile			bgTexture
option add *Frame.Tile			bgTexture
option add *Htext.Tile			bgTexture
option add *TileOffset			0
option add *HighlightThickness		0
option add *Element.ScaleSymbols	no
option add *Element.Smooth		linear
option add *activeLine.Color		yellow4
option add *activeLine.Fill		yellow
option add *activeLine.LineWidth	0
option add *Element.Pixels		3
option add *Graph.halo			7i

set FFT_SIZE 8192
set FFT_SIZE_DIV2 4096
set FRAC_SIZE 100000
global FFT_SIZE

blt::vector xdata($FFT_SIZE)
blt::vector xdata2($FFT_SIZE_DIV2)
blt::vector ts($FFT_SIZE)
blt::vector ps($FFT_SIZE)
blt::vector gs($FFT_SIZE)
blt::vector svd($FFT_SIZE)
blt::vector frac_x(2)
blt::vector pos(2)
blt::tabset .ts


set temp [expr double(11025)/double($FFT_SIZE_DIV2)]

for {set i 0} {$i<$FFT_SIZE_DIV2} {incr i 1} {
  set xdata2($i) [ expr double($i+0.5)*$temp]
}

for {set i 0} {$i<$FFT_SIZE} {incr i 1} {
set xdata($i) $i
set ts($i) 0
set svd($i) 0
set ps($i) 0
set gs($i) 0
#puts "x value is $xdata($i) $ts($i)"

}





frame .ts.tab1
frame .ts.tab2
frame .ts.tab3
frame .ts.tab4



.ts insert end "test"

.ts tab configure "test" -text "Waveform" -window ".ts.tab1"


.ts insert end "second"
.ts tab configure "second" -text "Envelope" -window ".ts.tab3"

.ts insert end "third"
.ts tab configure "third" -text "Spectrogram" -window ".ts.tab4"

#frame .ts.tab4.x_win -container yes

source "env_display.tcl"



#pack .ts \
  #-fill both \
 # -expand yes \
 # -side top


#set spcrtrgm_wid [scan [winfo id .ts.tab4] %x]

set spcrtgrm_cmd "./DCOMPOSR $argv"


#puts "Value in the frame $argv $spcrtrgm_wid"




#toplevel .ts.tab4.x_win -use $argv

set cont [blt::container .ts.tab4.xwin -width 512 -height 512]
exec /bin/sh -c $spcrtgrm_cmd -into [ expr [winfo id .ts.tab4.xwin ]] &
pack $cont -fill both -expand yes 




frame .ts.tab1.ps -width 256
set graph .ts.tab1.ps.graph

set graph2 .ts.tab1.graph2
set graph3 .ts.tab2.graph3

set graph4 .ts.tab1.ps.graph4

blt::graph $graph -title "Ps"  -font Arial \
    -plotborderwidth 1 -plotrelief solid  -plotpadx 0 -plotpady 0 -width 512 \
    -plotbackground black


blt::graph $graph4 -title "Eig"  -font Arial \
    -plotborderwidth 1 -plotrelief solid  -plotpadx 0 -plotpady 0 -width 512 \
    -plotbackground black


$graph xaxis configure \
    -grid 1 \
    -loose true \
    -title "X Axis Label" 


$graph yaxis configure \
    -grid 1


$graph element create line3 -symbol none -color green -fill green2 \
    -linewidth 1 -outlinewidth 1 -pixels 2 -x xdata2 -y ps

$graph4 element create line4 -symbol none -color green -fill green2 \
    -linewidth 1 -outlinewidth 1 -pixels 2 -x xdata2 -y svd


blt::graph $graph2 -title "Ts"  -font Arial \
    -plotborderwidth 1 -plotrelief solid  -plotpadx 0 -plotpady 0 -width 1024

$graph2 xaxis configure \
    -loose true \
    -title "X Axis Label" 

$graph2 element create line4 -symbol none -color green4 -fill green2 \
    -linewidth 1 -outlinewidth 1 -pixels 2 -x xdata -y ts

#grid .ts.spec

grid .ts 
grid .ts.tab1.ps


grid $graph $graph4
grid $graph2
frame .ts.tab2.ctrl_fr
set listenport 2022

socket -server accept_connection $listenport


proc accept_connection {cid ip port} {
    puts "Incoming Connection from $ip:$port"
    fconfigure $cid -translation binary -buffering none
    fileevent $cid readable "readdata $cid"
}



proc readdata {cid} {

    global ts
    global ps
    global gs
    global FFT_SIZE
    global FFT_SIZE_DIV2

    #puts "Reading data in pipe"

    set bytes_in [ps binread $cid $FFT_SIZE_DIV2 -at 0 -format r4]
 
    #puts "Read $bytes_in bytes "

    if { $bytes_in == 0 } {

       puts "Socket closed by remote host"; close $cid; return
    }

    set bytes_in [svd binread $cid $FFT_SIZE_DIV2 -at 0 -format r4]

    #puts "Read $bytes_in bytes "

    if { $bytes_in == 0 } {

       puts "Socket closed by remote host"; close $cid; return
    }



    set bytes_in [ts binread $cid $FFT_SIZE -at 0 -format r4]
 
    #puts "Read $bytes_in bytes "

    if { $bytes_in == 0 } {

       puts "Socket closed by remote host."; close $cid; return
    }

}




Blt_ZoomStack $graph
Blt_Crosshairs $graph
Blt_ClosestPoint $graph


Blt_ZoomStack $graph4
Blt_Crosshairs $graph4
Blt_ClosestPoint $graph4


Blt_ZoomStack $graph2
Blt_Crosshairs $graph2
Blt_ClosestPoint $graph2
