/*
Copyright 2004-2020 Kendall F. Morris

This file is part of the Scope software suite.

    The Scope software suite is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    The suite is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the suite.  If not, see <https://www.gnu.org/licenses/>.
*/



#include <stdio.h>
#include <stdlib.h>
#include "blockList.h"


struct block_list {
        int                     begin;
        int                     end;
        long long               offset;
        struct block_list       *prev;
        struct block_list       *next;
};

typedef struct block_list       ELEMENT;
typedef ELEMENT                 *LINK;


static struct{
	LINK	list;		/* a pointer to the list		*/
	int	defined;	/* a flag, 1 if defined, 0 if not	*/
	int	count;		/* # of blocks in list	 		*/
	LINK	current;	/* pointer to current record 		*/
} BlockList[17];

static int ListCount;


void InitListTools()
{
   int i;

   ListCount = 0;
   for(i=0;i<17;i++)
      BlockList[i].defined = False;
}


static void Delete(node)
     LINK node;
{
   if(node->prev != NIL)
      node->prev->next = node->next;
   if(node->next != NIL)
      node->next->prev = node->prev;

   free(node);
}


static LINK Find(ListId,begin,end)
	int ListId;
	int begin;
	int end;
{
   LINK Current;


   Current = BlockList[ListId].list->next;
   while((Current->begin != begin || Current->end != end)
	 && Current->next != NIL)
      Current = Current->next;

   if(Current->next == NIL)
      return(NIL);
   else
      return(Current);
}


static int NextIndex()
{
   int i;

   i = 1;
   while(BlockList[i].defined == True)
      i++;

   return(i);
}


int InitList()
{
   LINK Current;
   int index;

   ListCount++;
   index = NextIndex();

   BlockList[index].defined = True;
   BlockList[index].count = 0;
   BlockList[index].current = NIL;
   BlockList[index].list = (LINK) malloc(sizeof(ELEMENT));
   Current = BlockList[index].list;

   Current->begin = NIL;
   Current->end   = NIL;
   Current->prev  = NIL;

   Current->next  = (LINK) malloc(sizeof(ELEMENT));

   Current = Current->next;
	
   Current->begin = NIL;
   Current->end   = NIL;
   Current->next  = NIL;
   Current->prev  = BlockList[index].list;

   return(index);
}


void ClearList(ListId)
     int ListId;
{
   int count, i;

   if(BlockList[ListId].defined == True){
      count = BlockList[ListId].count;

      for(i=0;i<count;i++)
	 Delete(BlockList[ListId].list->next);

      BlockList[ListId].count = 0;
      BlockList[ListId].current = NIL;
   }
   else
      printf("List identifier %d undefined\n\n", ListId);
}


static int ValidId(ListId)
     int ListId;
{

   if(ListId < 0 || ListId >16){
      printf("List identifier out of range\n");
      return(False);
   }

   if(BlockList[ListId].defined == False){
      printf("List identifier %d undefined\n\n", ListId);
      return(False);
   }

   return(True);
}


int DestroyMyList(ListId)
     int ListId;
{
   int blockCount, i;

   if(ValidId(ListId) != True)
      return(NIL);

   blockCount = BlockList[ListId].count;

   for(i=0;i<blockCount;i++)
      DelBlock(ListId,BlockList[ListId].list->next->begin,
	       BlockList[ListId].list->next->end);

   Delete(BlockList[ListId].list->next);
   Delete(BlockList[ListId].list);
   BlockList[ListId].defined = False;
   ListCount--;
   return(1);
}
	

int FirstHdtBlock(int ListId, int *begin, int *end, long long *offset)
{
   if(ValidId(ListId) != True)
      return(NIL);

   if(BlockList[ListId].count > 0)
   {
      BlockList[ListId].current = BlockList[ListId].list->next;
      *begin  = BlockList[ListId].current->begin;	
      *end    = BlockList[ListId].current->end;
      *offset = BlockList[ListId].current->offset;
   } 
   else 
      return(NIL);  	

   return(1);
}

int FirstBlock(int ListId, int *begin, int *end)
{
   long long offset;
   return FirstHdtBlock (ListId, begin, end, &offset);
}

int NextHdtBlock(int ListId, int *begin, int *end, long long *offset)
{
   if(ValidId(ListId) != True)
      return(NIL);

   if(BlockList[ListId].current->next->next == NIL)
      return(NIL);

   BlockList[ListId].current = BlockList[ListId].current->next;

   *begin  = BlockList[ListId].current->begin;
   *end    = BlockList[ListId].current->end;
   *offset = BlockList[ListId].current->offset;

   return(1);
}

int NextBlock(int ListId, int *begin, int *end)
{
   long long offset;
   return NextHdtBlock (ListId, begin, end, &offset);
}

int PrevBlock(int ListId, int *begin, int *end)
{
   if(ValidId(ListId) != True)
      return(NIL);

   if(BlockList[ListId].current->prev->prev == NIL)
      return(NIL);

   BlockList[ListId].current = BlockList[ListId].current->prev;

   *begin = BlockList[ListId].current->begin;
   *end   = BlockList[ListId].current->end;

   return(1);
}


int CurrentBlock(int ListId, int *begin, int *end)
{
   if(ValidId(ListId) != True)
      return(NIL);

   if((BlockList[ListId].current->next == NIL)
      || (BlockList[ListId].current->prev == NIL))
      return(NIL);

   *begin = BlockList[ListId].current->begin;
   *end   = BlockList[ListId].current->end;

   return(1);
}



int FindTime(int Time, int ListId, int *begin, int *end)
{
   int found;
   int done;

   if(ValidId(ListId) != True || BlockList[ListId].count <= 0)
      return(NIL);

   found = NIL;
   done = False;

   while(done != True){

      if(Time > BlockList[ListId].current->end)

	 if(BlockList[ListId].current->next->next != NIL)
	    BlockList[ListId].current = BlockList[ListId].current->next;
	 else
	    done = True;

      else if(Time < BlockList[ListId].current->begin) 

	 if(BlockList[ListId].current->prev->prev == NIL){
	    found = 1;
	    done = True;
	 }
	 else if(Time <= BlockList[ListId].current->prev->end)
	    BlockList[ListId].current = BlockList[ListId].current->prev;
	 else {
	    found = 1;
	    done = True;
	 }

      else {
	 found = 1;
	 done = True;
      }
   }

   *begin = BlockList[ListId].current->begin;
   *end   = BlockList[ListId].current->end;
   return(found);
}

int InsHdtBlock(int ListId, int begin, int end, long long offset)
{
   LINK NewBlock, Current;
   int redundantBlock;

   if(ValidId(ListId) != True)
      return(NIL);

   /* locate insetion point of block based on begin time */

   Current = BlockList[ListId].list->next;

   while(begin > Current->begin && Current->next != NIL)
      Current = Current->next;

   /* check for redundant block */

   redundantBlock = False;
   if(Current->prev != NIL)
      if(end <= Current->prev->end)
	 redundantBlock = True;

   if(redundantBlock == False){
      /* create new block record and insert */

      NewBlock = (LINK) malloc(sizeof(ELEMENT));
      NewBlock->begin = begin;
      NewBlock->end   = end;
      NewBlock->offset = offset;
      NewBlock->prev  = Current->prev;
      NewBlock->next  = Current;

      NewBlock->prev->next = NewBlock;
      NewBlock->next->prev = NewBlock;
      BlockList[ListId].count++;
      BlockList[ListId].current = NewBlock;
	

      /* check for and handle overlap of block previous to new block */

      if(NewBlock->prev->prev != NIL && NewBlock->prev->end >= begin){
	 NewBlock->begin = NewBlock->prev->begin;
	 NewBlock->offset = NewBlock->prev->offset;
	 Delete(NewBlock->prev);
	 BlockList[ListId].count--;
      }


      /* check for and handle overlap of blocks after new block  */

      while(NewBlock->next->next != NIL 
	    && NewBlock->next->begin <= end){
	 if(NewBlock->end < NewBlock->next->end) 
	    NewBlock->end = NewBlock->next->end;
	 Delete(NewBlock->next);
	 BlockList[ListId].count--;
      }
   }
   Current = BlockList[ListId].list->next;
   return(1);
}

int InsBlock(int ListId, int begin, int end)
{
   return InsHdtBlock (ListId, begin, end, 0);
}

int DelBlock(int ListId, int begin, int end)
{
   LINK target;
	
   if(ValidId(ListId) != True)
      return(NIL);

   if((target = Find(ListId,begin,end)) != NIL) {
      Delete(target);
      BlockList[ListId].count--;

      if(BlockList[ListId].current == target)
	 BlockList[ListId].current = BlockList[ListId].list->next;
      return(1);
   }
   else 
      return(NIL);
}

int BlockCount(int ListId)
{
	return(BlockList[ListId].count);
}


int TruncList(int ListId, int begin, int end)
{
	LINK target;
	
	if(ValidId(ListId) != True)
	  return(NIL);

	if((target = Find(ListId,begin,end)) != NIL) {
	  while(target->next->next != NIL){
	    Delete(target->next);
	    BlockList[ListId].count--;
	  }
	  Delete(target);
	  BlockList[ListId].count--;

	  if(BlockList[ListId].current == target)
	    BlockList[ListId].current = BlockList[ListId].list->next;
	  return(1);
	}
	else 
	  return(NIL);
}

/* Local Variables: */
/* c-basic-offset: 3 */
/* c-file-offsets:  ((substatement-open . 0)) */
/* End: */
