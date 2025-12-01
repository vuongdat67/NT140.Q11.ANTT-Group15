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
        vscode.commands.registerCommand('filevault.setExecutablePath', setExecutablePath)
    );
    
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

// Run FileVault command
function runFileVault(args: string[]): Promise<{ stdout: string; stderr: string }> {
    return new Promise((resolve, reject) => {
        const executable = getExecutablePath();
        outputChannel.appendLine(`> ${executable} ${args.join(' ')}`);
        
        const process = child_process.spawn(executable, args, {
            shell: true
        });
        
        let stdout = '';
        let stderr = '';
        
        process.stdout.on('data', (data) => {
            stdout += data.toString();
            outputChannel.append(data.toString());
        });
        
        process.stderr.on('data', (data) => {
            stderr += data.toString();
            outputChannel.append(data.toString());
        });
        
        process.on('close', (code) => {
            if (code === 0) {
                resolve({ stdout, stderr });
            } else {
                reject(new Error(stderr || `Process exited with code ${code}`));
            }
        });
        
        process.on('error', (error) => {
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
        
        // Get algorithm
        const config = vscode.workspace.getConfiguration('filevault');
        const algorithms = [
            { label: 'AES-256-GCM', description: 'Recommended - NIST Standard', value: 'aes-256-gcm' },
            { label: 'ChaCha20-Poly1305', description: 'Fast software implementation', value: 'chacha20-poly1305' },
            { label: 'Kyber-1024-Hybrid', description: 'Post-Quantum Resistant', value: 'kyber-1024-hybrid' },
            { label: 'Serpent-256-GCM', description: 'Maximum security', value: 'serpent-256-gcm' },
            { label: 'Camellia-256-GCM', description: 'Japan Standard', value: 'camellia-256-gcm' },
            { label: 'ARIA-256-GCM', description: 'Korea Standard', value: 'aria-256-gcm' },
            { label: 'SM4-GCM', description: 'China Standard', value: 'sm4-gcm' }
        ];
        
        const algorithm = await vscode.window.showQuickPick(algorithms, {
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
        
        const security = await vscode.window.showQuickPick(securityLevels, {
            placeHolder: 'Select security level',
            title: 'Security Level'
        });
        
        if (!security) {
            return;
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
            await runFileVault([
                'encrypt',
                filePath,
                outputPath,
                '-a', algorithm.value,
                '-s', security.value,
                '-p', password
            ]);
        });
        
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
                title: 'Select encrypted file',
                filters: {
                    'FileVault Encrypted': ['fvlt']
                }
            });
            if (!fileUri || fileUri.length === 0) {
                return;
            }
            filePath = fileUri[0].fsPath;
        }
        
        const result = await runFileVault(['info', filePath]);
        
        // Show in output channel
        outputChannel.show();
        
        // Also show in a quick pick or webview
        vscode.window.showInformationMessage(
            `File: ${path.basename(filePath)} - See Output panel for details`,
            'Show Output'
        ).then(result => {
            if (result === 'Show Output') {
                outputChannel.show();
            }
        });
        
    } catch (error: any) {
        vscode.window.showErrorMessage(`Failed to get file info: ${error.message}`);
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
        
        const result = await runFileVault(['hash', filePath, '-a', algorithm.value]);
        
        // Extract hash from output
        const hashMatch = result.stdout.match(/^([A-Fa-f0-9]+)/m);
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
