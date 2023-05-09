//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Memory physical module mm/mm-memphy.c
 */
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

/*  
 *  MEMPHY_mv_csr - move MEMPHY cursor
 *  @mp: memphy struct
 *  @offset: offset
 */
/*
MEMPHY_mv_csr: This function moves the cursor of a memphy_struct to the specified offset index. 
The command line syntax is: MEMPHY_mv_csr <memphy_struct> <offset>
*/
int MEMPHY_mv_csr(struct memphy_struct *mp, int offset) // move cursor to offset index
{
   int numstep = 0;

   mp->cursor = 0;
   while(numstep < offset && numstep < mp->maxsz){
     /* Traverse sequentially */
     mp->cursor = (mp->cursor + 1) % mp->maxsz;
     numstep++;
   }

   return 0;
}

/*
 *  MEMPHY_seq_read - read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
/*
MEMPHY_seq_read: This function reads the value at the specified address in the memphy_struct when it is in sequential access mode. 
The command line syntax is: MEMPHY_seq_read <memphy_struct> <addr> <value>
*/
int MEMPHY_seq_read(struct memphy_struct *mp, int addr, BYTE *value) // assign the value in addr in mp to value
{
   if (mp == NULL)
     return -1;

   if (!mp->rdmflg) // rdmflg define random access or serial
     return -1; /* Not compatible mode for sequential read */

   MEMPHY_mv_csr(mp, addr); // just to take time to modify moving pointer in hard disk.
   *value = (BYTE) mp->storage[addr];

   return 0;
}

/*
 *  MEMPHY_read read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
/*
MEMPHY_read: This function reads the value at the specified address in the memphy_struct. 
If it is in sequential access mode, it calls MEMPHY_seq_read
Otherwise, it directly reads from the storage. 
The command line syntax is: MEMPHY_read <memphy_struct> <addr> <value>
*/
int MEMPHY_read(struct memphy_struct * mp, int addr, BYTE *value)
{
   if (mp == NULL)
     return -1;

   if (mp->rdmflg)
      *value = mp->storage[addr];
   else /* Sequential access device */
      return MEMPHY_seq_read(mp, addr, value);

   return 0;
}

/*
 *  MEMPHY_seq_write - write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
/*
MEMPHY_seq_write: This function writes the specified data to the address in the memphy_struct when it is in sequential access mode. 
The command line syntax is: MEMPHY_seq_write <memphy_struct> <addr> <data>
*/
int MEMPHY_seq_write(struct memphy_struct * mp, int addr, BYTE value)
{

   if (mp == NULL)
     return -1;

   if (!mp->rdmflg)
     return -1; /* Not compatible mode for sequential read */

   MEMPHY_mv_csr(mp, addr);
   mp->storage[addr] = value;

   return 0;
}

/*
 *  MEMPHY_write-write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
/*
MEMPHY_write: This function writes the specified data to the address in the memphy_struct. 
If it is in sequential access mode, it calls MEMPHY_seq_write. Otherwise, it directly writes to the storage. 
The command line syntax is: MEMPHY_write <memphy_struct> <addr> <data>
*/
int MEMPHY_write(struct memphy_struct * mp, int addr, BYTE data)
{
   if (mp == NULL)
     return -1;

   if (mp->rdmflg)
      mp->storage[addr] = data;
   else /* Sequential access device */
      return MEMPHY_seq_write(mp, addr, data);

   return 0;
}

/*
 *  MEMPHY_format-format MEMPHY device
 *  @mp: memphy struct
 */
/*
MEMPHY_format: This function formats the memphy_struct by setting up the free frame list for the specified page size. 
The command line syntax is: MEMPHY_format <memphy_struct> <page_size>
*/
int MEMPHY_format(struct memphy_struct *mp, int pagesz)
{
    /* This setting come with fixed constant PAGESZ */
    int numfp = mp->maxsz / pagesz;
    struct framephy_struct *newfst, *fst;
    int iter = 0;

    if (numfp <= 0)
      return -1;

    /* Init head of free framephy list */ 
    fst = malloc(sizeof(struct framephy_struct));
    fst->fpn = iter;
    mp->free_fp_list = fst;

    /* We have list with first element, fill in the rest num-1 element member*/
    for (iter = 1; iter < numfp ; iter++)
    {
       newfst =  malloc(sizeof(struct framephy_struct));
       newfst->fpn = iter;
       newfst->fp_next = NULL;
       fst->fp_next = newfst;
       fst = newfst;
    }

    return 0;
}
/*
MEMPHY_get_freefp: This function returns the first free frame from the free frame list of the memphy_struct. 
The command line syntax is: MEMPHY_get_freefp <memphy_struct> <retfpn>
*/
int MEMPHY_get_freefp(struct memphy_struct *mp, int *retfpn) // information of freefp pass into retfpn
{
   struct framephy_struct *fp = mp->free_fp_list;

   if (fp == NULL)
     return -1;

   *retfpn = fp->fpn;
   mp->free_fp_list = fp->fp_next;

   /* MEMPHY is iteratively used up until its exhausted TODO here
    * No garbage collector acting then it not been released
    */

   free(fp);

   return 0;
}
/*
MEMPHY_dump: This function dumps the content of the memphy_struct. 
The command line syntax is: MEMPHY_dump <memphy_struct>
*/
int MEMPHY_dump(struct memphy_struct * mp) // is dump is print all content of memory ?
{
    /*TODO dump memphy contnt mp->storage
     *     for tracing the memory content
     */
    printf("%s\n", mp->storage);
    return 0;
}
/*
MEMPHY_put_freefp: This function puts the specified free frame number to the free frame list of the memphy_struct. 
The command line syntax is: MEMPHY_put_freefp <memphy_struct> <fpn>
*/
int MEMPHY_put_freefp(struct memphy_struct *mp, int fpn)
{
   struct framephy_struct *fp = mp->free_fp_list;
   struct framephy_struct *newnode = malloc(sizeof(struct framephy_struct));

   /* Create new node with value fpn */
   newnode->fpn = fpn;
   newnode->fp_next = fp;
   mp->free_fp_list = newnode;

   return 0;
}


/*
 *  Init MEMPHY struct
 */
/*
init_memphy: This function initializes the memphy_struct. 
The command line syntax is: init_memphy <memphy_struct> <max_size> <randomfl
*/
int init_memphy(struct memphy_struct *mp, int max_size, int randomflg)
{
   mp->storage = (BYTE *)malloc(max_size*sizeof(BYTE));
   mp->maxsz = max_size;

   MEMPHY_format(mp,PAGING_PAGESZ);

   mp->rdmflg = (randomflg != 0)?1:0;

   if (!mp->rdmflg )   /* Not Ramdom acess device, then it serial device*/
      mp->cursor = 0;

   return 0;
}

//#endif
