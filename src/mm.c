//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */

#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include "flag.h"

/* 
 * init_pte - Initialize PTE entry
 */
/*
This function is used to initialize a Page Table Entry (PTE) based on the provided parameters. 
It takes a pointer to a PTE, and sets various bit fields based on the values of pre, fpn, drt, swp, swptyp, and swpoff. 
It returns 0 on success, or -1 on failure.
*/
int init_pte(uint32_t *pte,
             int pre,    // present
             int fpn,    // FPN
             int drt,    // dirty
             int swp,    // swap
             int swptyp, // swap type
             int swpoff) //swap offset
{
  if (pre != 0) {
    if (swp == 0) { // Non swap ~ page online
      if (fpn == 0) 
        return -1; // Invalid setting

      /* Valid setting with FPN */
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
    } else { // page swapped
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT); 
      SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
    }
  }

  return 0;   
}

/* 
 * pte_set_swap - Set PTE entry for swapped page
 * @pte    : target page table entry (PTE)
 * @swptyp : swap type
 * @swpoff : swap offset
 */
/*
This function is used to set the PTE entry for a swapped page. 
It takes a pointer to a PTE and the swap type and offset values. 
It sets various bit fields in the PTE to indicate that the page has been swapped out to disk. 
It returns 0 on success.
*/
int pte_set_swap(uint32_t *pte, int swptyp, int swpoff)
{
  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

  return 0;
}

/* 
 * pte_set_swap - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 */
/*
This function is used to set the PTE entry for an on-line page. 
It takes a pointer to a PTE and the frame page number (FPN) as input. 
It sets various bit fields in the PTE to indicate that the page is on-line and mapped to the given FPN. 
It returns 0 on success.
*/
int pte_set_fpn(uint32_t *pte, int fpn)
{
  SETBIT(*pte, PAGING_PTE_PRESENT_MASK); // set present bit to one
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK); // set swap bit to zero swap bit indicate is it in swap disk

  SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT); 

  return 0;
}


/* 
 * vmap_page_range - map a range of page at aligned address
 */
/*
This function is used to map a range of pages to physical frames in memory. 
It takes a pointer to the process control block (caller), the start address of the page range (addr), 
the number of pages in the range (pgnum), a list of physical frames (frames), 
and a return value ret_rg to hold the mapped region information. 
The function maps the pages to frames and updates the page table accordingly. It returns 0 on success.
*/
int vmap_page_range(struct pcb_t *caller, // process call
                                int addr, // start address which is aligned to pagesz
                               int pgnum, // num of mapping page
           struct framephy_struct **frames,// list of the mapped frames
              struct vm_rg_struct *ret_rg)// return mapped region, the real mapped fp
{                                         // no guarantee all given pages are mapped
  //uint32_t * pte = malloc(sizeof(uint32_t));
  //struct framephy_struct *fpit = malloc(sizeof(struct framephy_struct));
  //int  fpn;
  int pgit = 0;
  int pgn = PAGING_PGN(addr);

  ret_rg->rg_end = ret_rg->rg_start = addr; // at least the very first space is usable

  //fpit->fp_next = frames;

  /* TODO map range of frame to address space
   *      [addr to addr + pgnum*PAGING_PAGESZ]
   *      in page table caller->mm->pgd[]
   */
  for (pgit = 0; pgit<pgnum; pgit++){
    pte_set_fpn(&caller->mm->pgd[pgn + pgit], frames[pgit]->fpn);
    frames[pgit]->owner = caller->mm;
    frames[pgit]->fp_next = caller->mram->used_fp_list; // add this phyframe to used fram list
    caller->mram->used_fp_list = frames[pgit]; // add this phyframe to used fram list
   /* Tracking for later page replacement activities (if needed)
    * Enqueue new usage page */
    enlist_pgn_node(&caller->mm->fifo_pgn, pgn+pgit);
    ret_rg->rg_end += PAGING_PAGESZ - 1;
  }

  return 0;
}

/* 
 * alloc_pages_range - allocate req_pgnum of frame in ram
 * @caller    : caller
 * @req_pgnum : request page num
 * @frm_lst   : frame list
 */
/*
This function is used to allocate a range of physical frames in memory for a process. 
It takes a pointer to the process control block (caller), the requested number of pages (req_pgnum), 
and a pointer to a list of physical frames (frm_lst).
The function allocates the required frames and updates the page table accordingly. It returns 0 on success.
*/
struct framephy_struct* dequeue_last_frame(struct memphy_struct* phyframe){ // move last pointer in use fp to free fp, return the fpn of last frame
  if (phyframe->used_fp_list->fp_next == NULL){
    struct framephy_struct* result = phyframe->used_fp_list;
    phyframe->used_fp_list = NULL;
    return result;
  }
  struct framephy_struct* pointer = phyframe->used_fp_list;
  while (pointer->fp_next->fp_next != NULL)
    pointer = pointer->fp_next; // move to the near last pointer
  struct framephy_struct* result = pointer->fp_next; // take fpn of the last pointer
  // pointer->fp_next->fp_next = phyframe->free_fp_list; // add last pointer to free list
  // phyframe->free_fp_list = pointer->fp_next->fp_next; // make last pointer is the head
  pointer->fp_next = NULL; // set near last pointer next to null
  return result;
}

int alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct** frm_lst) // allocate content to physical disk
{ // if both ram and swap full return -1 if only ram full return -3000
  int pgit, fpn;
  //struct framephy_struct *newfp_str;
  //frm_lst = malloc(req_pgnum*sizeof(struct framephy_struct*));
  for(pgit = 0; pgit < req_pgnum; pgit++)
  {
    if(MEMPHY_get_freefp(caller->mram, &fpn) == 0)
   {
     // There must be a fucking TODO here why the fuck she dont put it here
     struct framephy_struct* alloc_new_frame = malloc(sizeof(struct framephy_struct));
     alloc_new_frame->fpn = fpn;
     alloc_new_frame->owner = caller->mm;
     frm_lst[pgit] = alloc_new_frame;
     //if (pgit > 0) frm_lst[pgit-1]->fp_next = alloc_new_frame;
   } else {  // ERROR CODE of obtaining somes but not enough frames maybe there must be a TODO here !
            // push data from ram to swap to have empty page. and store data.
      if(MEMPHY_get_freefp(caller->active_mswp, &fpn) == 0){
        // swap memory from ram to swap
        struct framephy_struct* remove_frame_ram = dequeue_last_frame(caller->mram);
        frm_lst[pgit] = remove_frame_ram;
        __swap_cp_page(caller->mram, remove_frame_ram->fpn, caller->active_mswp, fpn);
        struct framephy_struct* remove_frame_swap = malloc(sizeof(struct framephy_struct));
        remove_frame_swap->fpn = fpn;
        remove_frame_swap->fp_next = caller->active_mswp->free_fp_list;
        caller->active_mswp->free_fp_list = remove_frame_swap;
      } else return -1;
   }
 }

  return 0;
}


/* 
 * vm_map_ram - do the mapping all vm are to ram storage device
 * @caller    : caller
 * @astart    : vm area start
 * @aend      : vm area end
 * @mapstart  : start mapping point
 * @incpgnum  : number of mapped page
 * @ret_rg    : returned region
 */
/*
This function maps a range of virtual addresses to a range of physical frames in RAM. 
The function first tries to allocate the required number of frames using alloc_pages_range function. 
If allocation is successful, it maps the allocated frames to the virtual addresses using vmap_page_range function. 
If allocation fails due to lack of memory, the function returns -1. The caller process, the virtual address range to be mapped, 
and the number of frames to be allocated are provided as input arguments to this function.*/
int vm_map_ram(struct pcb_t *caller, int astart, int aend, int mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
{
  struct framephy_struct** frm_lst = malloc(incpgnum*sizeof(struct framephy_struct*));
  int ret_alloc;

  /*@bksysnet: author provides a feasible solution of getting frames
   *FATAL logic in here, wrong behaviour if we have not enough page
   *i.e. we request 1000 frames meanwhile our RAM has size of 3 frames
   *Don't try to perform that case in this simple work, it will result
   *in endless procedure of swap-off to get frame and we have not provide 
   *duplicate control mechanism, keep it simple
   */
  if (FLAG) printf("Flag 300\n");
  ret_alloc = alloc_pages_range(caller, incpgnum, frm_lst); // return 0 if successfull incpgnum is number of page need to map
  if (FLAG) printf("Flag 299\n");
  if (FLAG) printf("incpgnum is %d\n", incpgnum);
  if (FLAG) printf("num array %lu\n", incpgnum*sizeof(struct framephy_struct*)/sizeof(frm_lst[0]));
  if (ret_alloc < 0 && ret_alloc != -3000){
    if (FLAG) printf("Flag 298\n");
    return -1;
  }

  /* Out of memory */
  if (ret_alloc == -3000) 
  {
#ifdef MMDBG
  printf("OOM: vm_map_ram out of memory \n");
#endif
     return -1;
  }

  /* it leaves the case of memory is enough but half in ram, half in swap
   * do the swaping all to swapper to get the all in ram */
  if (FLAG) printf("Flag 297\n");
  vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);
  if (FLAG) printf("Flag 296\n");

  return 0;
}

/* Swap copy content page from source frame to destination frame 
 * @mpsrc  : source memphy
 * @srcfpn : source physical page number (FPN)
 * @mpdst  : destination memphy
 * @dstfpn : destination physical page number (FPN)
 **/
/*
This function copies the contents of a physical page at a source frame to a physical page at a destination frame. 
The source and destination frames are provided as input arguments, along with the page numbers within each frame.
*/
int __swap_cp_page(struct memphy_struct *mpsrc, int srcfpn,
                struct memphy_struct *mpdst, int dstfpn) 
{
  int cellidx;
  int addrsrc,addrdst;
  for(cellidx = 0; cellidx < PAGING_PAGESZ; cellidx++)
  {
    addrsrc = srcfpn * PAGING_PAGESZ + cellidx;
    addrdst = dstfpn * PAGING_PAGESZ + cellidx;

    BYTE data;
    MEMPHY_read(mpsrc, addrsrc, &data);
    MEMPHY_write(mpdst, addrdst, data);
  }

  return 0;
}

/*
 *Initialize a empty Memory Management instance
 * @mm:     self mm
 * @caller: mm owner
 */
/*
This function initializes a memory management instance. 
It creates a new virtual memory area (VMA) and adds it to the memory management's VMA list. 
It also initializes a new range structure and adds it to the free range list of the VMA. 
Finally, it sets the memory management's VMA pointer to the new VMA
*/
int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
  struct vm_area_struct * vma = malloc(sizeof(struct vm_area_struct));

  mm->pgd = malloc(PAGING_MAX_PGN*sizeof(uint32_t));

  /* By default the owner comes with at least one vma */
  vma->vm_id = 1;
  vma->vm_start = 0;
  vma->vm_end = vma->vm_start;
  vma->sbrk = vma->vm_start;
  struct vm_rg_struct *first_rg = init_vm_rg(vma->vm_start, vma->vm_end);
  enlist_vm_rg_node(&vma->vm_freerg_list, first_rg);
  // for (int i = 0; i<PAGING_MAX_SYMTBL_SZ; i++) {
  //   struct vm_rg_struct dump_rg; 
  //   mm->symrgtbl[i] = dump_rg;
  // }

  vma->vm_next = NULL;
  vma->vm_mm = mm; /*point back to vma owner */

  mm->mmap = vma;

  return 0;
}
/*
This function initializes a new range structure for a virtual memory area. 
It takes the starting and ending virtual addresses of the range as input arguments and returns a pointer to the new range structure.
*/
struct vm_rg_struct* init_vm_rg(int rg_start, int rg_end)
{
  struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));

  rgnode->rg_start = rg_start;
  rgnode->rg_end = rg_end;
  rgnode->rg_next = NULL;

  return rgnode;
}
/*
This function adds a range structure to the free range list of a virtual memory area. 
It takes a pointer to the head of the free range list and a pointer to the range structure to be added as input arguments.
*/
int enlist_vm_rg_node(struct vm_rg_struct **rglist, struct vm_rg_struct* rgnode)
{
  rgnode->rg_next = *rglist;
  *rglist = rgnode;

  return 0;
}
/*
This function adds a physical page number to a list of page numbers. 
It takes a pointer to the head of the list and the physical page number to be added as input arguments.
*/
int enlist_pgn_node(struct pgn_t **plist, int pgn)
{
  struct pgn_t* pnode = malloc(sizeof(struct pgn_t));

  pnode->pgn = pgn;
  pnode->pg_next = *plist;
  *plist = pnode;

  return 0;
}
/*This function prints the list of physical frames.*/
int print_list_fp(struct framephy_struct *ifp)
{
   struct framephy_struct *fp = ifp;
 
   printf("print_list_fp: ");
   if (fp == NULL) {printf("NULL list\n"); return -1;}
   printf("\n");
   while (fp != NULL )
   {
       printf("fp[%d]\n",fp->fpn);
       fp = fp->fp_next;
   }
   printf("\n");
   return 0;
}

int print_list_rg(struct vm_rg_struct *irg)
{
   struct vm_rg_struct *rg = irg;
 
   printf("print_list_rg: ");
   if (rg == NULL) {printf("NULL list\n"); return -1;}
   printf("\n");
   while (rg != NULL)
   {
       printf("rg[%ld->%ld]\n",rg->rg_start, rg->rg_end);
       rg = rg->rg_next;
   }
   printf("\n");
   return 0;
}

int print_list_vma(struct vm_area_struct *ivma)
{
   struct vm_area_struct *vma = ivma;
 
   printf("print_list_vma: ");
   if (vma == NULL) {printf("NULL list\n"); return -1;}
   printf("\n");
   while (vma != NULL )
   {
       printf("va[%ld->%ld]\n",vma->vm_start, vma->vm_end);
       vma = vma->vm_next;
   }
   printf("\n");
   return 0;
}

int print_list_pgn(struct pgn_t *ip)
{
   printf("print_list_pgn: ");
   if (ip == NULL) {printf("NULL list\n"); return -1;}
   printf("\n");
   while (ip != NULL )
   {
       printf("va[%d]-\n",ip->pgn);
       ip = ip->pg_next;
   }
   printf("n");
   return 0;
}

int print_pgtbl(struct pcb_t *caller, uint32_t start, uint32_t end)
{
  int pgn_start,pgn_end;
  int pgit;

  if(end == -1){
    pgn_start = 0;
    struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, 0);
    end = cur_vma->vm_end;
  }
  pgn_start = PAGING_PGN(start);
  pgn_end = PAGING_PGN(end);

  printf("print_pgtbl: %d - %d", start, end);
  if (caller == NULL) {printf("NULL caller\n"); return -1;}
    printf("\n");


  for(pgit = pgn_start; pgit < pgn_end; pgit++)
  {
     printf("%08ld: %08x\n", pgit * sizeof(uint32_t), caller->mm->pgd[pgit]);
  }

  return 0;
}

//#endif
