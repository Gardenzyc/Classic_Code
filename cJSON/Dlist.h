#ifndef __DLIST_H__
#define __DLIST_H__

#ifdef __cplusplus
extern "C"{
#endif

typedef struct Dlist{
	struct Dlist *Pre,*Next;
}Dlist_t; 

#define OFF_SET_OF(TYPE, MEMBER) ((char *)&((TYPE *)0)->MEMBER)
#define GET_CONTAINER_OF(P, T, M) ((T *)((char *)(P) - OFF_SET_OF(T, M)))
//#define GET_CLASS_CONTAINER_OF(P, C, M) ((C *)((char *)(P) - ((char *)&(M) - (char *)this )))
#define DLIST_INIT(DLIST) ({&DLIST,&DLIST})

static void DlistInit(Dlist_t *Dlist)
{
	Dlist->Pre = Dlist->Next = Dlist;
}

static int DlistIsEmpty(Dlist_t *Dlist)
{
	return (Dlist->Next == Dlist);
}

/* 遍历获取节点信息*/
static Dlist_t *DlistQueryNext(Dlist_t *Dlist, Dlist_t *Node)
{
	if(DlistIsEmpty(Dlist))
		return NULL;

	if(!Node)
		return Dlist->Next;
	else
	{
		if(Node->Next == Dlist)
			return NULL;
		else
			return Node->Next;
	}
}

static Dlist_t *DlistQueuePre(Dlist_t *Dlist, Dlist_t *Node)
{
	if(DlistIsEmpty(Dlist))
		return NULL;

	if(!Node)
		return Dlist->Pre;
	else
	{
		if(Node->Pre == Dlist)
			return NULL;
		else
			return Node->Pre;
	}
}

static void DlistRemove(Dlist_t *Dlist)
{
	Dlist->Pre->Next = Dlist->Next;
	Dlist->Next->Pre = Dlist->Pre;
	DlistInit(Dlist);
}

static void DlistAdd(Dlist_t *Dlist, Dlist_t *Node)
{
	Node->Next = Dlist->Next;
	Node->Pre = Dlist;
	Dlist->Next->Pre = Node;
	Dlist->Next = Node;
}

static void DlistAddTail(Dlist_t *Dlist, Dlist_t *Node)
{
	Node->Next = Dlist;
	Node->Pre = Dlist->Pre;
	Dlist->Pre->Next = Node;
	Dlist->Pre = Node;
}

static Dlist_t *DlistGet(Dlist_t *Dlist)
{
	Dlist_t *Node = NULL;
	if(!DlistIsEmpty(Dlist))
	{
		Node = Dlist->Next;
		DlistRemove(Node);
	}

	return Node;
}

static Dlist_t *DlistGetTail(Dlist_t *Dlist)
{
	Dlist_t *Node = NULL;
	if(!DlistIsEmpty(Dlist))
	{
		Node = Dlist->Pre;
		DlistRemove(Node);
	}

	return Node;
}

static void DlistMoveLists(Dlist_t *Dlist, Dlist_t *Nodes)
{
	if(DlistIsEmpty(Dlist))
		return;

	Dlist->Pre->Next = Nodes->Next;
	Nodes->Next->Pre = Dlist->Pre;
	Nodes->Next = Dlist->Next;
	Dlist->Next->Pre = Nodes;
	DlistInit(Dlist);
}

static void DlistMoveListsToTail(Dlist_t *Dlist, Dlist_t *Nodes)
{
	if(DlistIsEmpty(Dlist))
		return;

	Dlist->Next->Pre = Nodes->Pre;
	Nodes->Pre->Next = Dlist->Next;
	Nodes->Pre = Dlist->Pre;
	Dlist->Pre->Next = Nodes;
	DlistInit(Dlist);
}

static unsigned int DlistCount(Dlist_t *Dlist)
{
	unsigned int Count = 0;
	Dlist_t *Node = NULL;
	while(NULL != (Node = DlistQueryNext(Dlist, Node)))
		Count++;

	return Count;
}

#ifdef __cplusplus
}
#endif

#endif