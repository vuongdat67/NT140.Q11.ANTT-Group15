import * as vscode from 'vscode';
import * as path from 'path';
import * as child_process from 'child_process';
import * as fs from 'fs';

let outputChannel: vscode.OutputChannel;

export function activate(context: vscode.ExtensionContext) {
    outputChannel = vscode.window.createOutputChannel('FileVault');
    
    // Register commands
    context.subscriptions.push(
        vscode.commands.registerCommand('filevault.encrypt', encryptFile),
        vscode.commands.registerCommand('filevault.decrypt', decryptFile),
        vscode.commands.registerCommand('filevault.info', showFileInfo),
        vscode.commands.registerCommand('filevault.keygen', generateKeyPair),
        vscode.commands.registerCommand('filevault.hash', calculateHash),
        vscode.commands.registerCommand('filevault.benchmark', runBenchmark),
        vscode.commands.registerCommand('filevault.list', listAlgorithms),
        vscode.commands.registerCommand('filevault.compress', compressFile),
        vscode.commands.registerCommand('filevault.decompress', decompressFile),
        vscode.commands.registerCommand('filevault.archiveCreate', createArchive),
        vscode.commands.registerCommand('filevault.archiveExtract', extractArchive),
        vscode.commands.registerCommand('filevault.stegoEmbed', stegoEmbed),
        vscode.commands.registerCommand('filevault.stegoExtract', stegoExtract),
        vscode.commands.registerCommand('filevault.stegoCapacity', stegoCapacity),
        vscode.commands.registerCommand('filevault.setExecutablePath', setExecutablePath)
    );

    // Auto-detect executable on activation
    autoDetectExecutable();
    
    outputChannel.appendLine('FileVault extension activated');
}

export function deactivate() {
    outputChannel.dispose();
}

// Get FileVault executable path
function getExecutablePath(): string {
    const config = vscode.workspace.getConfiguration('filevault');
    const configPath = config.get<string>('executablePath');
    
    if (configPath && configPath.length > 0 && fs.existsSync(configPath)) {
        return configPath;
    }
    
    // Try common locations on Windows
    const possiblePaths = [
        // Current workspace
        path.join(vscode.workspace.workspaceFolders?.[0]?.uri.fsPath || '', 'build', 'build', 'Release', 'bin', 'release', 'filevault.exe'),
        path.join(vscode.workspace.workspaceFolders?.[0]?.uri.fsPath || '', 'dist', 'filevault.exe'),
        // Common install locations
        path.join(process.env.PROGRAMFILES || 'C:\\Program Files', 'FileVault', 'filevault.exe'),
        path.join(process.env.LOCALAPPDATA || '', 'FileVault', 'filevault.exe'),
        path.join(process.env.USERPROFILE || '', 'filevault', 'filevault.exe'),
        // Linux/macOS
        '/usr/local/bin/filevault',
        '/usr/bin/filevault',
        path.join(process.env.HOME || '', '.local', 'bin', 'filevault')
    ];
    
    for (const p of possiblePaths) {
        try {
            if (p && fs.existsSync(p)) {
                outputChannel.appendLine(`Found FileVault at: ${p}`);
                return p;
            }
        } catch {
            // Ignore
        }
    }
    
    // Not found - prompt user
    vscode.window.showWarningMessage(
        'FileVault executable not found. Please configure the path in settings.',
        'Open Settings'
    ).then(selection => {
        if (selection === 'Open Settings') {
            vscode.commands.executeCommand('workbench.action.openSettings', 'filevault.executablePath');
        }
    });
    
    return configPath || 'filevault';
}

// Auto-detect and save executable path
async function autoDetectExecutable(): Promise<void> {
    const config = vscode.workspace.getConfiguration('filevault');
    const configPath = config.get<string>('executablePath');
    
    // Already configured
    if (configPath && configPath.length > 0 && fs.existsSync(configPath)) {
        outputChannel.appendLine(`FileVault configured at: ${configPath}`);
        return;
    }
    
    // Try to find executable
    const isWin = process.platform === 'win32';
    const exeName = isWin ? 'filevault.exe' : 'filevault';
    
    const possiblePaths = [
        // Current workspace build paths
        path.join(vscode.workspace.workspaceFolders?.[0]?.uri.fsPath || '', 'build', 'build', 'Release', 'bin', 'release', exeName),
        path.join(vscode.workspace.workspaceFolders?.[0]?.uri.fsPath || '', 'build', 'build', 'Release', 'bin', exeName),
        path.join(vscode.workspace.workspaceFolders?.[0]?.uri.fsPath || '', 'build', 'Release', exeName),
        path.join(vscode.workspace.workspaceFolders?.[0]?.uri.fsPath || '', 'build', exeName),
        path.join(vscode.workspace.workspaceFolders?.[0]?.uri.fsPath || '', 'dist', exeName),
        path.join(vscode.workspace.workspaceFolders?.[0]?.uri.fsPath || '', 'bin', exeName),
        // Common install locations
        path.join(process.env.PROGRAMFILES || 'C:\\Program Files', 'FileVault', exeName),
        path.join(process.env.LOCALAPPDATA || '', 'FileVault', exeName),
        path.join(process.env.USERPROFILE || '', 'filevault', exeName),
        // Linux/macOS
        '/usr/local/bin/filevault',
        '/usr/bin/filevault',
        path.join(process.env.HOME || '', '.local', 'bin', 'filevault')
    ];
    
    for (const p of possiblePaths) {
        try {
            if (p && fs.existsSync(p)) {
                // Found! Save to config
                await config.update('executablePath', p, vscode.ConfigurationTarget.Global);
                outputChannel.appendLine(`Auto-detected FileVault at: ${p}`);
                vscode.window.showInformationMessage(`FileVault found: ${path.basename(path.dirname(p))}/${path.basename(p)}`);
                return;
            }
        } catch {
            // Ignore
        }
    }
    
    // Not found - show notification
    outputChannel.appendLine('FileVault executable not found in common locations');
}

// Strip ANSI escape codes from output
function stripAnsiCodes(text: string): string {
    // Remove ANSI color codes and other escape sequences
    return text.replace(/\x1b\[[0-9;]*[a-zA-Z]/g, '');
}

// Run FileVault command
function runFileVault(args: string[]): Promise<{ stdout: string; stderr: string }> {
    return new Promise((resolve, reject) => {
        const executable = getExecutablePath();
        outputChannel.appendLine(`> "${executable}" ${args.map(a => a.includes(' ') ? `"${a}"` : a).join(' ')}`);
        
        // Use shell: false for proper argument passing
        // spawn() with shell:false requires proper setup on Windows
        const childProcess = child_process.spawn(executable, args, {
            shell: false,
            windowsHide: true,
            stdio: ['ignore', 'pipe', 'pipe'],  // Explicit stdio
            env: process.env  // Inherit environment from Node.js process
        });
        
        let stdout = '';
        let stderr = '';
        
        childProcess.stdout.on('data', (data: Buffer) => {
            const text = data.toString();
            stdout += text;
            // Strip ANSI codes before displaying in output channel
            outputChannel.append(stripAnsiCodes(text));
        });
        
        childProcess.stderr.on('data', (data: Buffer) => {
            const text = data.toString();
            stderr += text;
            // Strip ANSI codes before displaying in output channel
            outputChannel.append(stripAnsiCodes(text));
        });
        
        childProcess.on('close', (code: number | null) => {
            if (code === 0) {
                resolve({ 
                    stdout: stripAnsiCodes(stdout), 
                    stderr: stripAnsiCodes(stderr) 
                });
            } else {
                reject(new Error(stripAnsiCodes(stderr) || `Process exited with code ${code}`));
            }
        });
        
        childProcess.on('error', (error: Error) => {
            reject(error);
        });
    });
}

// Encrypt file
async function encryptFile(uri?: vscode.Uri) {
    try {
        // Get file path
        let filePath: string;
        if (uri) {
            filePath = uri.fsPath;
        } else {
            const fileUri = await vscode.window.showOpenDialog({
                canSelectMany: false,
                title: 'Select file to encrypt'
            });
            if (!fileUri || fileUri.length === 0) {
                return;
            }
            filePath = fileUri[0].fsPath;
        }
        
        // Get config and choose mode or custom
        const config = vscode.workspace.getConfiguration('filevault');
        
        // First ask: use preset mode or custom?
        const modeChoice = await vscode.window.showQuickPick([
            { label: '$(rocket) Basic Mode', description: 'Fast encryption, good security (casual use)', value: 'basic' },
            { label: '$(shield) Standard Mode', description: 'Balanced security & performance (recommended)', value: 'standard' },
            { label: '$(lock) Advanced Mode', description: 'Maximum security, slower (high-value data)', value: 'advanced' },
            { label: '$(settings-gear) Custom', description: 'Choose algorithm and security manually', value: 'custom' }
        ], {
            placeHolder: 'Select encryption mode',
            title: 'Encryption Mode'
        });
        
        if (!modeChoice) {
            return;
        }
        
        let algorithm: any = null;
        let security: any = null;
        let useMode: string | null = null;
        
        if (modeChoice.value !== 'custom') {
            // Use mode preset
            useMode = modeChoice.value;
        } else {
            // Custom: ask for algorithm and security
            const algorithms = [
                // AEAD (Recommended)
                { label: '$(shield) AES-256-GCM', description: 'Recommended - NIST Standard', value: 'aes-256-gcm' },
                { label: '$(shield) AES-128-GCM', description: 'Fast - NIST Standard', value: 'aes-128-gcm' },
                { label: '$(shield) ChaCha20-Poly1305', description: 'Fast software implementation', value: 'chacha20-poly1305' },
                { label: '$(shield) Serpent-256-GCM', description: 'Maximum security - AES finalist', value: 'serpent-256-gcm' },
                { label: '$(shield) Twofish-256-GCM', description: 'AES finalist', value: 'twofish-256-gcm' },
                { label: '$(shield) Camellia-256-GCM', description: 'Japan (CRYPTREC)', value: 'camellia-256-gcm' },
                { label: '$(shield) ARIA-256-GCM', description: 'Korea (KS X 1213)', value: 'aria-256-gcm' },
                { label: '$(shield) SM4-GCM', description: 'China (GB/T 32907)', value: 'sm4-gcm' },
                // Post-Quantum
                { label: '$(rocket) Kyber-1024-Hybrid', description: 'Post-Quantum - NIST Level 5', value: 'kyber-1024-hybrid' },
                { label: '$(rocket) Kyber-768-Hybrid', description: 'Post-Quantum - NIST Level 3', value: 'kyber-768-hybrid' },
                { label: '$(rocket) Kyber-512-Hybrid', description: 'Post-Quantum - NIST Level 1', value: 'kyber-512-hybrid' },
                // Block modes
                { label: '$(lock) AES-256-CBC', description: 'Block mode + HMAC', value: 'aes-256-cbc' },
                { label: '$(lock) AES-256-CTR', description: 'Counter mode', value: 'aes-256-ctr' },
                { label: '$(lock) AES-256-XTS', description: 'Disk encryption mode', value: 'aes-256-xts' }
            ];
            
            algorithm = await vscode.window.showQuickPick(algorithms, {
                placeHolder: 'Select encryption algorithm',
                title: 'Encryption Algorithm'
            });
            
            if (!algorithm) {
                return;
            }
            
            // Get security level
            const securityLevels = [
                { label: 'Medium', description: 'Recommended - balanced security/performance', value: 'medium' },
                { label: 'Strong', description: 'Higher security, slower', value: 'strong' },
                { label: 'Paranoid', description: 'Maximum security, slowest', value: 'paranoid' },
                { label: 'Weak', description: 'Fast, for testing only', value: 'weak' }
            ];
            
            security = await vscode.window.showQuickPick(securityLevels, {
                placeHolder: 'Select security level',
                title: 'Security Level'
            });
            
            if (!security) {
                return;
            }
        }
        
        // Get password
        const password = await vscode.window.showInputBox({
            prompt: 'Enter encryption password',
            password: true,
            placeHolder: 'Password',
            validateInput: (value) => {
                if (!value || value.length < 8) {
                    return 'Password must be at least 8 characters';
                }
                return null;
            }
        });
        
        if (!password) {
            return;
        }
        
        // Confirm password
        const confirmPassword = await vscode.window.showInputBox({
            prompt: 'Confirm password',
            password: true,
            placeHolder: 'Confirm Password'
        });
        
        if (password !== confirmPassword) {
            vscode.window.showErrorMessage('Passwords do not match');
            return;
        }
        
        // Run encryption
        const outputPath = filePath + '.fvlt';
        
        await vscode.window.withProgress({
            location: vscode.ProgressLocation.Notification,
            title: 'Encrypting file...',
            cancellable: false
        }, async () => {
            const args = ['encrypt', filePath, outputPath];
            
            if (useMode) {
                // Use mode preset
                args.push('-m', useMode);
            } else {
                // Use custom algorithm and security
                args.push('-a', algorithm.value);
                args.push('-s', security.value);
            }
            
            args.push('-p', password);
            
            await runFileVault(args);
        });
        
        // Verify output file was created
        if (!fs.existsSync(outputPath)) {
            throw new Error('Encryption failed: output file not created');
        }
        
        const showNotifications = config.get<boolean>('showNotifications', true);
        if (showNotifications) {
            const result = await vscode.window.showInformationMessage(
                `File encrypted successfully: ${path.basename(outputPath)}`,
                'Open Folder',
                'Delete Original'
            );
            
            if (result === 'Open Folder') {
                vscode.commands.executeCommand('revealFileInOS', vscode.Uri.file(outputPath));
            } else if (result === 'Delete Original') {
                fs.unlinkSync(filePath);
                vscode.window.showInformationMessage('Original file deleted');
            }
        }
        
    } catch (error: any) {
        vscode.window.showErrorMessage(`Encryption failed: ${error.message}`);
        outputChannel.appendLine(`Error: ${error.message}`);
    }
}

// Decrypt file
async function decryptFile(uri?: vscode.Uri) {
    try {
        // Get file path
        let filePath: string;
        if (uri) {
            filePath = uri.fsPath;
        } else {
            const fileUri = await vscode.window.showOpenDialog({
                canSelectMany: false,
                title: 'Select file to decrypt',
                filters: {
                    'FileVault Encrypted': ['fvlt']
                }
            });
            if (!fileUri || fileUri.length === 0) {
                return;
            }
            filePath = fileUri[0].fsPath;
        }
        
        // Get password
        const password = await vscode.window.showInputBox({
            prompt: 'Enter decryption password',
            password: true,
            placeHolder: 'Password'
        });
        
        if (!password) {
            return;
        }
        
        // Determine output path
        let outputPath = filePath.replace(/\.fvlt$/, '');
        if (outputPath === filePath) {
            outputPath = filePath + '.decrypted';
        }
        
        // Check if output exists
        if (fs.existsSync(outputPath)) {
            const overwrite = await vscode.window.showWarningMessage(
                `File ${path.basename(outputPath)} already exists. Overwrite?`,
                'Yes', 'No'
            );
            if (overwrite !== 'Yes') {
                return;
            }
        }
        
        // Run decryption
        await vscode.window.withProgress({
            location: vscode.ProgressLocation.Notification,
            title: 'Decrypting file...',
            cancellable: false
        }, async () => {
            await runFileVault([
                'decrypt',
                filePath,
                outputPath,
                '-p', password
            ]);
        });
        
        // Verify output file was created
        if (!fs.existsSync(outputPath)) {
            throw new Error('Decryption failed: output file not created');
        }
        
        const config = vscode.workspace.getConfiguration('filevault');
        const showNotifications = config.get<boolean>('showNotifications', true);
        if (showNotifications) {
            const result = await vscode.window.showInformationMessage(
                `File decrypted successfully: ${path.basename(outputPath)}`,
                'Open File',
                'Open Folder'
            );
            
            if (result === 'Open File') {
                vscode.window.showTextDocument(vscode.Uri.file(outputPath));
            } else if (result === 'Open Folder') {
                vscode.commands.executeCommand('revealFileInOS', vscode.Uri.file(outputPath));
            }
        }
        
    } catch (error: any) {
        vscode.window.showErrorMessage(`Decryption failed: ${error.message}`);
        outputChannel.appendLine(`Error: ${error.message}`);
    }
}

// Show file info
async function showFileInfo(uri?: vscode.Uri) {
    try {
        let filePath: string;
        if (uri) {
            filePath = uri.fsPath;
        } else {
            const fileUri = await vscode.window.showOpenDialog({
                canSelectMany: false,
                title: 'Select file to inspect',
                filters: {
                    'FileVault Encrypted': ['fvlt'],
                    'All Files': ['*']
                }
            });
            if (!fileUri || fileUri.length === 0) {
                return;
            }
            filePath = fileUri[0].fsPath;
        }
        
        // Check if it's an encrypted file
        const isEncrypted = filePath.endsWith('.fvlt');
        
        if (isEncrypted) {
            // Show encrypted file metadata
            const result = await runFileVault(['info', filePath]);
            outputChannel.show();
            
            vscode.window.showInformationMessage(
                `File: ${path.basename(filePath)} - See Output panel for details`,
                'Show Output'
            ).then(result => {
                if (result === 'Show Output') {
                    outputChannel.show();
                }
            });
        } else {
            // Show file in different formats
            const formats = [
                { label: 'Hexadecimal', description: 'View file as hex dump', value: 'hex' },
                { label: 'Binary', description: 'View file as binary', value: 'binary' },
                { label: 'Base64', description: 'View file as base64', value: 'base64' },
                { label: 'File Stats', description: 'Show file size, type, permissions', value: 'stats' }
            ];
            
            const format = await vscode.window.showQuickPick(formats, {
                placeHolder: 'Select view format',
                title: 'File Viewer'
            });
            
            if (!format) {
                return;
            }
            
            const fileBuffer = fs.readFileSync(filePath);
            const fileStats = fs.statSync(filePath);
            const maxBytes = 1024; // Show first 1KB for large files
            const displayBuffer = fileBuffer.length > maxBytes ? fileBuffer.subarray(0, maxBytes) : fileBuffer;
            
            outputChannel.show();
            outputChannel.appendLine(`\n${'='.repeat(60)}`);
            outputChannel.appendLine(`File: ${path.basename(filePath)}`);
            outputChannel.appendLine(`Path: ${filePath}`);
            outputChannel.appendLine(`Size: ${fileStats.size} bytes`);
            
            switch (format.value) {
                case 'hex':
                    outputChannel.appendLine(`Format: Hexadecimal${fileBuffer.length > maxBytes ? ` (showing first ${maxBytes} bytes)` : ''}`);
                    outputChannel.appendLine(`${'='.repeat(60)}\n`);
                    
                    // Format: offset | hex bytes | ASCII
                    for (let i = 0; i < displayBuffer.length; i += 16) {
                        const chunk = displayBuffer.subarray(i, Math.min(i + 16, displayBuffer.length));
                        const offset = i.toString(16).padStart(8, '0');
                        const hexPart = Array.from(chunk).map(b => b.toString(16).padStart(2, '0')).join(' ');
                        const asciiPart = Array.from(chunk).map(b => (b >= 32 && b < 127) ? String.fromCharCode(b) : '.').join('');
                        outputChannel.appendLine(`${offset}  ${hexPart.padEnd(48, ' ')}  |${asciiPart}|`);
                    }
                    break;
                
                case 'binary':
                    outputChannel.appendLine(`Format: Binary${fileBuffer.length > maxBytes ? ` (showing first ${maxBytes} bytes)` : ''}`);
                    outputChannel.appendLine(`${'='.repeat(60)}\n`);
                    
                    for (let i = 0; i < displayBuffer.length; i += 8) {
                        const chunk = displayBuffer.subarray(i, Math.min(i + 8, displayBuffer.length));
                        const offset = i.toString(16).padStart(8, '0');
                        const binaryPart = Array.from(chunk).map(b => b.toString(2).padStart(8, '0')).join(' ');
                        outputChannel.appendLine(`${offset}  ${binaryPart}`);
                    }
                    break;
                
                case 'base64':
                    outputChannel.appendLine(`Format: Base64`);
                    outputChannel.appendLine(`${'='.repeat(60)}\n`);
                    const base64 = fileBuffer.toString('base64');
                    // Wrap at 76 characters (MIME standard)
                    for (let i = 0; i < base64.length; i += 76) {
                        outputChannel.appendLine(base64.substring(i, i + 76));
                    }
                    break;
                
                case 'stats':
                    outputChannel.appendLine(`Format: File Statistics`);
                    outputChannel.appendLine(`${'='.repeat(60)}\n`);
                    outputChannel.appendLine(`Created:  ${fileStats.birthtime.toISOString()}`);
                    outputChannel.appendLine(`Modified: ${fileStats.mtime.toISOString()}`);
                    outputChannel.appendLine(`Accessed: ${fileStats.atime.toISOString()}`);
                    outputChannel.appendLine(`Mode:     ${fileStats.mode.toString(8)}`);
                    outputChannel.appendLine(`IsFile:   ${fileStats.isFile()}`);
                    outputChannel.appendLine(`IsDir:    ${fileStats.isDirectory()}`);
                    
                    // Try to detect file type by magic bytes
                    if (fileBuffer.length >= 4) {
                        const magic = Array.from(fileBuffer.subarray(0, 4)).map(b => b.toString(16).padStart(2, '0')).join('');
                        outputChannel.appendLine(`Magic:    ${magic}`);
                        
                        const fileTypes: {[key: string]: string} = {
                            '89504e47': 'PNG image',
                            'ffd8ffe0': 'JPEG image (JFIF)',
                            'ffd8ffe1': 'JPEG image (EXIF)',
                            '47494638': 'GIF image',
                            '424d': 'BMP image',
                            '504b0304': 'ZIP archive',
                            '504b0506': 'ZIP archive (empty)',
                            '504b0708': 'ZIP archive (spanned)',
                            '25504446': 'PDF document',
                            'd0cf11e0': 'Microsoft Office document',
                            '52617221': 'RAR archive',
                            '1f8b08': 'GZIP archive',
                            '4d5a': 'Windows executable',
                            '7f454c46': 'ELF executable',
                            'cafebabe': 'Java class file'
                        };
                        
                        for (const [sig, type] of Object.entries(fileTypes)) {
                            if (magic.startsWith(sig)) {
                                outputChannel.appendLine(`Type:     ${type}`);
                                break;
                            }
                        }
                    }
                    break;
            }
            
            outputChannel.appendLine(`\n${'='.repeat(60)}`);
            
            if (format.value !== 'stats') {
                const action = await vscode.window.showInformationMessage(
                    `File displayed as ${format.label}`,
                    'Copy to Clipboard',
                    'Show Output'
                );
                
                if (action === 'Copy to Clipboard') {
                    let copyText = '';
                    switch (format.value) {
                        case 'hex':
                            copyText = Array.from(fileBuffer).map(b => b.toString(16).padStart(2, '0')).join('');
                            break;
                        case 'binary':
                            copyText = Array.from(fileBuffer).map(b => b.toString(2).padStart(8, '0')).join('');
                            break;
                        case 'base64':
                            copyText = fileBuffer.toString('base64');
                            break;
                    }
                    await vscode.env.clipboard.writeText(copyText);
                    vscode.window.showInformationMessage('Copied to clipboard');
                } else if (action === 'Show Output') {
                    outputChannel.show();
                }
            }
        }
        
    } catch (error: any) {
        vscode.window.showErrorMessage(`Failed to show file info: ${error.message}`);
    }
}

// Generate key pair
async function generateKeyPair() {
    try {
        const algorithms = [
            { label: 'RSA-4096', description: 'Traditional asymmetric encryption', value: 'rsa-4096' },
            { label: 'RSA-3072', description: 'Faster RSA', value: 'rsa-3072' },
            { label: 'Kyber-1024', description: 'Post-Quantum KEM - NIST Level 5', value: 'kyber-1024' },
            { label: 'Kyber-768', description: 'Post-Quantum KEM - NIST Level 3', value: 'kyber-768' },
            { label: 'Dilithium-5', description: 'Post-Quantum Signatures - NIST Level 5', value: 'dilithium-5' },
            { label: 'Dilithium-3', description: 'Post-Quantum Signatures - NIST Level 3', value: 'dilithium-3' },
            { label: 'ECC-P521', description: 'Elliptic Curve - 256-bit security', value: 'ecc-p521' },
            { label: 'ECC-P384', description: 'Elliptic Curve - 192-bit security', value: 'ecc-p384' }
        ];
        
        const algorithm = await vscode.window.showQuickPick(algorithms, {
            placeHolder: 'Select key algorithm',
            title: 'Key Generation'
        });
        
        if (!algorithm) {
            return;
        }
        
        // Get output folder
        const folder = await vscode.window.showOpenDialog({
            canSelectFiles: false,
            canSelectFolders: true,
            canSelectMany: false,
            title: 'Select folder to save keys'
        });
        
        if (!folder || folder.length === 0) {
            return;
        }
        
        // Get key name
        const keyName = await vscode.window.showInputBox({
            prompt: 'Enter key name',
            placeHolder: 'my-key',
            value: algorithm.value.toLowerCase().replace('-', '_')
        });
        
        if (!keyName) {
            return;
        }
        
        const outputPath = path.join(folder[0].fsPath, keyName);
        
        await vscode.window.withProgress({
            location: vscode.ProgressLocation.Notification,
            title: `Generating ${algorithm.label} key pair...`,
            cancellable: false
        }, async () => {
            await runFileVault([
                'keygen',
                '-a', algorithm.value,
                '-o', outputPath
            ]);
        });
        
        vscode.window.showInformationMessage(
            `Key pair generated: ${keyName}.pub and ${keyName}.key`,
            'Open Folder'
        ).then(result => {
            if (result === 'Open Folder') {
                vscode.commands.executeCommand('revealFileInOS', folder[0]);
            }
        });
        
    } catch (error: any) {
        vscode.window.showErrorMessage(`Key generation failed: ${error.message}`);
    }
}

// Calculate hash
async function calculateHash(uri?: vscode.Uri) {
    try {
        let filePath: string;
        if (uri) {
            filePath = uri.fsPath;
        } else {
            const fileUri = await vscode.window.showOpenDialog({
                canSelectMany: false,
                title: 'Select file to hash'
            });
            if (!fileUri || fileUri.length === 0) {
                return;
            }
            filePath = fileUri[0].fsPath;
        }
        
        const algorithms = [
            { label: 'SHA-256', description: 'Standard hash', value: 'sha256' },
            { label: 'SHA-512', description: 'Stronger hash', value: 'sha512' },
            { label: 'SHA3-256', description: 'NIST SHA-3', value: 'sha3-256' },
            { label: 'BLAKE2b', description: 'Modern, fastest', value: 'blake2b' }
        ];
        
        const algorithm = await vscode.window.showQuickPick(algorithms, {
            placeHolder: 'Select hash algorithm',
            title: 'Hash Algorithm'
        });
        
        if (!algorithm) {
            return;
        }
        
        // Select output format
        const formats = [
            { label: 'Hexadecimal', description: 'Standard hex format (default)', value: 'hex' },
            { label: 'Base64', description: 'Compact base64 encoding', value: 'base64' },
            { label: 'Binary', description: 'Binary representation', value: 'binary' }
        ];
        
        const format = await vscode.window.showQuickPick(formats, {
            placeHolder: 'Select output format',
            title: 'Hash Output Format'
        });
        
        if (!format) {
            return;
        }
        
        const args = ['hash', filePath, '-a', algorithm.value];
        if (format.value !== 'hex') {
            args.push('--format', format.value);
        }
        
        const result = await runFileVault(args);
        
        // Extract hash from output
        const hashMatch = result.stdout.match(/^([A-Za-z0-9+/=]+)/m);
        const hash = hashMatch ? hashMatch[1] : result.stdout.trim();
        
        const action = await vscode.window.showInformationMessage(
            `${algorithm.label}: ${hash.substring(0, 16)}...`,
            'Copy Hash',
            'Show Full'
        );
        
        if (action === 'Copy Hash') {
            vscode.env.clipboard.writeText(hash);
            vscode.window.showInformationMessage('Hash copied to clipboard');
        } else if (action === 'Show Full') {
            outputChannel.appendLine(`\n${algorithm.label} hash of ${path.basename(filePath)}:`);
            outputChannel.appendLine(hash);
            outputChannel.show();
        }
        
    } catch (error: any) {
        vscode.window.showErrorMessage(`Hash calculation failed: ${error.message}`);
    }
}

// Run benchmark
async function runBenchmark() {
    try {
        const benchmarkTypes = [
            { label: 'All Algorithms', description: 'Complete benchmark', value: '' },
            { label: 'Symmetric Only', description: 'AES, ChaCha20, etc.', value: '--symmetric' },
            { label: 'Asymmetric Only', description: 'RSA, ECC', value: '--asymmetric' },
            { label: 'Post-Quantum Only', description: 'Kyber, Dilithium', value: '--pqc' }
        ];
        
        const benchmarkType = await vscode.window.showQuickPick(benchmarkTypes, {
            placeHolder: 'Select benchmark type',
            title: 'Benchmark'
        });
        
        if (!benchmarkType) {
            return;
        }
        
        outputChannel.show();
        outputChannel.appendLine('\n========================================');
        outputChannel.appendLine('Running FileVault Benchmark...');
        outputChannel.appendLine('========================================\n');
        
        const args = ['benchmark'];
        if (benchmarkType.value) {
            args.push(benchmarkType.value);
        }
        
        await vscode.window.withProgress({
            location: vscode.ProgressLocation.Notification,
            title: 'Running benchmark...',
            cancellable: false
        }, async () => {
            await runFileVault(args);
        });
        
        vscode.window.showInformationMessage('Benchmark completed - see Output panel');
        
    } catch (error: any) {
        vscode.window.showErrorMessage(`Benchmark failed: ${error.message}`);
    }
}

// List all algorithms
async function listAlgorithms() {
    try {
        outputChannel.show();
        outputChannel.appendLine('\n========================================');
        outputChannel.appendLine('FileVault - Available Algorithms');
        outputChannel.appendLine('========================================\n');
        
        await vscode.window.withProgress({
            location: vscode.ProgressLocation.Notification,
            title: 'Loading algorithms...',
            cancellable: false
        }, async () => {
            await runFileVault(['list']);
        });
        
        vscode.window.showInformationMessage('See Output panel for full algorithm list');
        
    } catch (error: any) {
        vscode.window.showErrorMessage(`Failed to list algorithms: ${error.message}`);
    }
}

// Compress file
async function compressFile(uri?: vscode.Uri) {
    try {
        let filePath: string;
        if (uri) {
            filePath = uri.fsPath;
        } else {
            const fileUri = await vscode.window.showOpenDialog({
                canSelectMany: false,
                title: 'Select file to compress'
            });
            if (!fileUri || fileUri.length === 0) {
                return;
            }
            filePath = fileUri[0].fsPath;
        }
        
        const algorithms = [
            { label: 'LZMA', description: 'Best compression ratio, slower', value: 'lzma' },
            { label: 'BZIP2', description: '⚠️ Temporarily disabled - use ZLIB or LZMA', value: 'bzip2' },
            { label: 'ZLIB', description: 'Fast compression', value: 'zlib' }
        ];
        
        const algorithm = await vscode.window.showQuickPick(algorithms, {
            placeHolder: 'Select compression algorithm',
            title: 'Compression Algorithm'
        });
        
        if (!algorithm) {
            return;
        }
        
        const outputPath = filePath + '.compressed';
        
        await vscode.window.withProgress({
            location: vscode.ProgressLocation.Notification,
            title: 'Compressing file...',
            cancellable: false
        }, async () => {
            await runFileVault([
                'compress',
                filePath,
                '-a', algorithm.value,
                '-o', outputPath
            ]);
        });
        
        vscode.window.showInformationMessage(
            `File compressed: ${path.basename(outputPath)}`,
            'Open Folder'
        ).then((result: any) => {
            if (result === 'Open Folder') {
                vscode.commands.executeCommand('revealFileInOS', vscode.Uri.file(outputPath));
            }
        });
        
    } catch (error: any) {
        vscode.window.showErrorMessage(`Compression failed: ${error.message}`);
    }
}

// Create archive
async function createArchive() {
    try {
        const files = await vscode.window.showOpenDialog({
            canSelectMany: true,
            title: 'Select files to archive'
        });
        
        if (!files || files.length === 0) {
            return;
        }
        
        // MUST ask password FIRST to avoid CLI hanging on stdin
        const password = await vscode.window.showInputBox({
            prompt: 'Enter archive password (required - minimum 6 characters)',
            password: true,
            validateInput: (value) => {
                if (!value || value.length < 6) {
                    return 'Password must be at least 6 characters';
                }
                return null;
            }
        });
        
        if (!password) {
            vscode.window.showWarningMessage('Archive creation cancelled - password required');
            return;
        }
        
        const outputPath = await vscode.window.showSaveDialog({
            title: 'Save archive as',
            filters: {
                'FileVault Archive': ['fva']
            }
        });
        
        if (!outputPath) {
            return;
        }
        
        const args = ['archive', 'create'];
        files.forEach(f => args.push(f.fsPath));
        args.push('-o', outputPath.fsPath);
        args.push('-p', password);  // Always pass password
        
        await vscode.window.withProgress({
            location: vscode.ProgressLocation.Notification,
            title: 'Creating archive...',
            cancellable: false
        }, async () => {
            await runFileVault(args);
        });
        
        vscode.window.showInformationMessage(
            `Archive created: ${path.basename(outputPath.fsPath)}`,
            'Open Folder'
        ).then((result: any) => {
            if (result === 'Open Folder') {
                vscode.commands.executeCommand('revealFileInOS', outputPath);
            }
        });
        
    } catch (error: any) {
        vscode.window.showErrorMessage(`Archive creation failed: ${error.message}`);
    }
}

// Steganography
async function steganography(uri?: vscode.Uri) {
    try {
        const action = await vscode.window.showQuickPick([
            { label: 'Hide', description: 'Hide data in image', value: 'hide' },
            { label: 'Extract', description: 'Extract hidden data', value: 'extract' }
        ], {
            placeHolder: 'Select action',
            title: 'Steganography'
        });
        
        if (!action) {
            return;
        }
        
        if (action.value === 'hide') {
            // Hide data in image
            const coverImage = await vscode.window.showOpenDialog({
                canSelectMany: false,
                title: 'Select cover image',
                filters: {
                    'Images': ['png', 'bmp', 'jpg', 'jpeg', 'gif', 'webp'],
                    'All Files': ['*']
                }
            });
            
            if (!coverImage || coverImage.length === 0) {
                return;
            }
            
            const dataFile = await vscode.window.showOpenDialog({
                canSelectMany: false,
                title: 'Select file to hide'
            });
            
            if (!dataFile || dataFile.length === 0) {
                return;
            }
            
            const outputPath = coverImage[0].fsPath.replace(/\.(png|bmp)$/i, '_stego.$1');
            
            await vscode.window.withProgress({
                location: vscode.ProgressLocation.Notification,
                title: 'Hiding data...',
                cancellable: false
            }, async () => {
                // Note: Currently CLI doesn't support filename metadata
                // User will need to manually rename extracted file
                await runFileVault([
                    'stego',
                    'embed',
                    dataFile[0].fsPath,
                    coverImage[0].fsPath,
                    outputPath
                ]);
            });
            
            vscode.window.showInformationMessage(
                `Data hidden in: ${path.basename(outputPath)}`,
                'Open Folder'
            ).then((result: any) => {
                if (result === 'Open Folder') {
                    vscode.commands.executeCommand('revealFileInOS', vscode.Uri.file(outputPath));
                }
            });
            
        } else {
            // Extract hidden data
            const stegoImage = await vscode.window.showOpenDialog({
                canSelectMany: false,
                title: 'Select stego image',
                filters: {
                    'Images': ['png', 'bmp', 'jpg', 'jpeg', 'gif', 'webp'],
                    'All Files': ['*']
                }
            });
            
            if (!stegoImage || stegoImage.length === 0) {
                return;
            }
            
            const outputPath = stegoImage[0].fsPath + '_extracted.dat';
            
            await vscode.window.withProgress({
                location: vscode.ProgressLocation.Notification,
                title: 'Extracting data...',
                cancellable: false
            }, async () => {
                await runFileVault([
                    'stego',
                    'extract',
                    stegoImage[0].fsPath,
                    outputPath
                ]);
            });
            
            vscode.window.showInformationMessage(
                `Data extracted to: ${path.basename(outputPath)}`,
                'Open File',
                'Open Folder'
            ).then((result: any) => {
                if (result === 'Open File') {
                    vscode.window.showTextDocument(vscode.Uri.file(outputPath));
                } else if (result === 'Open Folder') {
                    vscode.commands.executeCommand('revealFileInOS', vscode.Uri.file(outputPath));
                }
            });
        }
        
    } catch (error: any) {
        vscode.window.showErrorMessage(`Steganography operation failed: ${error.message}`);
    }
}

// Decompress file
async function decompressFile(uri?: vscode.Uri) {
    try {
        let filePath: string;
        if (uri) {
            filePath = uri.fsPath;
        } else {
            const fileUri = await vscode.window.showOpenDialog({
                canSelectMany: false,
                title: 'Select file to decompress',
                filters: {
                    'Compressed Files': ['zlib', 'bz2', 'xz', 'lzma', 'zz', 'compressed'],
                    'All Files': ['*']
                }
            });
            if (!fileUri || fileUri.length === 0) {
                return;
            }
            filePath = fileUri[0].fsPath;
        }
        
        // Ask for output path
        const defaultOutput = filePath.replace(/\.(zlib|bz2|xz|lzma|zz|compressed)$/i, '') || filePath + '.decompressed';
        const outputPath = await vscode.window.showSaveDialog({
            title: 'Save decompressed file as',
            defaultUri: vscode.Uri.file(defaultOutput)
        });
        
        if (!outputPath) {
            return;
        }
        
        await vscode.window.withProgress({
            location: vscode.ProgressLocation.Notification,
            title: 'Decompressing...',
            cancellable: false
        }, async () => {
            // Use auto-detect for algorithm
            await runFileVault(['decompress', filePath, '-o', outputPath.fsPath]);
        });
        
        vscode.window.showInformationMessage(
            `File decompressed: ${path.basename(outputPath.fsPath)}`,
            'Open File',
            'Open Folder'
        ).then((result: any) => {
            if (result === 'Open File') {
                vscode.window.showTextDocument(outputPath);
            } else if (result === 'Open Folder') {
                vscode.commands.executeCommand('revealFileInOS', outputPath);
            }
        });
        
    } catch (error: any) {
        vscode.window.showErrorMessage(`Decompression failed: ${error.message}`);
    }
}

// Extract archive
async function extractArchive(uri?: vscode.Uri) {
    try {
        let archivePath: string;
        if (uri) {
            archivePath = uri.fsPath;
        } else {
            const fileUri = await vscode.window.showOpenDialog({
                canSelectMany: false,
                title: 'Select archive to extract',
                filters: {
                    'FileVault Archive': ['fva']
                }
            });
            if (!fileUri || fileUri.length === 0) {
                return;
            }
            archivePath = fileUri[0].fsPath;
        }
        
        const outputDir = await vscode.window.showOpenDialog({
            canSelectFiles: false,
            canSelectFolders: true,
            canSelectMany: false,
            title: 'Select output directory'
        });
        
        if (!outputDir || outputDir.length === 0) {
            return;
        }
        
        const password = await vscode.window.showInputBox({
            prompt: 'Enter archive password',
            password: true,
            placeHolder: 'Password'
        });
        
        if (!password) {
            return;
        }
        
        await vscode.window.withProgress({
            location: vscode.ProgressLocation.Notification,
            title: 'Extracting archive...',
            cancellable: false
        }, async () => {
            await runFileVault([
                'archive', 'extract',
                archivePath,
                '-o', outputDir[0].fsPath,
                '-p', password
            ]);
        });
        
        vscode.window.showInformationMessage(
            'Archive extracted successfully',
            'Open Folder'
        ).then((result: any) => {
            if (result === 'Open Folder') {
                vscode.commands.executeCommand('revealFileInOS', outputDir[0]);
            }
        });
        
    } catch (error: any) {
        vscode.window.showErrorMessage(`Archive extraction failed: ${error.message}`);
    }
}

// Steganography - Embed
async function stegoEmbed() {
    try {
        const dataFile = await vscode.window.showOpenDialog({
            canSelectMany: false,
            title: 'Select file to hide'
        });
        
        if (!dataFile || dataFile.length === 0) {
            return;
        }
        
        const coverImage = await vscode.window.showOpenDialog({
            canSelectMany: false,
            title: 'Select cover image',
            filters: {
                'Images': ['png', 'bmp', 'jpg', 'jpeg', 'gif', 'webp'],
                'All Files': ['*']
            }
        });
        
        if (!coverImage || coverImage.length === 0) {
            return;
        }
        
        const outputPath = await vscode.window.showSaveDialog({
            title: 'Save stego image as',
            filters: {
                'Images': ['png', 'bmp', 'jpg', 'jpeg', 'gif', 'webp'],
                'All Files': ['*']
            },
            defaultUri: vscode.Uri.file(coverImage[0].fsPath.replace(/\.(png|bmp|jpg|jpeg|gif|webp)$/i, '_stego.$1'))
        });
        
        if (!outputPath) {
            return;
        }
        
        await vscode.window.withProgress({
            location: vscode.ProgressLocation.Notification,
            title: 'Embedding data...',
            cancellable: false
        }, async () => {
            await runFileVault([
                'stego', 'embed',
                dataFile[0].fsPath,
                coverImage[0].fsPath,
                outputPath.fsPath
            ]);
        });
        
        // Verify output file was created
        if (!fs.existsSync(outputPath.fsPath)) {
            throw new Error('Stego embed failed: output file not created');
        }
        
        vscode.window.showInformationMessage(
            `Data hidden in: ${path.basename(outputPath.fsPath)}`,
            'Open Folder'
        ).then((result: any) => {
            if (result === 'Open Folder') {
                vscode.commands.executeCommand('revealFileInOS', outputPath);
            }
        });
        
    } catch (error: any) {
        vscode.window.showErrorMessage(`Stego embed failed: ${error.message}`);
    }
}

// Steganography - Extract
async function stegoExtract(uri?: vscode.Uri) {
    try {
        let stegoImage: string;
        if (uri) {
            stegoImage = uri.fsPath;
        } else {
            const fileUri = await vscode.window.showOpenDialog({
                canSelectMany: false,
                title: 'Select stego image',
                filters: {
                    'Images': ['png', 'bmp', 'jpg', 'jpeg', 'gif', 'webp'],
                    'All Files': ['*']
                }
            });
            if (!fileUri || fileUri.length === 0) {
                return;
            }
            stegoImage = fileUri[0].fsPath;
        }
        
        // Extract to temp location first to get the actual filename from metadata
        const tempDir = path.join(require('os').tmpdir(), 'filevault-extract');
        if (!fs.existsSync(tempDir)) {
            fs.mkdirSync(tempDir, { recursive: true });
        }
        
        const tempOutput = path.join(tempDir, 'temp_extract');
        
        let extractedFilePath: string | undefined;
        
        await vscode.window.withProgress({
            location: vscode.ProgressLocation.Notification,
            title: 'Extracting hidden data...',
            cancellable: false
        }, async () => {
            // Extract to temp - CLI will rename based on metadata
            await runFileVault([
                'stego', 'extract',
                stegoImage,
                tempOutput
            ]);
            
            // Find the actual extracted file (CLI renames it with original filename)
            const tempFiles = fs.readdirSync(tempDir);
            if (tempFiles.length === 0) {
                throw new Error('No file was extracted');
            }
            
            // CLI should have created a file with the original name
            extractedFilePath = path.join(tempDir, tempFiles[0]);
        });
        
        if (!extractedFilePath) {
            throw new Error('Failed to extract file');
        }
        
        // Now show save dialog with the correct original filename
        const originalFilename = path.basename(extractedFilePath);
        const imageDir = path.dirname(stegoImage);
        
        const outputPath = await vscode.window.showSaveDialog({
            title: 'Save extracted file as',
            defaultUri: vscode.Uri.file(path.join(imageDir, originalFilename)),
            filters: {
                'All Files': ['*']
            }
        });
        
        if (!outputPath) {
            // Clean up temp file
            fs.unlinkSync(extractedFilePath);
            return;
        }
        
        // Move from temp to final location
        fs.copyFileSync(extractedFilePath, outputPath.fsPath);
        fs.unlinkSync(extractedFilePath);
        
        vscode.window.showInformationMessage(
            `Data extracted: ${path.basename(outputPath.fsPath)}`,
            'Open File',
            'Open Folder'
        ).then((result: any) => {
            if (result === 'Open File') {
                vscode.window.showTextDocument(outputPath);
            } else if (result === 'Open Folder') {
                vscode.commands.executeCommand('revealFileInOS', outputPath);
            }
        });
        
    } catch (error: any) {
        vscode.window.showErrorMessage(`Stego extract failed: ${error.message}`);
    }
}

// Steganography - Check capacity
async function stegoCapacity(uri?: vscode.Uri) {
    try {
        let imagePath: string;
        if (uri) {
            imagePath = uri.fsPath;
        } else {
            const fileUri = await vscode.window.showOpenDialog({
                canSelectMany: false,
                title: 'Select image to check capacity',
                filters: {
                    'Images': ['png', 'bmp', 'jpg', 'jpeg', 'gif', 'webp'],
                    'All Files': ['*']
                }
            });
            if (!fileUri || fileUri.length === 0) {
                return;
            }
            imagePath = fileUri[0].fsPath;
        }
        
        outputChannel.show();
        await runFileVault(['stego', 'capacity', imagePath]);
        
    } catch (error: any) {
        vscode.window.showErrorMessage(`Capacity check failed: ${error.message}`);
    }
}

// Set executable path
async function setExecutablePath() {
    const fileUri = await vscode.window.showOpenDialog({
        canSelectFiles: true,
        canSelectFolders: false,
        canSelectMany: false,
        title: 'Select FileVault executable',
        filters: process.platform === 'win32' 
            ? { 'Executable': ['exe'] }
            : undefined
    });
    
    if (!fileUri || fileUri.length === 0) {
        return;
    }
    
    const config = vscode.workspace.getConfiguration('filevault');
    await config.update('executablePath', fileUri[0].fsPath, vscode.ConfigurationTarget.Global);
    
    vscode.window.showInformationMessage(`FileVault path set to: ${fileUri[0].fsPath}`);
}
