# Some wheels im reinventing

 ## fptr.h
a pointer + a length, used as both a generic pointer and 
a "slice" in this library
meant to hold on to raw data 
 ## types.h
 - a bunch of zig style typedefs , things like u8,f64
 - slice and nullable macros for even more zig style types
 ## my-List 
 - my-list.h is a basic dynamic list implementation
 - very close to [the vec in CC](https://github.com/JacksonAllan/CC) 
 - example of macro usage: 
```c
  mList(int) list = mList_init(localArena, int);
  mList_push(list, 4);
  mList_push(list, 5);
  mList_push(list, 6);
  mList_push(list, 7);
  // null since the list isnt that long
  int *elem = mList_get(list, 10);
  mList_foreach(list, int, v, {
    println("{}", v);
  });
```
 ## hhmap.h
 - hash map with linked-list style collision resolution
 but linked list elements are in a normal list buffer

 - example of macro usage: 
```c
  mHmap(int, int) map = mHmap_init(localArena, int, int);
  mHmap_set(map, 1, 1);
  mHmap_set(map, 2, 4);
  mHmap_set(map, 3, 9);
  mHmap_set(map, 4, 16);
  // null since key doesnt exist
  // also in the map otherwise
  int *six = mHmap_get(map, 6);
  mHmap_foreach(map, int, key, int, val, {
    println("{} -> {}", key, val);
  });
```
 ## stringList.h
 **TODO** 
 - metadata array + char array for array of arbitrary size elements 
 ## cSum.h
 basic checksum written for my keyboard project
 ## vason.h (didnt know vson was taken lol)
 custom config language parser
 tried to make it as flexible as possible, 
 it can hypothetically parse a json file 
### basic syntax
```vason
    object 
        {key:value; key2:value2;}
    list 
        [value,value,...]
    string
        `string'
```
- library doesnt reallocate the actual string input 
- both / and \ are escape characters, highest priority
- strings start with '`' and end with ''', second highest priority
    - they dont nescecarily have to though
    - uninterupted lines of characters are grouped
- {} and [] are interchangeable
- maps are triggered by the : or = characters
 ## print.h
 a *lot* of macros that make printing easier ( hopefully )
 based on u/jacksaccountonreddit's [better c generics](https://github.com/JacksonAllan/CC/blob/main/articles/Better_C_Generics_Part_1_The_Extendible_Generic.md) 
 ```c
    #include "print.h"
    #include "wheels.h"
    // print, println, print_wf, and println_wf macros
    // print_wf takes an outputFunction
    typedef void (*outputFunction)(char *, unsigned int length, char flush);

    typedef struct {
      int x;
      int y;
    } point;
    REGISTER_PRINTER(point, {
      PUTS("{x:", 3);
      USETYPEPRINTER(int, in.x); // use already registered printer
      PUTS(",", 1);
      PUTS("y:", 2);
      USETYPEPRINTER(int, in.y);
      PUTS("}", 1);
    })
    // now you can call this with
    print("${point}",((point){0,0}));

    #include "printer/genericName.h" // advances counter
    MAKE_PRINT_ARG_TYPE(point);
    // will create a typedef in the generic list
    // that matches point
    // now you can call it with
    print("${}",((point){0,0}));
 ```
 ## wheels.h
 this library was supposed to contain single header libraries, however,
 some structures depend on eachother, which means the implementation macros
 need to be called in order, this would be really annoying so wheels.h 
 just ensures that all the libraries have the implementatoins called in order
``` c
    #include "wheels/umap.h"
    #include "wheels/stringList.h"
    #include "wheels/wheels.h"
    // umap is dependant on both stringlist and my-list
    // stringlist is dependant on my-list 
    // wheels will implement in this order
    //  umap -> stringlist  -> my-list 
```

