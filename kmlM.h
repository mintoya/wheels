#pragma once
#ifndef KMLM_H
  #define KMLM_H
  #include "fptr.h"
  #include "omap.h"
  #include "wchar.h"

  #define min(a, b) ((a < b) ? (a) : (b))
  #define max(a, b) ((a > b) ? (a) : (b))

unsigned int kml_indexOf(fptr string, char c);
fptr kml_until(char delim, fptr string);
fptr kml_behind(char delim, fptr string);
fptr kml_inside(char limits[2], fptr string);
fptr kml_around(char limits[2], fptr string);
fptr kml_after(fptr main, um_fp slice);
fptr kml_trimPadding(fptr in);
fptr findIndex(fptr str, unsigned int index);
fptr findKey(fptr str, um_fp key);

  #define fpChar(fptr) ((char *)fptr.ptr)
static OMap *parse(OMap *parent, stringList *lparent, fptr kml);

static stringList *parseList(stringList *lparent, fptr kml) {
  if (!kml.width) {
    if (!(lparent)) {
      return stringList_new(defaultAlloc);
    } else {
      return lparent ? lparent : NULL;
    }
  }
  if (!(lparent)) {
    lparent = stringList_new(defaultAlloc);
  }
  kml = kml_trimPadding(kml);
  fptr val, pval;
  switch (fpChar(kml)[0]) {
    case '[': {
      val = kml_around("[]", kml);
      pval = kml_inside("[]", kml);
      stringList_scoped *newmap = parseList(NULL, pval);
      if (newmap) {
        if (lparent) {
          StringList_appendObj(lparent, OMOJA(newmap, SLIST));
        }
      }
    } break;
    case '{': {
      val = kml_around("{}", kml);
      pval = kml_inside("{}", kml);
      OMap_scoped *newmap = parse(NULL, NULL, pval);
      if (newmap) {
        if (lparent) {
          StringList_appendObj(lparent, OMOJA(newmap, OMAP));
        }
      }
    } break;
    case '"': {
      kml.ptr++;
      kml.width--;
      pval = (fptr){
          .width = kml_indexOf(kml, '"'),
          .ptr = kml.ptr,
      };
      val = pval;
      if (pval.width == kml.width) {
        pval.width--;
        val.width--;
      } else {
        val.width++;
      }
      kml.ptr--;
      kml.width++;
      StringList_appendObj(lparent, OMOJA(&pval, RAW));
    } break;
    default: {
      val = kml_behind(',', kml);
      pval = kml_until(',', kml);
      StringList_appendObj(lparent, OMOJA(&pval, RAW));
    }
  }
  fptr next = kml_after(kml, val);
  next = kml_trimPadding(next);
  if (next.width && fpChar(next)[0] == ',') {
    next.width--;
    next.ptr++;
  }
  next = kml_trimPadding(next);
  return parseList(lparent, next);
}
static OMap *parse(OMap *parent, stringList *lparent, fptr kml) {
  kml = kml_trimPadding(kml);
  if (!kml.width) {
    if (!(parent || lparent)) {
      return OMap_new(defaultAlloc);
    } else {
      return parent ? parent : NULL;
    }
  }
  if (!(parent || lparent)) {
    parent = OMap_new(defaultAlloc);
  }
  if (fpChar(kml)[0] == '{') {
    kml = kml_inside("{}", kml);
  }
  kml = kml_trimPadding(kml);
  if (!kml.width) {
    if (!(parent || lparent)) {
      return OMap_new(defaultAlloc);
    } else {
      return parent ? parent : NULL;
    }
  }

  if (kml_indexOf(kml, ':') == kml.width) {
    return parent ? parent : NULL;
  }
  fptr key = kml_until(':', kml);
  fptr val = kml_after(kml, kml_behind(':', kml)), pval;

  val = kml_trimPadding(val);
  key = kml_trimPadding(key);

  switch (fpChar(val)[0]) {
    case '[': {
      val = kml_around("[]", val);
      pval = kml_inside("[]", val);
      stringList_scoped *newmap = parseList(NULL, pval);
      if (newmap) {
        if (lparent) {
          StringList_appendObj(lparent, OMOJA(newmap, SLIST));
        }
        if (parent) {
          OMap_setObj(parent, kml_trimPadding(key), OMOJA(newmap, SLIST));
        }
      }
    } break;
    case '{': {
      val = kml_around("{}", val);
      pval = kml_inside("{}", val);
      OMap_scoped *newmap = parse(NULL, NULL, pval);
      if (newmap) {
        if (lparent) {
          StringList_appendObj(lparent, OMOJA(newmap, OMAP));
        }
        if (parent) {
          OMap_setObj(parent, kml_trimPadding(key), OMOJA(newmap, OMAP));
        }
      }
    } break;
    case '"': {
      val.ptr++;
      val.width--;
      pval = kml_until('"', val);
      val = pval;
      val.width++;
      OMap_set(parent, kml_trimPadding(key), pval);
    } break;
    default: {
      val = kml_behind(';', val);
      pval = kml_until(';', val);
      OMap_set(parent, kml_trimPadding(key), pval);
    }
  }
  return parse(parent, lparent, kml_after(kml, val));
}

#endif // KMLM_H
#if defined(KMLM_C) || (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #define KMLM_C (1)
#endif
#ifdef KMLM_C

// index after end if c not present
unsigned int kml_indexOf(um_fp string, char c) {
  int i;
  char *ptr = (char *)string.ptr;
  for (i = 0; i < string.width && ptr[i] != c; i++)
    ;
  return i;
}
um_fp kml_inside(char limits[2], um_fp string) {
  if (!string.width)
    return nullUmf;
  char front = limits[0];
  char back = limits[1];

  int in_single = 0;
  int in_double = 0;

  for (int i = 0; i < string.width; i++) {
    char c = ((char *)string.ptr)[i];

    // toggle quote state
    if (c == '"' && !in_single)
      in_double = !in_double;
    else if (c == '\'' && !in_double)
      in_single = !in_single;

    if (!in_single && !in_double && c == front) {
      unsigned int counter = 1;
      for (int ii = 1; ii + i < string.width; ii++) {
        c = ((char *)string.ptr)[i + ii];

        // toggle quote state inside the nested scan
        if (c == '"' && !in_single)
          in_double = !in_double;
        else if (c == '\'' && !in_double)
          in_single = !in_single;

        if (!in_single && !in_double) {
          if (c == front) {
            counter++;
          } else if (c == back) {
            counter--;
          }
        }

        if (!counter)
          return (um_fp){
              .width = (size_t)ii - 1,
              .ptr = ((uint8_t *)string.ptr) + i + 1,
          };
        // if (i + ii == string.width - 1)
        //   return (um_fp){.ptr = ((uint8_t *)string.ptr) + i + 1,
        //                  .width = (size_t)ii - 1};
        if (i + ii == string.width - 1)
          return (um_fp){
              .width = (size_t)ii,
              .ptr = ((uint8_t *)string.ptr) + i + 1,
          };
      }
    }
  }
  return nullUmf;
}

um_fp kml_around(char limits[2], um_fp string) {
  if (!string.width)
    return nullUmf;
  char front = limits[0];
  char back = limits[1];

  int in_single = 0;
  int in_double = 0;

  for (int i = 0; i < string.width; i++) {
    char c = ((char *)string.ptr)[i];
    if (c == '"' && !in_single)
      in_double = !in_double;
    else if (c == '\'' && !in_double)
      in_single = !in_single;
    if (!in_single && !in_double && c == front) {
      unsigned int counter = 1;
      for (int ii = 1; ii + i < string.width; ii++) {
        c = ((char *)string.ptr)[i + ii];

        if (c == '"' && !in_single)
          in_double = !in_double;
        else if (c == '\'' && !in_double)
          in_single = !in_single;

        if (!in_single && !in_double) {
          if (c == front) {
            counter++;
          } else if (c == back) {
            counter--;
          }
        }

        if (!counter)
          return (um_fp){
              .width = (size_t)ii + 1,
              .ptr = ((uint8_t *)string.ptr) + i,
          };
        if (i + ii == string.width - 1)
          return (um_fp){
              .width = (size_t)ii + 1,
              .ptr = ((uint8_t *)string.ptr) + i,
          };
      }
    }
  }
  return nullUmf;
}

um_fp kml_until(char delim, um_fp string) {
  int i = 0;
  char *ptr = (char *)string.ptr;
  while (i < string.width && ptr[i] != delim) {
    i++;
  }
  string.width = i;
  return string;
}
um_fp kml_behind(char delim, um_fp string) {
  int i = 0;
  while (i < string.width && ((char *)string.ptr)[i] != delim) {
    i++;
  }
  string.width = min(i + 1, string.width);
  return string;
}
um_fp kml_after(um_fp main, um_fp slice) {
  if (!(main.ptr && main.width))
    return nullUmf;
  if (!(slice.ptr && slice.width))
    return nullUmf;
  char *mainStart = (char *)main.ptr;
  char *mainEnd = mainStart + main.width;
  char *sliceStart = (char *)slice.ptr;
  char *sliceEnd = sliceStart + slice.width;

  // assert(sliceStart >= mainStart && sliceEnd <= mainEnd);
  if (!(sliceStart >= mainStart && sliceEnd <= mainEnd))
    return nullUmf;

  return (um_fp){
      .width = (size_t)(mainEnd - sliceEnd),
      .ptr = (uint8_t *)sliceEnd,
  };
}
char isSkip(char c) {
  switch (c) {
    case ' ':
    case '\n':
    case '\r':
    case '\0':
    case '\t':
      return 1;
    default:
      return 0;
  }
}
um_fp kml_trimPadding(um_fp in) {
  if (!in.width)
    return nullFptr;
  um_fp res = in;
  unsigned int front = 0;
  unsigned int back = in.width - 1;
  while (front < in.width && isSkip(((char *)in.ptr)[front])) {
    front++;
  }
  while (back > front && isSkip(((char *)in.ptr)[back])) {
    back--;
  }
  fptr *splits = fptr_stack_split(in, front, back + 1);

  res = splits[1];
  return res;
}
  #undef min
  #undef max
#endif
