; 编译指令 nasm -felf32 -g Convert.asm -o Convert.o   ld -g -melf_i386 Convert.o -o Convert
section .data
    InputPrompt db "Input:", 0 
    OutputPrompt db "Output:", 0

    dataBufferLen db 0; 统计输入的数据
    dataWithBlankLen db 0;统计输入的数据连同blank的数量 
    inputBufferLen db 0;

    ErrorMessage db 'Error occurred!', 0
    NewLine db 0x0A
    BinaryPrefix db "0b"
    OctalPrefix db "0o"
    HexPrefix db "0x"

section .bss
    inputBuffer resb 36 
    computationBuffer resb 36 ; 中间计算的区域
    resultBuffer resb 120
    
section .text
    global _start

_start:
    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx

getInput:
    call ResetBuffer
    mov esi, inputBuffer
    ; 一次性从缓冲区中读取所有的数据，esi是当前要读取的内存的指针
    mov eax, 3; 系统调用3,读取输入
    mov ebx, 0; 在标准输入中读取输入
    mov ecx, esi; 将inputBuffer作为缓冲区
    mov edx, 36; 一次读取一个字节的长度
    int 0x80
getInputLoop:
    ; 如果读进去的是q，就直接退出程序
    cmp byte[inputBuffer], "q"
    je quit
    
    ; 增加一个读进去的数字
    add byte[inputBufferLen],1

    cmp byte[esi], " "; 检查是否输入数字结束,这里出问题了
    je blankSpace

    ; 用一个寄存器r8判断输入是否是一个数字
    mov al,48
    cmp byte[esi],al
    jl OutputError

    mov al,57
    cmp byte[esi], al
    jg OutputError

    add byte[dataBufferLen],1 ;将两个值+1,恰好能抵消掉初始值
    add byte[dataWithBlankLen], 1
    inc esi; 指向下一个内存位置
    jmp getInputLoop


blankSpace: 
    add byte[dataWithBlankLen],1 ;上一步跳转过来会获得一个空白格
    inc esi ;让指针指向下一个位置

    sub byte[dataWithBlankLen],1;先抵消掉
omittingAllBlankSpace:
    ; 清空所有的空白格
    add byte[dataWithBlankLen], 1; 增加计数
    add byte[inputBufferLen],1 ; 又读入了一个数据
    inc esi;
    cmp byte[esi]," "
    je omittingAllBlankSpace

; 再读一次，最后的，看看是否是换行符号
CheckRadixLegal:
    cmp byte[esi],10
    jne OutputError
    dec esi ; 复原esi

JudgeRadix:
    call inputBufToComputationBuf ; 将数据传输到ComputationBuf先
    ; 这里就是指向参数类型了,然后判断接受的参数是否合理
    cmp byte[esi], "b"
    je ToBinary
    cmp byte[esi],"o"
    je ToOctal
    cmp byte[esi], "h"
    je ToHex

quit:
    ; 退出程序
    mov eax, 1 ; 系统调用号：exit
    xor ebx, ebx ; 退出状态码：0
    int 0x80 ; 调用系统调用

; 将数值全部移动到ComputationBuf里面,顺便把所有的数字转化为十进制
inputBufToComputationBuf:
    ;先清空寄存器
    xor al,al;

    ;保护寄存器
    mov ebx,esi
    mov ecx,edi

    mov esi,inputBuffer
    mov edi,computationBuffer

    Loop:
        mov al,byte[esi]
        mov byte[edi],al
        sub byte[edi],"0"
        inc esi
        inc edi
        cmp byte[esi]," "
        jne Loop

    ;恢复寄存器
    mov esi,ebx
    mov edi,ecx
    ret
    
    
ToBinary:
    call Convert
    call BitToChar
    call OutputBinary
ToOctal:
    call Convert
    call BitToChar
    call OutputOctal
ToHex:
    call Convert
    call BitToChar
    call OutputHex

; 将resultBuf全部转化为字符
BitToChar:
    mov esi,resultBuffer
    mov ecx, 0
    BitToChar_Loop:
        mov al,byte[esi]
        add al,"0"
        mov byte[esi],al
        inc ecx
        inc esi
        cmp ecx,120
        jne BitToChar_Loop
    ret
;
is_all_zeros:
    mov esi,computationBuffer
    movzx ecx,byte[dataBufferLen]
    xor al, al ; 将 al 寄存器清零，用于保存结果

    ; 循环遍历字符串，loop会检查ecx寄存器的值
    loop:
        mov bl, byte[esi] ; 读取当前地址的字符到 bl 寄存器
        test bl, bl ; 检查 dl 寄存器的值是否为0
        jnz non_zero ; 如果非0，跳转到 non_zero 标签处

        inc esi
        cmp ecx,0
        loop loop

    jmp all_zero

    non_zero:
        mov al, 0 ; 将 al 寄存器设置为非0，表示存在非0字符
        mov esi,computationBuffer
        movzx ecx,byte[dataBufferLen]
        add esi,ecx
        sub esi,1;重置为最后一格
        ret
    all_zero:
        mov al,1
        mov esi,computationBuffer
        movzx ecx,byte[dataBufferLen]
        add esi,ecx
        sub esi,1
        ret ; 返回函数调用点




; 将数字转化成二进制保留在resultBuf中
; ecx 用于computationBuf的计数，edx用于resultBuf计数，esi用于CompuationBuf指针，edi用于resultBuf指针
Convert:
    ;resultBuf刚开始初始化，不用进入循环，一定要找号循环的依赖关系，哪些是需要进入循环处理的，哪些是不需要的，还是需求不明确，算法有误导致的编码错误
    mov edi, resultBuffer
    mov edx, 119
    add edi, edx; result指针指向最后一个
    ; 一直除，直到全部为0
    ToBinaryInnerLoop:
        call is_all_zeros
        test al,al
        jnz  Convert_Ret; 如果已经全部是0了，就可以输出了,先检查是否所有为0,此时esi就会指向最后一个位置

        ; 到这里esi已经指向最后一个
        ;每次循环前,先查看最后一个位置是奇数还是偶数

        mov eax, computationBuffer
        movzx ebx, byte[dataBufferLen]
        add eax,ebx ;获取最后一个位置的指针
        sub eax,1
        mov al,byte[eax]

        and al,1
        cmp al, 1; 如果是奇
        jne EvenLabel

        OddLabel:
            mov byte[edi],1
            jmp DivAllBy2

        EvenLabel:
            mov byte[edi],0
            jmp DivAllBy2
    

        DivAllBy2:
        xor ah,ah
        xor al,al
        mov al,byte[esi] ; 被除数存储在al中
        mov bl, 2; 除数放在bl中
        div bl ; div指令后面跟的是除数，商会放在alow中，余数会放在ahigh中
        
        mov byte[esi],al
        cmp ah,1 ;如果余数等于1，就在下一个字节处+5，否则就直接读取最后一个
        je RemainingOneLabel
        jne End_Label_1

        ;如果中间除的时候有余数1,丢给下一位
        RemainingOneLabel:
            ; 开大一点空间，即使给最后一位的后一位+5也只是忽略，不会报错
            add byte[esi+1],5
            jmp End_Label_1

        End_Label_1:
            dec esi ; computation指针左移一个
            dec ecx ;computation计数器减少一个
            cmp ecx,0
            jne DivAllBy2 ; 如果还没有把所有的都除以2,就还得再处理一下

            ;运行到这里的时候就是内部循环结束了
            dec edi; result的指针左移一个
            dec edx; result的计数左移一个
            jmp ToBinaryInnerLoop; 不然就跳出循环，回到大的循环
    Convert_Ret:
        ret

    ; 已经全部除以2了，再检查一下最后一位是奇数还是偶数，如果是奇数就在resultBuf的最低位置补1，否则0

OutputBinary:
    mov ecx, -1
    mov edi, -1
    ;消除所有的前导0
    InnerLoop_1:
        inc ecx
        inc edi ; 计算前导0的数量
        cmp byte[resultBuffer + ecx],"0"
        je InnerLoop_1

    call OutputBinaryPrefix
    ;可以开始输出了
    mov eax, 4 ; 系统调用号：write
    mov ebx, 1 ; 文件描述符：标准输出
    ;由于内存地址无法直接和寄存器相加，所以折中
    mov ecx, resultBuffer
    add ecx, edi
    mov edx, 120
    sub edx, edi
    int 0x80 ; 调用系统调用

    ;回到getInput
    call OutputNewLine
    jmp getInput

    
        
OutputOctal:
    mov ecx, 2
    mov esi, resultBuffer
    add esi, 2
    InnerLoop_2:
        cmp byte[esi - 2],"1"
        jne fourBit
        mov al, 4
        mov bl, byte[esi]
        add bl, al
        mov byte[esi],bl

        fourBit:
        cmp byte[esi-1],"1"
        jne fourBit_End
        mov al, 2
        mov bl, byte[esi]
        add bl, al
        mov byte[esi],bl

        fourBit_End:
        add ecx, 3
        add esi, 3
        cmp ecx, 122
        jne InnerLoop_2

    ;遍历40个位置，删掉所有的前导0
    omittingAllPreZero:
        mov esi, resultBuffer
        add esi,2
        mov ecx,2
        sub esi,3
        sub ecx, 3
        Octal_omittingAllPreZero_loop:
            add esi,3
            add ecx, 3
            cmp ecx,122
            je Octal_Inner_Output
            ;上面相当于循环的末尾
            cmp byte[esi],"0"
            je Octal_omittingAllPreZero_loop

    ;此时esi是指向第一个不是0的结点
    Octal_Inner_Output:
        call OutputOctalPrefix
        sub esi,3
        sub ecx,3
        Octal_Inner_Output_loop:
        add esi,3
        add ecx,3

        ;上面相当于循环的末尾
        ;寄存器保护
        mov edi,ecx
        mov eax, 4 ; 系统调用号：write
        mov ebx, 1 ; 文件描述符：标准输出
        ;由于内存地址无法直接和寄存器相加，所以折中
        mov ecx, esi
        mov edx, 1
        int 0x80 ; 调用系统调用
        mov ecx, edi
        cmp ecx,119
        jne Octal_Inner_Output_loop
        call OutputNewLine
        jmp getInput
    
OutputHex:
    mov ecx, 3
    mov esi, resultBuffer
    add esi, 3
    OutputHex_Inner_Loop:
        cmp byte[esi - 3],"1"
        jne Hex_EightBit
        mov al, 8
        mov bl, byte[esi]
        add bl, al
        mov byte[esi],bl

        Hex_EightBit:
        cmp byte[esi-2],"1"
        jne Hex_FourBit
        mov al, 4
        mov bl, byte[esi]
        add bl, al
        mov byte[esi],bl

        Hex_FourBit:
        cmp byte[esi-1],"1"
        jne Hex_EndBit
        mov al, 2
        mov bl, byte[esi]
        add bl, al
        mov byte[esi],bl

        Hex_EndBit:
        add ecx, 4
        add esi, 4
        cmp ecx, 123
        jne OutputHex_Inner_Loop

    ;遍历40个位置，删掉所有的前导0
    Hex_omittingAllPreZero:
        mov esi, resultBuffer
        add esi,3
        mov ecx,3
        sub esi,4
        sub ecx, 4
        Hex_omittingAllPreZero_loop:
            add esi,4
            add ecx, 4
            cmp ecx,123
            je Octal_Inner_Output
            ;上面相当于循环的末尾
            cmp byte[esi],"0"
            je Hex_omittingAllPreZero_loop

    ;此时esi是指向第一个不是0的结点
    Hex_Inner_Output:
        call OutputHexPrefix
        sub esi,4
        sub ecx,4
        Hex_Inner_Output_loop:
        add esi,4
        add ecx,4

        ;转化成英文数字
        mov al,byte[esi]
        cmp al,58
        jl NotAboveTen
        cmp al,63
        jg NotAboveTen
        add al, 39
        mov byte[esi],al

        NotAboveTen:

        ;上面相当于循环的末尾
        ;保护寄存器
        mov edi,ecx
        mov eax, 4 ; 系统调用号：write
        mov ebx, 1 ; 文件描述符：标准输出
        ;由于内存地址无法直接和寄存器相加，所以折中
        mov ecx, esi
        mov edx, 1
        int 0x80 ; 调用系统调用
        mov ecx,edi
        cmp ecx,119
        jne Hex_Inner_Output_loop
        call OutputNewLine
        jmp getInput


ResetBuffer: ; 重新清空Buffer的内容,用于接受下一次输入
    ; 清空computaionBuffer和inputBuffer
    mov ecx, 36
    loop1:
        mov byte[inputBuffer +ecx - 1],0 ;重新赋值为0
        mov byte[computationBuffer + ecx - 1],0
        dec ecx
        cmp ecx, 0
        jne loop1

    ; 清空ecx
    xor ecx,ecx

    mov ecx,119

    loop2:
        mov byte[resultBuffer + ecx - 1],0 ;重新赋值为0
        dec ecx
        cmp ecx, 0
        jne loop2
    ; 清空resultBuffer

    ;清空resultBufferLen
    mov byte[dataBufferLen],0
    mov byte[dataWithBlankLen],0
    mov byte[inputBufferLen],0
    ret

OutputNewLine:
    mov eax, 4 ; 系统调用号：write
    mov ebx, 1 ; 文件描述符：标准输出
    ;由于内存地址无法直接和寄存器相加，所以折中
    mov ecx, NewLine
    mov edx, 1
    int 0x80 ; 调用系统调用
    ret

OutputBinaryPrefix:
    mov edi,ecx
    mov eax, 4 ; 系统调用号：write
    mov ebx, 1 ; 文件描述符：标准输出
    ;由于内存地址无法直接和寄存器相加，所以折中
    mov ecx, BinaryPrefix
    mov edx, 2
    int 0x80 ; 调用系统调用
    mov ecx,edi
    ret

OutputOctalPrefix:
    mov edi,ecx
    mov eax, 4 ; 系统调用号：write
    mov ebx, 1 ; 文件描述符：标准输出
    ;由于内存地址无法直接和寄存器相加，所以折中
    mov ecx, OctalPrefix
    mov edx, 2
    int 0x80 ; 调用系统调用
    mov ecx,edi
    ret

OutputHexPrefix:
    ; 保护一下ecx 
    mov edi,ecx
    mov eax, 4 ; 系统调用号：write
    mov ebx, 1 ; 文件描述符：标准输出
    ;由于内存地址无法直接和寄存器相加，所以折中
    mov ecx, HexPrefix
    mov edx, 2
    int 0x80 ; 调用系统调用
    mov ecx,edi
    ret

OutputError:
    ; 错误情况下的处理代码
    ; 输出错误信息
    mov eax, 4 ; 系统调用号：write
    mov ebx, 1 ; 文件描述符：标准输出
    mov ecx, ErrorMessage ; 错误信息的地址
    mov edx, 15 ; 错误信息的长度
    int 0x80 ; 调用系统调用F
    call OutputNewLine
    jmp quit