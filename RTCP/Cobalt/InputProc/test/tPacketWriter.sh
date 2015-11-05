#!/bin/sh

# Remove the shared memory region if the test crashes
trap "ipcrm -M 0x10000003 2>/dev/null || true" EXIT

./runctest.sh tPacketWriter
