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

#you can use command below to find out folders not effected by inheritance.Also it’s important to note that if the explicit permissions allow access, then the inherited permissions will never be checked.

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
## further works

I was thinking, create a user group, only grant read access to c:, Windows directory and other required folders to this user group, use SetTokenInformation(TokenGroups) to add the sid of this user group to the token of the process, use CreateRestrictedToken(UserGroup ) to restrict the process to this user group, is it simple to achieve the storage isolation of the process. Windows ACL check will check twice, the second time specifically to check if it is in the group being restricted.

[see](https://learn.microsoft.com/en-us/windows/win32/secauthz/restricted-tokens)
