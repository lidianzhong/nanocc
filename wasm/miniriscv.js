// === Minimal RISC-V 32 Interpreter for SysY (Subset) ===
// Supports basic integer instructions, stack operations, and simple control flow needed for SysY
class MiniRiscV {
    constructor() {
        this.REG_NAMES = [
            "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
            "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
            "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
            "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
        ];
        // Map register alias to index
        this.REG_MAP = {};
        this.REG_NAMES.forEach((name, idx) => {
            this.REG_MAP[name] = idx;
            this.REG_MAP[`x${idx}`] = idx;
            if (name === 's0') this.REG_MAP['fp'] = 8;
        });

        this.reset();
    }

    reset() {
        this.regs = new Int32Array(32);
        this.memory = new Uint8Array(256 * 1024 * 1024); // 256MB Ram
        this.regs[2] = this.memory.length - 0x100; // Initial SP (at end of memory)
        this.pc = 0;
        this.labels = {}; // label -> address
        this.instructions = []; // parsed instructions
        
        // Track memory allocations for .data segment
        this.heapPointer = 0x10000; // Start data segment at 64KB
    }

    parse(asm) {
        this.reset();
        const lines = asm.split('\n');
        let currentSection = '.text';
        
        // First pass: locate labels and data
        let iAddr = 0; // instruction index
        
        for (let line of lines) {
            line = line.trim();
            if (!line || line.startsWith('#') || line.startsWith(';')) continue;

            // Remove comments
            line = line.split('#')[0].trim();
            if (!line) continue;

            // Handle labels (foo: or foo: .word 10)
            if (line.indexOf(':') !== -1) {
                const idx = line.indexOf(':');
                const label = line.slice(0, idx).trim();
                
                if (currentSection === '.text') {
                    this.labels[label] = iAddr;
                } else if (currentSection === '.data') {
                    if (this.heapPointer % 4 !== 0) this.heapPointer += (4 - (this.heapPointer % 4));
                    this.labels[label] = this.heapPointer;
                }
                line = line.slice(idx + 1).trim();
                if (!line) continue;
            }

            if (line.startsWith('.')) {
                const parts = line.split(/\s+/);
                const dir = parts[0];
                if (dir === '.text' || dir === '.data') {
                    currentSection = dir;
                } else if (dir === '.globl' || dir === '.align' || dir === '.file' || dir === '.attribute' || dir === '.type' || dir === '.size') {
                    // ignore directives often produced by clang/gcc/nanocc
                } else if (dir === '.word' && currentSection === '.data') {
                    const values = line.replace('.word', '').replace(/,/g, ' ').trim().split(/\s+/);
                    for (let v of values) {
                        if (!v) continue;
                        const val = parseInt(v);
                        if (this.heapPointer % 4 !== 0) {
                            this.heapPointer += (4 - (this.heapPointer % 4));
                        }
                        this.writeInt32(this.heapPointer, val);
                        this.heapPointer += 4;
                    }
                } else if (dir === '.zero' && currentSection === '.data') {
                   const size = parseInt(parts[1]);
                   this.heapPointer += size;
                }
                continue;
            }

            // Instruction
            if (currentSection === '.text') {
                this.instructions.push({ line, pc: iAddr });
                iAddr++;
            }
        }
    }

    run(outputCallback) {
        const MAX_STEPS = 5000000;
        let steps = 0;
        
        // Find entry point
        let startPC = this.labels['main'];
        if (startPC === undefined) {
            outputCallback("Error: 'main' label not found.");
            return;
        }

        // Initialize RA to designated exit address (e.g. -1)
        this.regs[1] = -1;
        this.pc = startPC;

        try {
            while (this.pc >= 0 && this.pc < this.instructions.length && steps < MAX_STEPS) {
                const instObj = this.instructions[this.pc];
                const originalLine = instObj.line;
                
                // Clean commas and split
                const parts = originalLine.replace(/,/g, ' ').split(/\s+/);
                const op = parts[0];

                // PC increment
                let nextPC = this.pc + 1;

                switch (op) {
                    case 'addi': 
                        this.setReg(parts[1], this.getReg(parts[2]) + parseInt(parts[3]));
                        break;
                    case 'add':
                        this.setReg(parts[1], this.getReg(parts[2]) + this.getReg(parts[3]));
                        break;
                    case 'sub':
                        this.setReg(parts[1], this.getReg(parts[2]) - this.getReg(parts[3]));
                        break;
                    case 'mul':
                        this.setReg(parts[1], Math.imul(this.getReg(parts[2]), this.getReg(parts[3])));
                        break;
                    case 'div': 
                        const divisor = this.getReg(parts[3]);
                        if (divisor === 0) this.setReg(parts[1], -1);
                        else this.setReg(parts[1], (this.getReg(parts[2]) / divisor) | 0);
                        break;
                    case 'rem':
                        const remDivisor = this.getReg(parts[3]);
                        if (remDivisor === 0) this.setReg(parts[1], this.getReg(parts[2]));
                        else this.setReg(parts[1], this.getReg(parts[2]) % remDivisor);
                        break;
                    case 'slt': 
                        this.setReg(parts[1], (this.getReg(parts[2]) < this.getReg(parts[3])) ? 1 : 0);
                        break;
                    case 'sgt': 
                        this.setReg(parts[1], (this.getReg(parts[2]) > this.getReg(parts[3])) ? 1 : 0);
                        break;
                    case 'snez': 
                        this.setReg(parts[1], (this.getReg(parts[2]) !== 0) ? 1 : 0);
                        break;
                    case 'seqz': 
                        this.setReg(parts[1], (this.getReg(parts[2]) === 0) ? 1 : 0);
                        break;
                    case 'and':
                        this.setReg(parts[1], this.getReg(parts[2]) & this.getReg(parts[3]));
                        break;
                    case 'or':
                        this.setReg(parts[1], this.getReg(parts[2]) | this.getReg(parts[3]));
                        break; 
                    case 'xor':
                        this.setReg(parts[1], this.getReg(parts[2]) ^ this.getReg(parts[3]));
                        break;   
                    case 'slli':
                        this.setReg(parts[1], this.getReg(parts[2]) << parseInt(parts[3]));
                        break;  
                        
                    // Memory
                    case 'lw': {
                        // Match offset(base) or (base)
                        const match = parts[2].match(/^(-?\d+)?\((.+)\)$/);
                        if (match) {
                            const offset = match[1] ? parseInt(match[1]) : 0;
                            const base = this.getReg(match[2]);
                            this.setReg(parts[1], this.readInt32(base + offset));
                        }
                        break;
                    }
                    case 'sw': {
                        const match = parts[2].match(/^(-?\d+)?\((.+)\)$/);
                        if (match) {
                            const offset = match[1] ? parseInt(match[1]) : 0;
                            const base = this.getReg(match[2]);
                            const val = this.getReg(parts[1]);
                            this.writeInt32(base + offset, val);
                        }
                        break;
                    }

                    // Control Flow
                    case 'j': {
                        const target = this.labels[parts[1]];
                        if (target !== undefined) nextPC = target;
                        else throw new Error(`Label ${parts[1]} not found`);
                        break;
                    }
                    case 'bnez': {
                        if (this.getReg(parts[1]) !== 0) {
                            const target = this.labels[parts[2]];
                            if (target !== undefined) nextPC = target;
                        }
                        break;
                    }
                    case 'beqz': {
                        if (this.getReg(parts[1]) === 0) {
                            const target = this.labels[parts[2]];
                            if (target !== undefined) nextPC = target;
                        }
                        break;
                    }
                    case 'beq': {
                        if (this.getReg(parts[1]) === this.getReg(parts[2])) {
                            const target = this.labels[parts[3]];
                            if (target !== undefined) nextPC = target;
                        }
                        break;
                    }
                    case 'bne': {
                        if (this.getReg(parts[1]) !== this.getReg(parts[2])) {
                            const target = this.labels[parts[3]];
                            if (target !== undefined) nextPC = target;
                        }
                        break;
                    }
                    case 'blt': {
                        if (this.getReg(parts[1]) < this.getReg(parts[2])) {
                            const target = this.labels[parts[3]];
                            if (target !== undefined) nextPC = target;
                        }
                        break;
                    }
                    case 'bge': {
                        if (this.getReg(parts[1]) >= this.getReg(parts[2])) {
                            const target = this.labels[parts[3]];
                            if (target !== undefined) nextPC = target;
                        }
                        break;
                    }
                    case 'bgt': {
                        if (this.getReg(parts[1]) > this.getReg(parts[2])) {
                            const target = this.labels[parts[3]];
                            if (target !== undefined) nextPC = target;
                        }
                        break;
                    }
                    case 'ble': {
                        if (this.getReg(parts[1]) <= this.getReg(parts[2])) {
                            const target = this.labels[parts[3]];
                            if (target !== undefined) nextPC = target;
                        }
                        break;
                    }
                    case 'call': {
                        const funcName = parts[1];
                        if (funcName === 'putint') {
                            outputCallback("" + this.getReg('a0'));
                        } else if (funcName === 'putch') {
                            // Ensure it is a valid char code
                            const ch = this.getReg('a0');
                            outputCallback(String.fromCharCode(ch));
                        } else if (funcName === 'putarray') {
                            const n = this.getReg('a0');
                            const ptr = this.getReg('a1');
                            const res = [];
                            for(let i=0; i<n; i++) {
                                res.push(this.readInt32(ptr + i*4));
                            }
                            outputCallback(res.join(" "));
                        } else if (funcName === 'getint') {
                            const val = prompt("Input integer for getint:"); 
                            this.setReg('a0', parseInt(val || "0"));
                        } else if (funcName === 'getch') {
                            const val = prompt("Input char for getch:"); 
                            this.setReg('a0', val ? val.charCodeAt(0) : 0);
                        } else if (funcName === 'getarray') {
                            const ptr = this.getReg('a0');
                            const input = prompt("Input integers for getarray (space separated):");
                            if (!input) {
                                this.setReg('a0', 0);
                            } else {
                                const numStrs = input.trim().split(/\s+/);
                                let count = 0;
                                for (let s of numStrs) {
                                    if(s) {
                                        this.writeInt32(ptr + count * 4, parseInt(s));
                                        count++;
                                    }
                                }
                                this.setReg('a0', count);
                            }
                        } else if (funcName === 'starttime') {
                            const lineno = this.getReg('a0');
                            outputCallback(`\n[Timer Start] Line: ${lineno} At: ${performance.now().toFixed(2)}ms\n`); 
                        } else if (funcName === 'stoptime') {
                             const lineno = this.getReg('a0');
                            outputCallback(`\n[Timer Stop] Line: ${lineno} At: ${performance.now().toFixed(2)}ms\n`);
                        } else {
                            if (this.labels[funcName] !== undefined) {
                                        this.setReg('ra', this.pc + 1);
                                nextPC = this.labels[funcName];
                            } else {
                                throw new Error(`Function ${funcName} not found`);
                            }
                        }
                        break;
                    }
                    case 'ret': {
                        nextPC = this.getReg('ra');
                        if (nextPC === -1) {
                            return this.getReg('a0');
                        }
                        break;
                    }
                    case 'ecall': {
                        const syscall = this.getReg('a7');
                        if (syscall === 1) { // print_int
                            outputCallback("" + this.getReg('a0'));
                        } else if (syscall === 11) { // print_char
                            outputCallback(String.fromCharCode(this.getReg('a0')));
                        } else if (syscall === 10 || syscall === 93) { // exit
                             return this.getReg('a0');
                        } else {
                            outputCallback(`\nWarning: Unknown syscall ${syscall}`);
                        }
                        break;
                    }

                    // Pseudo
                    case 'li':
                        this.setReg(parts[1], parseInt(parts[2]));
                        break;
                    case 'mv':
                        this.setReg(parts[1], this.getReg(parts[2]));
                        break;
                    case 'la': {
                        const addr = this.labels[parts[2]];
                        if (addr !== undefined) this.setReg(parts[1], addr);
                        else throw new Error(`Symbol ${parts[2]} not found`);
                        break;
                    }
                    
                    default:
                        // Ignored or unknown
                }

                this.pc = nextPC;
                steps++;
            }
            if (steps >= MAX_STEPS) {
                outputCallback("\n[Error] Execution timeout (infinite loop?)");
            }

        } catch (e) {
            outputCallback(`\n[Runtime Error] ${e.message} at line: ${this.instructions[this.pc] ? this.instructions[this.pc].line : '?'}`);
            console.error(e);
        }
    }

    // Helpers
    // (Existing helpers omitted to be safe, but I should include them to keep the file valid if rewriting)
    getReg(name) {
        if (typeof name === 'string') {
            const idx = this.REG_MAP[name];
            if (idx !== undefined) return this.regs[idx];
        }
        return 0;
    }
    setReg(name, val) {
        const idx = this.REG_MAP[name];
        if (idx !== undefined && idx !== 0) { 
            this.regs[idx] = val | 0; 
        }
    }
    readInt32(addr) {
        // Allow reading from 0-4KB (often used for null checks or low mem) or up to memory size
        if (addr < 0 || addr > this.memory.length - 4) throw new Error("Segfault Read " + addr);
        return this.memory[addr] | (this.memory[addr+1] << 8) | (this.memory[addr+2] << 16) | (this.memory[addr+3] << 24);
    }
    writeInt32(addr, val) {
            if (addr < 0 || addr > this.memory.length - 4) throw new Error("Segfault Write " + addr);
            this.memory[addr] = val & 0xFF;
            this.memory[addr+1] = (val >> 8) & 0xFF;
            this.memory[addr+2] = (val >> 16) & 0xFF;
            this.memory[addr+3] = (val >> 24) & 0xFF;
    }
}

if (typeof module !== 'undefined' && module.exports) {
    module.exports = MiniRiscV;
}
