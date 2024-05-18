org 07c00h; 告诉汇编器程序加载到07c00h处
mov ax,cs
mov ds, ax
mov es, ax
call DispStr ; //调用显示字符串例程
jmp $; 无限循环


DispStr:
mov ax, BootMessage
mov bp, ax ; ES:BP = 串地址 // BP存储的是段地址
mov cx, 10; CX= 串长度
mov ax,01301h; AH= 13, AL = 01h
mov bx, 000ch; 页号为0(BH = 0),黑底红字（BL = 0Ch，高亮）
mov dl,0
int 10h; 10h号中断
ret

BootMessage : db "Hello,OS!"
times 510-($-$$) db 0;填充剩下的空间,使剩下的二进制代码恰好为512字节
dw 0xaa55;结束标志

