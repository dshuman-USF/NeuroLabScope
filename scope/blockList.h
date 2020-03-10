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



/* This is a header file for a data structure, which
   will be used for storing a list of marked blocks,
   which will used when filtering data output to a file.	*/


#define NIL 0
#define True 1
#define False 0



/* The following routines provide access to the list storage 
   structure.  The list implementation is kept hidden, and
    may change over time.					*/



extern void InitListTools();
	/* Initializes the block list toolbox.	*/

extern int InitList();
	/* Creates an initially empty list
	   and returns an integer list specifier, 
	   or NIL if the initialization failed.		*/

extern void ClearList();
	/* If the ListId is defined, all blocks will be removed
	   from the list.					*/

extern int DestroyMyList(int);
	/* int	blockList;	*/
	/* Empties and destroys a block list.
	   Returns 1 if successful, NIL if not.		*/


extern int FirstBlock(int, int*, int*);
	/* int	blockList;	*/
	/* int	begin;		*/
	/* int	end;		*/
	/* Returns the first block in the list, or NIL 
	   if there are no blocks in the list.		*/

int FirstHdtBlock(int ListId, int *begin, int *end, long long *offset);
	/* Returns the first block in the list, or NIL 
	   if there are no blocks in the list.		*/

extern int CurrentBlock(int, int*, int*);
	/* int	blockList;	*/
	/* int	begin;		*/
	/* int	end;		*/
	/* Returns the current block in the list, or NIL 
	   if there are no blocks in the list.	*/

extern int NextBlock(int, int*, int*);
	/* int	blockList;	*/
	/* int	begin;		*/
	/* int	end;		*/
	/* Returns the next block in the list, or NIL 
	   if there are no more blocks in the list.	*/

int NextHdtBlock(int ListId, int *begin, int *end, long long *offset);
	/* Returns the next block in the list, or NIL 
	   if there are no more blocks in the list.	*/

extern int PrevBlock(int, int*, int*);
	/* int	blockList;	*/
	/* int	begin;		*/
	/* int	end;		*/
	/* Returns the block before the current block,
	   or NIL if the current block is the first.	*/

extern int FindTime(int, int, int*, int*);
	/* int  Time;		*/
	/* int	blockList;	*/
	/* int	begin;		*/
	/* int	end;		*/
	/* Returns the block which contains or follows the time given
	   or NIL if the time is after all blocks in the blockList.	*/
  
extern int InsBlock(int, int, int);
	/* int	blockList;	*/
	/* int	begin		*/
	/* int	end		*/
	/* Returns 1 if insert was successfull, NIL if not.	*/	 

extern int InsHdtBlock(int ListId, int begin, int end, long long offset);
	/* Returns 1 if insert was successfull, NIL if not.	*/	 

extern int DelBlock(int, int, int);
	/* int	blockList;	*/
	/* int	begin		*/
	/* int	end		*/
	/* Returns 1 if deletion was successfull, NIL if not.	*/

extern int BlockCount(int);
	/* int blockList;	*/
	/* Returns the number of blocks in the given blockList. */

extern int TruncList(int, int, int);
	/* int blockList;	*/
	/* int	begin		*/
	/* int	end		*/
	/* Truncates the list at the specified block (including
	   the specified block).				*/
