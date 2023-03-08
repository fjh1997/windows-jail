# windows-jail

run powershell as admin
```
# add user jail
net user jail jail /add
mkdir C:\Users\jail
# deny jail to read all files except files which disable inheritance(usually system folder like windows,Program Files (x86),etc)
icacls  d:\ /deny "jail:(OI)(CI)f"
icacls  c:\ /deny "jail:(OI)(CI)f"
icacls  e:\ /deny "jail:(OI)(CI)f"

#you can use command below to findout folders not effected by above command.it’s important to note that if the explicit permissions allow access, then the inherited permissions will never be checked.

dir c:\ -Directory -recurse|get-acl|where { $_.AreAccessRulesProtected}|select @{Name="Path" ;Expression={Convert-Path $_.Path}},AreAccessRulesProtected|format-table

# enable jail to read some programs and folders (for example python)
icacls "C:\Users\Administrator\AppData\Local\Programs\Python\Python310\" /grant "jail:(OI)(CI)(RX)"

# run powershell as user jail
runas  /user:jail powershell.exe
```
in new prompt
```
# run program in restrict mode
runas /trustlevel:0x20000 C:\Users\Administrator\AppData\Local\Programs\Python\Python310\python.exe
```