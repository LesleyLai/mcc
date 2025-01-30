#ifndef MCC_SEMA_H
#define MCC_SEMA_H

// Semantic analysis pass

#include "arena.h"
#include "diagnostic.h"

typedef struct TranslationUnit TranslationUnit;

ErrorsView type_check(TranslationUnit* ast, Arena* permanent_arena);

#endif // MCC_SEMA_H
