#pragma once

#ifndef DISABLE_OPTIMIZATION
#define DISABLE_OPTIMIZATION __pragma(optimize("",off))
#endif 

#ifndef ENABLE_OPTIMIZATION
#define ENABLE_OPTIMIZATION __pragma(optimize("",on))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif