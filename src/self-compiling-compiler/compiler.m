; Elf32_Ehdr
  7f 45 4c 46 01 01 01  ; e_ident
  03 00 00 00 00 00 00 00 00
  02 00                 ; e_type
  03 00                 ; e_machine
  01 00 00 00           ; e_version
  54 80 04 08           ; e_entry = 0x08048000 + len(ehdr) + len(phdr)
  34 00 00 00           ; e_phoff = len(ehdr)
  00 00 00 00           ; e_shoff
  00 00 00 00           ; e_flags
  34 00                 ; e_ehsize = len(ehdr)
  20 00                 ; e_phentsize = len(phdr)
  01 00                 ; e_phnum
  00 00                 ; e_shentsize
  00 00                 ; e_shnum
  00 00                 ; e_shstrndx
; Elf32_Phdr
  01 00 00 00           ; p_type
  00 00 00 00           ; p_offset
  00 80 04 08           ; p_vaddr = 0x08048000
  00 80 04 08           ; p_paddr = 0x08048000
  bf 00 00 00   ;!      ; p_filesz = len(ehdr) + len(phdr) + len(prog)
  bf 00 00 00   ;!      ; p_memsz = len(ehdr) + len(phdr) + len(prog)
  05 00 00 00           ; p_flags
  00 10 00 00           ; p_align
; Instructions
; mov                assign data to memory or register
; jmp                goto a memory address
; cmp                compare two values and set result flags
; je jne jle         goto a memory address or skip to next instruction depending on flags
; add sub shl        add, subtract and shift left n bits
; push pop           stack operations
; int                system interrupt used for system function calls
; System Calls
; read               used to read from stdin
; write              used to write to stdout
; exit               used to tell the OS the process is finished
; Registers
; eax                ascii character just read
; ebx                byte being read (-1 => no bits read)
; ecx                comment state (0 => text, 1 => comments)
; init
    bb ff ff ff ff           ; mov    ebx,0xffffffff
; text_mode
    b9 00 00 00 00           ; mov    ecx,0x0
; next
    51                       ; push   ecx
    53                       ; push   ebx
    68 00 00 00 00           ; push   0x0
    b8 03 00 00 00           ; mov    eax,0x3
    bb 00 00 00 00           ; mov    ebx,0x0
    89 e1                    ; mov    ecx,esp
    ba 01 00 00 00           ; mov    edx,0x1
    cd 80                    ; int    0x80
    3d 00 00 00 00           ; cmp    eax,0x0
    7e 70                    ; jle    9b <end>
    58                       ; pop    eax
    5b                       ; pop    ebx
    59                       ; pop    ecx
    3d 0a 00 00 00           ; cmp    eax,0xa
    74 d0                    ; je     5 <text_mode>
    81 f9 01 00 00 00        ; cmp    ecx,0x1
    74 cd                    ; je     a <next>
    3d 3b 00 00 00           ; cmp    eax,0x3b
    74 4d                    ; je     91 <comment_mode>
    3d 20 00 00 00           ; cmp    eax,0x20
    74 bf                    ; je     a <next>
    3d 09 00 00 00           ; cmp    eax,0x9
    74 b8                    ; je     a <next>
; convert text to nibble
    2d 30 00 00 00           ; sub    eax,0x30
    3d 09 00 00 00           ; cmp    eax,0x9
    7e 05                    ; jle    63 <accumulate_result>
    2d 27 00 00 00           ; sub    eax,0x27
; accumulate_result
    81 fb ff ff ff ff        ; cmp    ebx,0xffffffff
    75 0a                    ; jne    75 <print_byte>
    c1 e0 04                 ; shl    eax,0x4
    89 c3                    ; mov    ebx,eax
    e9 95 ff ff ff           ; jmp    a <next>
; print_byte
    01 c3                    ; add    ebx,eax
    53                       ; push   ebx
    b8 04 00 00 00           ; mov    eax,0x4
    bb 01 00 00 00           ; mov    ebx,0x1
    89 e1                    ; mov    ecx,esp
    ba 01 00 00 00           ; mov    edx,0x1
    cd 80                    ; int    0x80
    5b                       ; pop    ebx
    e9 6f ff ff ff           ; jmp    0 <init>
; comment_mode
    b9 01 00 00 00           ; mov    ecx,0x1
    e9 6f ff ff ff           ; jmp    a <next>
; exit
    b8 01 00 00 00           ; mov    eax,0x1
    bb 00 00 00 00           ; mov    ebx,0x0
    cd 80                    ; int    0x80
