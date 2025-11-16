# Test decryption
$password = "TestPassword123"

# Use echo to pipe password to the CLI
$password | .\build\bin\Release\filevault.exe decrypt test.txt.fv -o test_decrypted.txt
