#!/bin/bash
# filepath: /home/dd/CFNAgg-ndnSIM-2.9/debug_cfnagg.sh

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

# NS_LOG="CFNAggregatorApp=info:\
# CFNRootApp=info:\
# CFNProducerApp=info:\
# ndn-cxx.nfd.Forwarder:debug" \

NS_LOG="ndn.CFNAggregatorApp=info:\
ndn.CFNRootApp=info:\
ndn.CFNProducerApp=info:\
cfnagg-simulation=info" \
stdbuf -oL -eL ./waf --run cfnagg-lite-simulation \
  --command-template="gdb -x log_file/gdb_init.txt --args %s \
  --configFile=cfnagg-config.conf" \
  2>&1 | tee log_file/log_lite_output.txt

echo "Debugging completed. Check logs in log_file/log_lite_output.txt"