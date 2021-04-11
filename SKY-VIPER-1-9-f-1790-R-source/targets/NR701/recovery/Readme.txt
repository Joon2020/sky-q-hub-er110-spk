Software recovery process for BSkyB broadband router.

Recovery process allows to recover the router from the non-working state. This may happen if the power was off during software upgrade.
This can only be done using a device connected to the router with Ethernet cable. Recovery via Wi-Fi is not possible. 

Recovery process will require:
- PC/Mac computer (Windows, MAC or Linux)
- Ethernet CAT5 cable (delivered with the router)
- recovery.img file (which can be found on CD delivered with the router) 

Recovery procedure:

1. Power off the router
2. Connect the PC/Mac computer to the router using Ethernet cable. Any Ethernet port in the router can be used. It's recommended for the recovery process to disconnect other devices from router.  
3. Press and hold WPS button.
4. While holding WPS buttton power on the router.
5. Router will enter recovery mode - all front panel LEDs will lit.
6. Release WPS button.
7. Wait until PC/Mac computer succesfully connects and obtains an IP address.

A. Browser recovery

8. Open the browser and navigate to http://192.168.0.1  
9. Recovery update page will open in the browser.
10. Press browse button. File selection window will appear.
11. Open recovery.img 
12. File selection window will close and "Software File Name" text box should show image name.
13. Press "Update Software button"
14. Recovery process will start which will take up to 2 minutes.
15. Router will then reboot automatically which completes recovery process.


B. Tftp transfer recovery (advanced)

- Windows
Check the letter of your CD-ROM, DVD-RW drive. It can be found in My computer. It should be D: E: etc.

8. Press Windows Start button
9. Press Run
10. write cmd in the newly opened window and press OK. Just cmd
11. A terminal will open
12. Put your CD-ROM drive letter and press Enter. For example write: 
D:
  and press enter
13. Write:
cd Windows
  and press enter
14. Write recovery.bat and press Enter 
15. Recovery process will start which will take up to 2 minutes.
16. Router will then reboot which completes recovery process.


- Linux
8. Open terminal
9. cd /media/RECOVERYCD/Linux
10 ./recovery.sh
11. Recovery process will start which will take up to 2 minutes.
12. Router will then reboot which completes recovery process.


- Mac
8. Open terminal
9. cd /Volumes/RECOVERYCD/Mac
10 ./recovery.sh
11. Recovery process will start which will take up to 2 minutes.
12. Router will then reboot which completes recovery process.


Troubleshooting.

7. If your PC/Mac device doesn't get an IP assigned try to assign a static IP address to it. This address must be from 192.168.0.2 to 192.168.0.255 range. You can check whether your computer sees the router by issuing ping 192.168.0.1 command.
