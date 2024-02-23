

blt::vector envelope($FFT_SIZE)

set env_disp .ts.tab3.env_graph

blt::graph $env_disp -title "Time Series Envelope"  -font Arial \
    -plotborderwidth 1 -plotrelief solid  -plotpadx 0 -plotpady 0 -width 1024 \
    -plotbackground black


$env_disp xaxis configure \
    -grid 1 \
    -loose true \
    -title "X Axis Label"


for {set i 0} {$i<$FFT_SIZE} {incr i 1} {
  set envelope($i) 0
}


$env_disp element create line4 -symbol none -color green4 -fill green2 \
    -linewidth 1 -outlinewidth 1 -pixels 2 -x xdata -y envelope


$env_disp yaxis configure \
    -grid 1





set env_port 2023

socket -server accept_env $env_port

proc accept_env {cid ip port} {
    puts "Incoming Connection from $ip:$port"
    fconfigure $cid -translation binary -buffering none
    fileevent $cid readable "readEnv $cid"
}

proc readEnv { cid } {

global envelope
global FFT_SIZE

    set bytes_in [envelope binread $cid $FFT_SIZE -at 0 -format r4]

    #puts "Read $bytes_in bytes "

    if { $bytes_in == 0 } {

       puts "Socket closed by remote host."; close $cid; return
    }

}



grid $env_disp


Blt_ZoomStack $env_disp
Blt_Crosshairs $env_disp
Blt_ClosestPoint $env_disp
