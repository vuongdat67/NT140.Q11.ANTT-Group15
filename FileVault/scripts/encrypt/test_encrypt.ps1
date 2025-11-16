# Test encryption with echo password
$password = "TestPassword123"
$confirmPassword = "TestPassword123"

# Use echo to pipe password to the CLI
($password, $confirmPassword) | .\build\bin\Release\filevault.exe encrypt test.txt -m standard
