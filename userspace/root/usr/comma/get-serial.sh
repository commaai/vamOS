#!/bin/sh

sed -n 's/.*androidboot.serialno=\([^ ]*\).*/\1/p' /proc/cmdline
