#include <tchar.h>  

  
/*------------------------------------------------------------------ 
FirstDriveFromMask( unitmask ) 
 
Description 
Finds the first valid drive letter from a mask of drive letters. 
The mask must be in the format bit 0 = A, bit 1 = B, bit 2 = C,  
and so on. A valid drive letter is defined when the  
corresponding bit is set to 1. 
 
Returns the first drive letter that was found. 
--------------------------------------------------------------------*/  

