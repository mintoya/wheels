Some wheels im reinventing
-
# fptr.h
a pointer + a length, used as both a generic pointer and 
a "slice" in this library
meant to hold on to raw data 
# types.h
 - a bunch of zig style typedefs , things like u8,f64
 - slice and nullable macros for even more zig style types
# myList 
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
  for_each_((var_ v, mList_vla(list)),{
    println("{}", v);
  })
```
# shortList 
 - the other dynamic list
 - stb-style list
 - lower overheaed than list, doesnt remember allocator or width
 - example of macro usage: 
```c
  int* list = msList_init(localArena, int);
  msList_push(localArena,list, 4);
  msList_push(localArena,list, 5);
  msList_push(localArena,list, 6);
  msList_push(localArena,list, 7);
  // null since the list isnt that long
  int *elem = msList_get(list, 10);
```
# hhmap.h
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
 # stringList.h
 - metadata array + char array for array of arbitrary size elements 
 # cSum.h
 basic checksum written for my keyboard project
# vason.h (didnt know vson was taken lol)
 custom config language parser
 it can hypothetically parse a json file 
### basic syntax
```vason
{
    object :
        {key:value, key2:value2,},
    list   :
        {value,value,...},
    both   :
        {key:value,second element, third element},
    string :
        'string'
}
```
- library doesnt reallocate the actual string input 
- / is escape
- strings start and end '\''
    - they dont nescecarily have to though
    - uninterupted lines of characters are grouped
- {} and [] are interchangeable for json compatibility
- not really lua-style, everything is an array, and there are no rules around pair names
 # print.h
 a *lot* of macros that make printing easier ( hopefully )
 generates a hashmap of slices to function pointers before main using constructor attribute
###  types that are handled automatically
 - fptr  : prints as hex bytes
 - isize : wrapper around usize
 - usize : main integer printer
 - f128  : no f64 yet for windows reasons, prints in scientific notation
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
      PUTS("{x:");
      USETYPEPRINTER(usize, in.x); // use already registered printer
      PUTS(",y:");
      USETYPEPRINTER(usize, in.y);
      PUTC('}');
    })
    // now you can call this with
    print("${point}",((point){0,0}));
 ```
# wheels.h
 this library was supposed to contain single header libraries, however,
 some structures depend on eachother, which means the implementation macros
 need to be called in order, this would be really annoying so wheels.h 
 just ensures that all the libraries have the implementatoins called in order
``` c
    #include "wheels/hhmap.h"
    #include "wheels/stringList.h"
    #include "wheels/wheels.h"
    // umap is dependant on both stringlist and my-list
    // stringlist is dependant on my-list 
    // wheels will implement in this order
    //  hhmap -> stringlist  -> my-list 
```

