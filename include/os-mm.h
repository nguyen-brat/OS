#ifndef OSMM_H
#define OSMM_H

#define MM_PAGING
#define PAGING_MAX_MMSWP 4 /* max number of supported swapped space */
#define PAGING_MAX_SYMTBL_SZ 30

typedef char BYTE;
typedef uint32_t addr_t;
//typedef unsigned int uint32_t;

struct pgn_t{
   int pgn;
   struct pgn_t *pg_next;
};

/*
 *  Memory region struct
 */
struct vm_rg_struct {
   unsigned long rg_start;
   unsigned long rg_end;

   struct vm_rg_struct *rg_next; // link list structure
};

/*
 *  Memory area struct
 */
struct vm_area_struct {
   unsigned long vm_id;
   unsigned long vm_start;
   unsigned long vm_end;

   unsigned long sbrk;
/*
 * Derived field
 * unsigned long vm_limit = vm_end - vm_start
 */
   struct mm_struct *vm_mm;
   struct vm_rg_struct *vm_freerg_list; // store free memory region in a linklist this is the head
   struct vm_area_struct *vm_next;
};

/* 
 * Memory management struct
 */
struct mm_struct {
   uint32_t *pgd; // manage the virtual memory it map virtual to disk. pgd[index] = value. index is page and value is adress (offset+frame)

   struct vm_area_struct *mmap;

   /* Currently we support a fixed number of symbol */
   struct vm_rg_struct symrgtbl[PAGING_MAX_SYMTBL_SZ];

   /* list of free page */
   struct pgn_t *fifo_pgn; // manage the order of pgn
};

/*
 * FRAME/MEM PHY struct
 */
struct framephy_struct {
   int fpn;
   struct framephy_struct *fp_next;

   /* Resereed for tracking allocated framed by virtual memory*/
   struct mm_struct* owner;
};

struct memphy_struct {
   /* Basic field of data and size */
   BYTE *storage; // BYTE is char storage maybe a string it is a array in size is maxsz
   int maxsz; // at begin it have int(maxsz/PAGINGSZ) frame
   
   /* Sequential device fields */ 
   int rdmflg; // defines the memory access is randomly or serially access
   int cursor;

   /* Management structure */
   struct framephy_struct *free_fp_list; //link list store head
   struct framephy_struct *used_fp_list; // link list store head
};

#endif
