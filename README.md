# MyDriver

Istance of chapter 4 of Windows Kernel Programming book
This driver allows to change the thread priority

Usage: 
```
sc.exe create mydriver type= kernel binPath= C:\Path\To\MyDriver.sys
sc.exe start mydriver
Client.exe <ThreadID> <Priority>
```