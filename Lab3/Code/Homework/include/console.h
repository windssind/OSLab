#pragma once
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef _ORANGES_CONSOLE_H_
#define _ORANGES_CONSOLE_H_

#define SCR_UP	1	/* scroll forward */
#define SCR_DN	-1	/* scroll backward */

#define SCREEN_SIZE		(80 * 25)
#define SCREEN_WIDTH		80

#define DEFAULT_CHAR_COLOR	0x07	/* 0000 0111 黑底白字 */

/*新增，记录光标曾在位置*/

typedef struct cursor_pos_stack 

{

	int ptr;

	int pos[SCREEN_SIZE];

}POSSTACK;

/* CONSOLE */
typedef struct s_console
{
	unsigned int	current_start_addr;	/* 当前显示到了什么位置	  */
	unsigned int	original_addr;		/* 当前控制台对应显存位置 */
	unsigned int	v_mem_limit;		/* 当前控制台占的显存大小 */
	unsigned int	cursor;			/* 当前光标位置 */
	POSSTACK pos_stack;	/*新增*/
}CONSOLE;

PRIVATE void push_pos(CONSOLE* p_con,int pos); // 用于将光标的位置压入栈中

PRIVATE int pop_pos(CONSOLE* p_con); // 用于从栈中取出光标的位置

PUBLIC void EXITEsc(CONSOLE* p_con);


#endif /* _ORANGES_CONSOLE_H_ */
