#!/bin/bash

# use max freq to boot up quickly, then limit
echo 1689600 | sudo tee /sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq
echo 1689600 | sudo tee /sys/devices/system/cpu/cpufreq/policy4/scaling_max_freq
