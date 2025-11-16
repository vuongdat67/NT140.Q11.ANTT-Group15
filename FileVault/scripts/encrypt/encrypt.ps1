# D:\00-Project\FileVault\build\bin\Release\filevault.exe encrypt D:\00-Project\FileVault\test.txt
# D:\00-Project\FileVault\build\bin\Release\filevault.exe decrypt D:\00-Project\FileVault\test.txt.fv

# Test encryption
$password = "TestPassword123"
$confirmPassword = "TestPassword123"
# Use echo to pipe password to the CLI
($password, $confirmPassword) | .\build\bin\Release\filevault.exe encrypt test.txt -m standard
# if the file ex