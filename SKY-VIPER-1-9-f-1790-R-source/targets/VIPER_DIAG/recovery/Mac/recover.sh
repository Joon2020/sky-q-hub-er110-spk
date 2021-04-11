#!/bin/bash
/usr/bin/tftp -e 192.168.0.1 << ftpEOF
binary
put recovery.img
q
ftpEOF
