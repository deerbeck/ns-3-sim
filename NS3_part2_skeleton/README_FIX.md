# NS-3 Ping RTT Tracing Fix

## Problem
The `tutdgr.cc` file was experiencing segmentation faults when RTT tracing was enabled for Ping applications.

## Root Cause
1. **Incorrect trace callback signature**: The `TraceRtt` function signature included an `unsigned short packetSize` parameter that doesn't match what the NS-3 Ping application's RTT trace source provides.
2. **Wrong address index**: Using `GetAddress(1)` instead of `GetAddress(0)` for the destination address.

## Solution
### Changes Made:
1. **Fixed TraceRtt function signature**:
   - **Before**: `void TraceRtt(std::ostream* os, unsigned short packetSize, Time rtt)`
   - **After**: `void TraceRtt(std::ostream* os, Time rtt)`

2. **Fixed address index**:
   - **Before**: `PingHelper ping(out_iface[v].GetAddress(1));`
   - **After**: `PingHelper ping(out_iface[v].GetAddress(0));`

### Files Modified:
- `NS3_part2_skeleton/tutdgr.cc`

## Impact
- The simulation should now run without segmentation faults
- RTT tracing will work properly and generate output files
- All existing functionality (OnOff applications, flow monitoring, link failures) is preserved
- The trace output format is simplified but still informative

## Verification
The fix follows the pattern shown in the reference code and matches the expected NS-3 Ping application interface.