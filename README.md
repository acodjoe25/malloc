# 7-malloc
To maintain compaction, we created a function coalesce() 
which first checks if either the next or previous blocks 
are unallocated. If either case is true, we pull that 
block and increase the size of the current block by the 
sum of either next + current block or previous + current 
block. In short, we merge our tiny free blocks to one big 
block by checking the allocated bit of adjacent blocks.

For mm_realloc, we have two cases. The first case is 
for when the requested size is smaller than the current 
size of the block. For this case, we check if what would 
be remaining (after splitting the block) is at least as 
big as what was requested, and if that is the case, we 
split. Otherwise, we just return the inputted ptr. This 
strategy helped optimize our code because it made 
splitting dynamic instead of being centered around a 
fixed value. In the other case, we simply call malloc 
and free, relying on free’s embedded coalescing (see 
more below).

Similarly, to achieve high throughput, we implemented 
similar splitting methods in malloc. Moreover, we reduced
 our calls to extend_heap (where we call mem_sbrk) as much 
 as possible. When we do extend the heap, we try to take 
 advantage of the call and extend more space than needed 
 (so that we may split, and have excess free blocks).

No known bugs

When building realloc, we initially wanted to
 check adjacent blocks to see if we can merge with 
 our current block to achieve the user’s desired size 
 field. However, to pass the traces, we decided to call
  malloc based on the inputted size and free the current 
  block that was passed in. WIth this strategy, our scores
   were passing. 
