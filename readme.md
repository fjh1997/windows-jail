# windows-jail
simple windows jail. to simulate runas /trustlevel and /user with pipeline to redirect stdout. just for fun.no guarantee
```
# run powershell as admin
# add user jail
net user jail jail /add
mkdir C:\Users\jail
# deny jail to read all files except files which disable inheritance(usually system folder like windows,Program Files (x86),etc)
icacls  d:\ /deny "jail:(OI)(CI)f"
icacls  c:\ /deny "jail:(OI)(CI)f"
icacls  e:\ /deny "jail:(OI)(CI)f"

#you can use command below to find out folders not effected by inheritance.Also it’s important to note that if the explicit permissions allow access, then the inherited permissions will never be checked.

dir c:\ -Directory -recurse|get-acl|where { $_.AreAccessRulesProtected}|select @{Name="Path" ;Expression={Convert-Path $_.Path}},AreAccessRulesProtected|format-table

# enable jail to read some programs and folders (for example python)
icacls "C:\Users\Administrator\AppData\Local\Programs\Python\Python310\" /grant "jail:(OI)(CI)(RX)"
# compaile programs to make jail
git clone https://github.com/fjh1997/windows-jail.git
cd windows-jail
msbuild RestrictShutdown.vcxproj
msbuild runasuser.vcxproj
cd telegram-evil-bot\bin-v143\x64\Debug
# run powershell as user jail with restrict permission,abolute path is needed
runasuser.exe jail jail  RestrictShutdown.exe  C:\WINDOWS\System32\WindowsPowerShell\v1.0\powershell.exe
```

## further works

I was thinking, create a user group, only grant read access to c:, Windows directory and other required folders to this user group, use SetTokenInformation(TokenGroups) to add the sid of this user group to the token of the process, use CreateRestrictedToken(UserGroup ) to restrict the process to this user group, is it simple to achieve the storage isolation of the process. Windows ACL check will check twice, the second time specifically to check if it is in the group being restricted.

[restricted-tokens](https://learn.microsoft.com/en-us/windows/win32/secauthz/restricted-tokens)
[safercomputetokenfromlevel](https://learn.microsoft.com/en-us/windows/win32/api/winsafer/nf-winsafer-safercomputetokenfromlevel)
