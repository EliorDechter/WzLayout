#ifndef WZ_LAYOUT
#define WZ_LAYOUT

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#define WZL_LOG(...) (void)0;

#define WZ_ASSERT(x) assert(x)

#define WZ_FLEX_FIT_LOOSE 0
#define WZ_FLEX_FIT_TIGHT 1

#define WZ_LAYOUT_NONE 0
#define WZ_LAYOUT_HORIZONTAL 1
#define WZ_LAYOUT_VERTICAL 2

#define	MAIN_AXIS_SIZE_TYPE_MIN  0
#define	MAIN_AXIS_SIZE_TYPE_MAX  1

#define WZ_UINT_MAX 4294967295

#define WZ_LOG_MESSAGE_MAX_SIZE 128

typedef struct wzl_str
{
	char* str;
	unsigned int len;
} wzl_str;

typedef struct WzlRect
{
	unsigned int x, y, w, h;
} WzlRect;

typedef struct WzWidgetDescriptor
{
	unsigned int constraint_min_w, constraint_min_h, constraint_max_w, constraint_max_h;
	unsigned int layout;
	unsigned int pad_left, pad_right, pad_top, pad_bottom;
	unsigned int gap;
	unsigned int* children;
	unsigned int children_count;
	unsigned int flex_factor;
	wzl_str file;
	unsigned int line;
	unsigned char free_from_parent_horizontally, free_from_parent_vertically;
	unsigned char flex_fit;
	unsigned char main_axis_size_type;
} WzWidgetDescriptor;

typedef struct WzDebugInfo
{
	unsigned int stage;
	unsigned int index;
	unsigned int constraint_min_w, constraint_min_h, constraint_max_w, constraint_max_h;
	unsigned int x, y, w, h;
} WzDebugInfo;

typedef struct WzLogMessage
{
	char str[WZ_LOG_MESSAGE_MAX_SIZE];
} WzLogMessage;

void wz_do_layout(unsigned int index,
	const WzWidgetDescriptor* widgets, WzlRect* rects,
	unsigned int count);

#endif