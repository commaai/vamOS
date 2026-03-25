#!/bin/bash
# Void Linux version - uses sv instead of systemctl

SSH_PARAM="/data/params/d/SshEnabled"
if [ -f "$SSH_PARAM" ] && [ "$(< $SSH_PARAM)" == "0" ]; then
  echo "Disabling SSH"
  sv down sshd
else
  # Default to enabled (fresh install or SshEnabled=1)
  echo "Enabling SSH"
  sv up sshd
fi
