/*
Heap Assignment for OS
GROUP Assignment

Name: Archit Jaiswal 1001543326
      Christian Teeples 1001122564

*/
#include <limits.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)      ((b) + 1)
#define BLOCK_HEADER(ptr)   ((struct _block *)(ptr) - 1)


static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

struct _block
{
   size_t  size;         /* Size of the allocated _block of memory in bytes */
   struct _block *prev;  /* Pointer to the previous _block of allcated memory   */
   struct _block *next;  /* Pointer to the next _block of allcated memory   */
   bool   free;          /* Is this _block free?                     */
   char   padding[3];
};


struct _block *freeList = NULL; /* Free list to track the _blocks available */
//freelist is our heap
struct _block *curr_from_last_iteration;
bool firstIteration = true;
/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct _block *findFreeBlock(struct _block **last, size_t size)
{
   num_requested++;
   struct _block *curr = freeList;

#if defined FIT && FIT == 0
   /* First fit */

   while (curr && !(curr->free && curr->size >= size))
   {
      *last = curr;
      curr  = curr->next;
   }
#endif

#if defined BEST && BEST == 0
   struct _block *temp1 = curr;

   //We are starting with next fit just to give curr a starting value
   while(curr && !(curr->free && curr->size >= size))
   {
      *last = curr;
      curr  = curr->next;
   }
   //we are going to iterate through all of the elements and switch out curr with temp1 if it is a better match
   while (temp1 && temp1->next != NULL)
   {
      temp1 = temp1->next;

      if ( temp1->free && temp1->size >= size)
      {
         if (temp1->size < curr->size)
         {
            *last = temp1;
            curr = temp1;
         }
      }
   }
#endif

#if defined WORST && WORST == 0
   struct _block *temp1 = curr;

   //We are starting with next fit just to give curr a starting value
   while(curr && !(curr->free && curr->size >= size))
   {
      *last = curr;
      curr  = curr->next;
   }
   //we are going to iterate through all of the elements and switch out curr with temp1 if it is a worse match
   while (curr && temp1->next != NULL)
   {
      temp1 = temp1->next;

      if ( temp1->free && temp1->size >= size)
      {
         if (temp1->size > curr->size)
         {
            *last = temp1;
            curr = temp1;
         }
      }
   }
#endif

#if defined NEXT && NEXT == 0
   //if this is our first iteration through the next fit algorithm, we will start out just like first fit
   if (firstIteration)
   {
      while (curr && !(curr->free && curr->size >= size))
      {
         *last = curr;
         curr  = curr->next;
      }
      firstIteration = false;
      curr_from_last_iteration = curr;
   }
   //on our 2nd iteration through next fit, we want to start where we left off. If we start in the middle and don't find anything that works,
   //we will continue back from the beginning
   else
   {
      bool firstLoopWorked = false;

      while (curr_from_last_iteration && !(curr_from_last_iteration->free && curr_from_last_iteration->size >= size))
      {
         *last = curr_from_last_iteration;
         curr_from_last_iteration  = curr_from_last_iteration->next;
         if (curr_from_last_iteration->free && curr_from_last_iteration->size >= size)
         {
            firstLoopWorked = true;

            curr = curr_from_last_iteration;
         }
      }
      if (firstLoopWorked == false)
      {
         while (curr && !(curr->free && curr->size >= size))
         {
            *last = curr;
            curr  = curr->next;
         }

         curr_from_last_iteration = curr;
      }

   }

#endif

   if (curr != NULL) { num_blocks++;}
   return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size)
{
   num_grows++;
   num_blocks++;

   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1)
   {
      return NULL;
   }

   /* Update freeList if not set */
   if (freeList == NULL)
   {
      freeList = curr;
   }

   /* Attach new _block to prev _block */
   if (last)
   {
      last->next = curr;
   }

   /* Update _block metadata */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;

   max_heap = max_heap + size;
   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process
 * or NULL if failed
 */

void *malloc(size_t size)
{
   //num_mallocs++;
   num_requested = num_requested + size;

   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }

   /* Handle 0 size */
   if (size == 0)
   {
      return NULL;
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Look for free _block */
   struct _block *last = freeList;
   struct _block *next = findFreeBlock(&last, size);

   /* TODO: Split free _block if possible */
   if (next && (next->size - size) > 0)
   {
      //use (char*) to cast arithmetic to amount of bites instead of amount of sizeof(block)
      num_splits++;
      num_reuses++;
      num_blocks++;

      last->size = size;
      last->next = next->next;
      next->next = (void*)next + sizeof(struct _block) + (next->size - size);
      next->next->next = last->next;
      next->next->size = last->size - (next->size - size) - sizeof(struct _block);
      next->next->free = true;

   }

   /* Could not find free _block, so grow heap */
   if (next == NULL)
   {
      next = growHeap(last, size);
   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL)
   {
      return NULL;
   }

   /* Mark _block as in use */
   next->free = false;
   num_mallocs++;

   /* Return data address associated with _block */
   return BLOCK_DATA(next);
}

void *calloc(size_t nmb, size_t size_value)
{
   void *returnVal = malloc(size_value);

   memset(returnVal, 0, nmb);

   num_mallocs--; //we want to counteract the increment in malloc bc calloc does not count as malloc

   return BLOCK_DATA(returnVal);
}

void *realloc(void *curr, size_t size_value)
{
  if(curr != NULL)
  {
   struct _block *returnVal = (struct _block *) malloc(size_value);

   memcpy(BLOCK_DATA(returnVal), curr, BLOCK_HEADER(curr)->size);

   free(curr);

   num_mallocs--;

   return BLOCK_DATA(returnVal);
  }
 return NULL;
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr)
{

   if (ptr == NULL)
   {
      return;
   }

   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;

   /* TODO: Coalesce free _blocks if needed */

   curr = freeList;
   if (curr && curr->free && curr->prev && curr->prev->free)
   {
      num_coalesces++;
      num_blocks--;
      curr->prev->size = curr->prev->size + curr->size + sizeof(struct _block);
      curr->prev->next = curr->next;
   }


   if(curr && curr->free && curr->next && curr->next->free)
   {
      num_coalesces++;
      num_blocks--;
      curr->size = curr->size + curr->next->size + sizeof(struct _block);
      curr->next = curr->next->next;
   }

   num_frees++;
}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
