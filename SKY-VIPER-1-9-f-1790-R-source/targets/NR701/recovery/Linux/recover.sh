#!/bin/bash
tftp 192.168.0.1 <<ftpEOF
binary
put recovery.img
q
ftpEOF
