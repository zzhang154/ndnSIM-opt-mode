#!/bin/bash
# filepath: /home/dd/ndnSIM-2.9-official/ndnSIM-2.9-official/debug_agg_mini.sh

# ./waf configure --disable-python --enable-examples -d optimized
# ./waf configure --disable-python --enable-examples -d debug

# 1. Create log directory
mkdir -p log_file

# 2. Create GDB init file inside log_file/
cat > log_file/gdb_init.txt << 'EOF'
set pagination off
set logging on
set logging file log_file/gdb.txt
set logging overwrite on

# Stop on various fatal signals:
handle SIGSEGV stop
handle SIGABRT stop
handle SIGILL stop
handle SIGBUS stop
handle SIGFPE stop
handle SIGINT stop
handle SIGTERM stop

# Whenever the program stops (like a crash), do a full backtrace
define hook-stop
  echo [Program stopped: printing stack trace]
  bt
end

run
quit
EOF

echo "Starting GDB session..."

# 3. Run simulation with GDB, referencing the init file in log_file/

# Configure logging as needed
NS_LOG="ndn.RootApp=info:\
ndn.AggregatorApp=info:\
ndn.LeafApp=info:\
agg-mini-simulation=info" \
stdbuf -oL -eL ./waf --run agg-mini \
  --command-template="gdb -x log_file/gdb_init.txt --args %s" \
  2>&1 | tee log_file/agg_mini_output.txt

echo "Debugging completed. Check logs in log_file/t.txt"agg_mini_outpu