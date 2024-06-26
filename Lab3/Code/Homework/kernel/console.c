
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
				  console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
							Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/
//

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"


PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE *p_con);
/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY *p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1; /* 显存总大小 (in WORD) */

	int con_v_mem_size = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;
	p_tty->p_console->pos_stack.ptr = 0;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	if (nr_tty == 0)
	{
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else
	{
		out_char(p_tty->p_console, nr_tty + '0');
		out_char(p_tty->p_console, '#');
	}

	set_cursor(p_tty->p_console->cursor);
}

/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE *p_con)
{
	return (p_con == &console_table[nr_current_console]);
}

/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE *p_con, char ch)
{
	u8 *p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);

	switch (ch)
	{
	case '\n':
		if (p_con->cursor < p_con->original_addr +
								p_con->v_mem_limit - SCREEN_WIDTH)
		{
			push_pos(p_con, p_con->cursor);
			p_con->cursor = p_con->original_addr + SCREEN_WIDTH *
													   ((p_con->cursor - p_con->original_addr) /
															SCREEN_WIDTH +
														1);
		}
		break;
	case '\b':
		if (p_con->cursor > p_con->original_addr)
		{
			/*
			p_con->cursor--;
			*(p_vmem - 2) = ' ';
			*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
			*/
			int prePos = pop_pos(p_con);
			if (prePos == -1 ||  (prePos < ESCBeginPos && ESCBeginPos > 1) ){
			break;
			}				
			else
			{
				for (int i = prePos; i < p_con->cursor; ++i)
				{
					*(p_vmem - 2) = ' ';
					*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
					p_vmem -= 2;
					if (mode == ESCMODE){
						ESCInputLength-=1;
					}
				}
				p_con->cursor = prePos;
			}
		}
		break;
	case '\t':
		if (p_con->cursor < p_con->original_addr +
								p_con->v_mem_limit - Indention)
		{
			int i;
			for (i = 0; i < Indention; ++i)
			{ // 用空格填充

				*p_vmem++ = ' ';

				*p_vmem++ = DEFAULT_CHAR_COLOR;
			}
			push_pos(p_con, p_con->cursor);

			p_con->cursor += Indention; // 调整光标
		}
		break;
	default:
		if (p_con->cursor <
			p_con->original_addr + p_con->v_mem_limit - 1)
		{
			push_pos(p_con, p_con->cursor);
			*p_vmem++ = ch;
			if (mode == INSERTMODE)
			{
				*p_vmem++ = DEFAULT_CHAR_COLOR;
			}
			else
			{
				*p_vmem++ = RED;
				ESCInputLength++;
				/*
				 *p_vmem ++ = '0' + ESCInputLength;
				 *p_vmem ++ = DEFAULT_CHAR_COLOR;
				 *p_vmem -= 2;*/
			}
			p_con->cursor++;
		}
		break;
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE)
	{
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}

/*======================================================================*
						   flush
*======================================================================*/
PRIVATE void flush(CONSOLE *p_con)
{
	set_cursor(p_con->cursor);
	set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
				set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}

PUBLIC void EXITEsc(CONSOLE *p_con)
{
	//out_byte(p_con,'0');
	u8 *p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);
	for (int i = 0; i < ESCInputLength; ++i)
	{

		*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
		*(p_vmem - 2) = ' ';
		p_vmem -= 2;
	}
	while (pop_pos(p_con) != ESCBeginPos);
	p_con->cursor = ESCBeginPos;
	p_vmem = (u8*)(V_MEM_BASE);
	for (int i = 0; i < p_con->cursor; ++i){
		*(p_vmem + 1) = DEFAULT_CHAR_COLOR;
		p_vmem += 2;
	}
	ESCInputLength = 0;
	ESCBeginPos = 0;
	flush(p_con);
}

/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console) /* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES))
	{
		return;
	}

	nr_current_console = nr_console;

	set_cursor(console_table[nr_console].cursor);
	set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE *p_con, int direction)
{
	if (direction == SCR_UP)
	{
		if (p_con->current_start_addr > p_con->original_addr)
		{
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN)
	{
		if (p_con->current_start_addr + SCREEN_SIZE <
			p_con->original_addr + p_con->v_mem_limit)
		{
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else
	{
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}

/*处理光标位置*/
PRIVATE void push_pos(CONSOLE *p_con, int pos)
{

	p_con->pos_stack.pos[p_con->pos_stack.ptr++] = pos;
}

PRIVATE int pop_pos(CONSOLE *p_con)
{

	if (p_con->pos_stack.ptr == 0)
	{

		return -1; // 不会发生这种情况
	}
	else
	{

		--p_con->pos_stack.ptr;

		return p_con->pos_stack.pos[p_con->pos_stack.ptr];
	}
}

PUBLIC void search(CONSOLE *p_con);

/*======================================================================*

		新增方法，搜索，并将搜索结果标为红色

 *======================================================================*/

PUBLIC void search(CONSOLE *p_con){

	int i,j;

	int begin,end; // 滑动窗口

	//out_char(p_con,ESCBeginPos + '0');
	for(i = 0; i < ESCBeginPos*2;i+=2){ // 遍历原始白色输入

		begin = end = i; // 初始化窗口为0

		int found = 1; // 是否匹配
		int k ;

		// 遍历匹配
		for(j = begin,k = 0;k<ESCInputLength*2;j+=2,k+=2){
			if(*((u8*)(V_MEM_BASE + j))==*((u8*)(V_MEM_BASE + ESCBeginPos*2 + k))){

				end+=2;

			}else{
				found = 0;
				break;
			}
		}

		// 如果找到，标红

		if(found == 1){

			for(j = begin;j<end;j+=2){

				*(u8*)(V_MEM_BASE + j + 1) = RED;

			}

		}

	}

}
