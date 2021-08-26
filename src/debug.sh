#! /bin/bash
if ! make -j$(nproc); then
	exit 1
fi

DEBUG_DIR=/tmp/dragonblocks_alpha_debug_$$/
mkdir $DEBUG_DIR

echo "singleplayer" > $DEBUG_DIR/name

COMMON="set confirm off
handle SIGTERM nostop print pass
handle SIGPIPE nostop noprint pass
set height 0
set \$_exitcode 1
define hook-stop
    if \$_exitcode == 0
        quit
    end
end
"

echo "$COMMON
run ::1 4000 < $DEBUG_DIR/name
" > $DEBUG_DIR/client_script

echo "$COMMON
run 4000
" > $DEBUG_DIR/server_script

konsole -e bash -c "
	echo \$\$ > $DEBUG_DIR/server_pid
	exec gdb --command $DEBUG_DIR/server_script ./DragonblocksServer
" & sleep 0.5

gdb --command $DEBUG_DIR/client_script ./Dragonblocks

kill `cat $DEBUG_DIR/server_pid`

rm -rf $DEBUG_DIR
