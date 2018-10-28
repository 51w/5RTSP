#ifndef _LIST_H_
#define _LIST_H_

/**********************************************************
功能: 计算MEMBER成员在TYPE结构体中的偏移量
**********************************************************/
#define offsetof(TYPE, MEMBER) (unsigned long)(&(((TYPE*)0)->MEMBER))

/********************************************************** 
功能: 计算链表元素的起始地址 
输入:  
    ptr：    type结构体中的链表指针 
    type:   结构体类型 
    member: 链表成员名称 
**********************************************************/  
#define container_of(ptr, type, member) (type *)((char*)(ptr) - offsetof(type, member))  
  
#define LIST_HEAD_INIT(name)    {&(name), &(name)}  
  
struct list  
{  
    struct list *prev, *next;  
};  
  
static inline void list_init(struct list *list)  
{  
    list->next = list;  
    list->prev = list;  
}  
  
static inline int list_empty(struct list *list)  
{  
    return list->next == list;  
}  
  
// 将new_link插入到link之前  
static inline void list_insert(struct list *link, struct list *new_link)  
{  
    new_link->prev        = link->prev;  
    new_link->next        = link;  
    new_link->prev->next = new_link;  
    new_link->next->prev = new_link;  
}  
  
/********************************************************** 
功能: 将new_link节点附加到list链表中 
**********************************************************/  
static inline void list_append(struct list *list, struct list *new_link)  
{  
    list_insert(list, new_link);  
}  
  
/********************************************************** 
    功能: 从链表中移除节点 
**********************************************************/  
static inline void list_remove(struct list *link)  
{  
    link->prev->next = link->next;  
    link->next->prev = link->prev;  
}  
  
/********************************************************** 
获取link节点对应的结构体变量地址 
link:   链表节点指针 
type:   结构体类型名 
member: 结构体成员变量名 
**********************************************************/  
#define list_entry(link, type, member)  container_of(link, type, member)  
  
  
/********************************************************** 
获取链表头节点对应的结构体变量地址 
list:   链表头指针 
type:   结构体类型名 
member: 结构体成员变量名 
Note: 
链表头节点实际为链表头的下一个节点,链表头未使用，相当于哨兵 
**********************************************************/  
#define list_head(list, type, member) list_entry((list)->next, type, member)  
  
/********************************************************** 
获取链表尾节点对应的结构体变量地址 
list:   链表头指针 
type:   结构体类型名 
member: 结构体成员变量名 
**********************************************************/  
#define list_tail(list, type, member) list_entry((list)->prev, type, member)  
  
/********************************************************** 
返回链表下一个节点对应的结构体指针 
elm:    结构体变量指针 
type:   结构体类型名 
member: 结构体成员变量名(链表变量名) 
**********************************************************/  
#define list_next(elm,type,member) list_entry((elm)->member.next, type, member)  
  
/********************************************************** 
遍历链表所有节点对应的结构体 
pos : 结构体指针 
type : 结构体类型名 
list : 链表头指针 
member : 结构体成员变量名(链表变量名) 
Note : 链表头未使用，因此遍历结束后，pos指向的不是有效的结构体地址 
**********************************************************/
#define list_for_each_entry(pos, type, list, member) \
for(pos = list_head(list, type, member); \
	&pos->member != (list); \
	pos = list_next(pos, type, member))
	

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
	pos = n, n = pos->next)	
	
	
#endif 