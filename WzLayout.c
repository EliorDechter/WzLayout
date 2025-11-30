
#include "WzLayout.h"

#define wz_log(x) (void)x;

WzWidgetDescriptor wz_widget_descriptor_create(
	unsigned int constraint_min_w, unsigned int constraint_min_h,
	unsigned int constraint_max_w, unsigned int constraint_max_h,
	unsigned int layout,
	unsigned int pad_left, unsigned int pad_right, unsigned int pad_top, unsigned int pad_bottom,
	unsigned int* children,
	unsigned int children_count,
	unsigned char free_from_parent_horizontally, unsigned char free_from_parent_vertically,
	unsigned char flex_fit
)
{

}

void wz_do_layout(unsigned int index,
	const WzWidgetDescriptor* widgets, WzlRect* rects,
	unsigned int count)
{
	unsigned int widgets_stack_count = 0;

	unsigned int size_per_flex_factor;
	WzWidgetDescriptor* parent;
	WzWidgetDescriptor* child;

	unsigned int constraint_max_w, constraint_max_h;

	unsigned int w;
	unsigned int h;
	unsigned int available_size_main_axis, available_size_cross_axis;
	int i;
	unsigned int children_flex_factor;
	unsigned int children_size, max_child_h;
	unsigned int children_h, max_child_w;
	unsigned int parent_index;

	const unsigned int max_depth = 10;
	unsigned int* widgets_visits = calloc(sizeof(*widgets_visits), count);
	unsigned int* widgets_stack = calloc(sizeof(*widgets_stack), max_depth);

	WzLogMessage* log_messages = malloc(sizeof(*log_messages) * count * 10);
	unsigned int log_messages_count = 0;

	widgets_stack[widgets_stack_count++] = index;

	unsigned int* constraint_min_main_axis, * constraint_max_main_axis,
		* constraint_min_cross_axis = 0, * constraint_max_cross_axis = 0,
		* actual_size_main_axis, * actual_size_cross_axis;

	unsigned int* child_constraint_min_main_axis, * child_constraint_max_main_axis,
		* child_constraint_min_cross_axis, * child_constraint_max_cross_axis,
		* child_actual_size_main_axis, * child_cross_axis_actual_size;

	unsigned int screen_size_main_axis, screen_size_cross_axis;

	unsigned int layout_widget_cross_axis_size;

	unsigned int padding_cross_axis;
	WzlRect* child_rect, * parent_rect;
	unsigned int root_w, root_h;

	root_w = widgets[index].constraint_max_w;
	root_h = widgets[index].constraint_max_h;

	rects[index].x = 0;
	rects[index].y = 0;

	// Constraints pass
	while (widgets_stack_count)
	{
		parent_index = widgets_stack[widgets_stack_count - 1];
		parent = &widgets[parent_index];
		parent_rect = &rects[parent_index];

		WZ_ASSERT(parent->constraint_max_w > 0);
		WZ_ASSERT(parent->constraint_max_h > 0);
		WZ_ASSERT(parent_rect->x >= 0);
		WZ_ASSERT(parent_rect->y >= 0);

		if (!parent->children_count)
		{
			// Size leaf widgets, and pop immediately
			// For now all leaf widgets must have a finite constraint
			// Later on we'll let them decide their own size based on their content
			WZ_ASSERT(!((parent->layout == WZ_LAYOUT_HORIZONTAL || parent->layout == WZ_LAYOUT_VERTICAL))
				&& !parent->children_count);

			WZ_ASSERT(parent->constraint_max_w != WZ_UINT_MAX);
			parent_rect->w = parent->constraint_max_w;

			WZ_ASSERT(parent->constraint_max_h != WZ_UINT_MAX);
			parent_rect->h = parent->constraint_max_h;

			WZ_ASSERT(parent_rect->w);
			WZ_ASSERT(parent_rect->h);
			WZ_ASSERT(parent_rect->w <= root_w);
			WZ_ASSERT(parent_rect->h <= root_h);
			WZ_ASSERT(parent_rect->w <= parent->constraint_max_w);
			WZ_ASSERT(parent_rect->h <= parent->constraint_max_h);
			WZ_ASSERT(parent_rect->w <= root_w);
			WZ_ASSERT(parent_rect->h <= root_h);

			wz_log(log_messages, &log_messages_count, "Leaf widget (%u %s %d) with constraints (%u, %u) determined its size (%u, %u)\n",
				parent_index, parent->file, parent->line,
				parent->constraint_max_w, parent->constraint_max_h, parent_rect->w, parent_rect->h);

			widgets_stack_count--;
		}
		else
		{
			// Handle widgets with children
			if (parent->layout == WZ_LAYOUT_HORIZONTAL || parent->layout == WZ_LAYOUT_VERTICAL)
			{
				if (parent->layout == WZ_LAYOUT_HORIZONTAL)
				{
					constraint_min_main_axis = &parent->constraint_min_w;
					constraint_max_main_axis = &parent->constraint_max_w;
					constraint_min_cross_axis = &parent->constraint_min_h;
					constraint_max_cross_axis = &parent->constraint_max_h;
					actual_size_main_axis = &parent_rect->w;
					actual_size_cross_axis = &parent_rect->h;
					screen_size_main_axis = root_w;
					screen_size_cross_axis = root_h;
					padding_cross_axis = parent->pad_top + parent->pad_bottom;
				}
				else if (parent->layout == WZ_LAYOUT_VERTICAL)
				{
					constraint_min_main_axis = &parent->constraint_min_h;
					constraint_max_main_axis = &parent->constraint_max_h;
					constraint_min_cross_axis = &parent->constraint_min_w;
					constraint_max_cross_axis = &parent->constraint_max_w;
					actual_size_main_axis = &parent_rect->h;
					actual_size_cross_axis = &parent_rect->w;
					screen_size_main_axis = root_h;
					screen_size_cross_axis = root_w;
					padding_cross_axis = parent->pad_left + parent->pad_right;
				}
				else
				{
					child_constraint_min_main_axis = child_constraint_max_main_axis =
						child_constraint_max_cross_axis = child_constraint_min_cross_axis = 0;
					constraint_max_main_axis = 0;
					actual_size_main_axis = actual_size_cross_axis = 0;

					WZ_ASSERT(0);
				}

				// You got 3 visits for layout widget.
				// 1. Non Flex children get fixed constraints
				// 2. Above children determine their desired size, and now we allocate available space to flex children
				// 3. It's the turn of the flex children to determine their size, and then we can finally assess the 
				// Layout widget's size using it's children's 
				if (widgets_visits[parent_index] == 0)
				{
					const char* widget_type = 0;
					if (parent->layout == WZ_LAYOUT_HORIZONTAL)
					{
						widget_type = "Row";
					}
					else if (parent->layout == WZ_LAYOUT_VERTICAL)
					{
						widget_type = "Column";
					}

					wz_log(log_messages, &log_messages_count, "%s (%s, %d) with constraints (main %u, cross %u) begins constrains non-flex children\n",
						widget_type, parent->file, parent->line,
						*constraint_max_main_axis, *constraint_max_cross_axis);

					// Give constraints to non flex children
					// A child with flex factor 0 recieves unbounded constraints in the main axis
					for (int i = 0; i < parent->children_count; ++i)
					{
						child = &widgets[parent->children[i]];

						if (child->flex_factor == 0)
						{
							if (parent->layout == WZ_LAYOUT_HORIZONTAL)
							{
								child_constraint_min_main_axis = &child->constraint_min_w;
								child_constraint_max_main_axis = &child->constraint_max_w;
								child_constraint_min_cross_axis = &child->constraint_min_h;
								child_constraint_max_cross_axis = &child->constraint_max_h;
							}
							else if (parent->layout == WZ_LAYOUT_VERTICAL)
							{
								child_constraint_min_main_axis = &child->constraint_min_h;
								child_constraint_max_main_axis = &child->constraint_max_h;
								child_constraint_min_cross_axis = &child->constraint_min_w;
								child_constraint_max_cross_axis = &child->constraint_max_w;
							}
							else
							{
								// Just to appease the compiler
								child_constraint_min_main_axis = child_constraint_max_main_axis =
									child_constraint_min_cross_axis = child_constraint_max_cross_axis = 0;
								WZ_ASSERT(0);
							}

							if (!child->free_from_parent_horizontally)
							{
								// Leave by default all constraints as they are
								// Child will only inherit the cross axis constraint
								WZ_ASSERT(*child_constraint_max_cross_axis);
								if (*constraint_max_cross_axis < *child_constraint_max_cross_axis)
								{
									WZ_ASSERT(*constraint_max_cross_axis > padding_cross_axis);
									*child_constraint_max_cross_axis = *constraint_max_cross_axis - padding_cross_axis;
								}

								WZ_ASSERT(*child_constraint_max_main_axis);
								WZ_ASSERT(*child_constraint_max_cross_axis);

								wz_log(log_messages, &log_messages_count, "Non-flex widget (%s, %d) with constraints (%u, %u)\n", child->file, child->line,
									child->constraint_max_w, child->constraint_max_h);
							}

							widgets_stack[widgets_stack_count] = parent->children[i];
							widgets_stack_count++;
						}
					}
					wz_log(log_messages, &log_messages_count, "%s (%s, %d) ends constrains non-flex children\n", widget_type, parent->file, parent->line);

					widgets_visits[parent_index] = 1;
				}
				else if (widgets_visits[parent_index] == 1)
				{
					// Give constraints to flex children, allocating from the availble space
					if (parent->layout == WZ_LAYOUT_HORIZONTAL)
					{
						available_size_main_axis = parent->constraint_max_w;
						available_size_cross_axis = parent->constraint_max_h;
					}
					else if (parent->layout == WZ_LAYOUT_VERTICAL)
					{
						available_size_main_axis = parent->constraint_max_h;
						available_size_cross_axis = parent->constraint_max_w;
					}

					children_flex_factor = 0;

					for (i = 0; i < parent->children_count; ++i)
					{
						child = &widgets[parent->children[i]];
						child_rect = &rects[parent->children[i]];

						if (child->free_from_parent_horizontally)
						{
							continue;
						}

						if (parent->layout == WZ_LAYOUT_HORIZONTAL)
						{
							child_actual_size_main_axis = &child_rect->w;
						}
						else if (parent->layout == WZ_LAYOUT_VERTICAL)
						{
							child_actual_size_main_axis = &child_rect->h;
						}
						else
						{
							child_actual_size_main_axis = 0;
						}

						if (!child->flex_factor)
						{
							WZ_ASSERT(*child_actual_size_main_axis);
							available_size_main_axis -= *child_actual_size_main_axis;
						}
						else
						{
							children_flex_factor += child->flex_factor;
						}
					}

					// Substract padding and child gap 
					if (parent->layout == WZ_LAYOUT_HORIZONTAL)
					{
						available_size_main_axis -= parent->pad_left + parent->pad_right;
					}
					else if (parent->layout == WZ_LAYOUT_VERTICAL)
					{
						available_size_main_axis -= parent->pad_top + parent->pad_bottom;
					}

					available_size_main_axis -= parent->gap * (parent->children_count - 1);

					if (children_flex_factor)
					{
						size_per_flex_factor = available_size_main_axis / children_flex_factor;
					}

					const char* widget_type = 0;
					if (parent->layout == WZ_LAYOUT_HORIZONTAL)
					{
						widget_type = "Row";
					}
					else if (parent->layout == WZ_LAYOUT_VERTICAL)
					{
						widget_type = "Column";
					}

					wz_log(log_messages, &log_messages_count, "%s (%s, %d) begins constrains flex children\n", widget_type, parent->file, parent->line);

					for (int i = 0; i < parent->children_count; ++i)
					{
						child = &widgets[parent->children[i]];

						if (parent->layout == WZ_LAYOUT_HORIZONTAL)
						{
							child_constraint_min_main_axis = &child->constraint_min_w;
							child_constraint_max_main_axis = &child->constraint_max_w;
							child_constraint_min_cross_axis = &child->constraint_min_h;
							child_constraint_max_cross_axis = &child->constraint_max_h;
						}
						else if (parent->layout == WZ_LAYOUT_VERTICAL)
						{
							child_constraint_min_main_axis = &child->constraint_min_h;
							child_constraint_max_main_axis = &child->constraint_max_h;
							child_constraint_min_cross_axis = &child->constraint_min_w;
							child_constraint_max_cross_axis = &child->constraint_max_w;
						}
						else
						{
							child_constraint_min_main_axis = child_constraint_max_main_axis =
								child_constraint_max_cross_axis = child_constraint_min_cross_axis = 0;
							WZ_ASSERT(0);
						}

						if (child->flex_factor)
						{
							unsigned int main_axis_size = size_per_flex_factor * child->flex_factor;

							if (parent->flex_fit == WZ_FLEX_FIT_TIGHT)
							{
								*child_constraint_min_main_axis = main_axis_size;
							}
							if (main_axis_size < *child_constraint_max_main_axis)
							{
								*child_constraint_max_main_axis = main_axis_size;
							}

							if (*constraint_min_cross_axis < *child_constraint_min_cross_axis)
							{
								*child_constraint_min_cross_axis = *constraint_min_cross_axis;
							}

							if (*constraint_max_cross_axis < *child_constraint_max_cross_axis)
							{
								*child_constraint_max_cross_axis = *constraint_max_cross_axis - padding_cross_axis;
							}

							wz_log(log_messages, &log_messages_count, "Flex widget (%s, %d) recieved constraints (%u, %u) \n", child->file, child->line,
								child->constraint_max_w, child->constraint_max_h);

							widgets_stack[widgets_stack_count] = parent->children[i];
							widgets_stack_count++;
						}
					}

					wz_log(log_messages, &log_messages_count, "%s (%s, %d) ends constrains flex children\n", widget_type, parent->file, parent->line);

					widgets_visits[parent_index] = 2;
				}
				else if (widgets_visits[parent_index] == 2)
				{
					// We finally determined the size of all the children of a widget with a layout
					// Now we determine it's size

					WZ_ASSERT(parent->children_count);

					// Layout widget is unconstrained in the main axis
					// It must shrink-wrap
					WZ_ASSERT(!(parent->layout == WZ_LAYOUT_HORIZONTAL &&
						parent->constraint_max_w == WZ_UINT_MAX &&
						parent->main_axis_size_type == MAIN_AXIS_SIZE_TYPE_MAX));
					WZ_ASSERT(!(parent->layout == WZ_LAYOUT_VERTICAL &&
						parent->constraint_max_h == WZ_UINT_MAX &&
						parent->main_axis_size_type == MAIN_AXIS_SIZE_TYPE_MAX));

					if (*constraint_max_main_axis == WZ_UINT_MAX)
					{
						children_size = 0;
						for (i = 0; i < parent->children_count; ++i)
						{
							child = &widgets[parent->children[i]];
							child_rect = &rects[parent->children[i]];

							if (parent->layout == WZ_LAYOUT_HORIZONTAL)
							{
								child_actual_size_main_axis = &child_rect->w;
							}
							else if (parent->layout == WZ_LAYOUT_VERTICAL)
							{
								child_actual_size_main_axis = &child_rect->h;
							}
							else
							{
								child_actual_size_main_axis = 0;
								WZ_ASSERT(0);
							}

							children_size += *child_actual_size_main_axis;
						}

						*actual_size_main_axis = children_size;
					}
					else
					{
						// Determine size of widget that is constrained in the main axis
						*actual_size_main_axis = *constraint_max_main_axis;
					}

					layout_widget_cross_axis_size = 0;
					for (int i = 0; i < parent->children_count; ++i)
					{
						child = &widgets[parent->children[i]];
						child_rect = &rects[parent->children[i]];

						WZ_ASSERT(child_rect->w <= root_w);
						WZ_ASSERT(child_rect->h <= root_h);

						if (parent->layout == WZ_LAYOUT_HORIZONTAL)
						{
							if (child_rect->h > layout_widget_cross_axis_size)
							{
								layout_widget_cross_axis_size = child_rect->h;
							}
						}
						else if (parent->layout == WZ_LAYOUT_VERTICAL)
						{
							if (child_rect->w > layout_widget_cross_axis_size)
							{
								layout_widget_cross_axis_size = child_rect->w;
							}
						}
					}

					layout_widget_cross_axis_size += parent->pad_top + parent->pad_bottom;

					WZ_ASSERT(layout_widget_cross_axis_size);
					WZ_ASSERT(layout_widget_cross_axis_size <= parent->constraint_max_w);
					*actual_size_cross_axis = layout_widget_cross_axis_size;

					if (parent->layout == WZ_LAYOUT_HORIZONTAL)
					{
						wz_log(log_messages, &log_messages_count, "Row widget (%s, %d) with constraints (%u, %u) determined its size (%u, %u)\n", parent->file, parent->line,
							parent->constraint_max_w, parent->constraint_max_h, parent_rect->w, parent_rect->h);
					}
					else if (parent->layout == WZ_LAYOUT_VERTICAL)
					{
						wz_log(log_messages, &log_messages_count, "Column widget (%s, %d) with constraints (%u, %u) determined its size (%u, %u)\n", parent->file, parent->line,
							parent->constraint_max_w, parent->constraint_max_h, parent_rect->w, parent_rect->h);
					}

					WZ_ASSERT(*actual_size_main_axis <= *constraint_max_main_axis);
					WZ_ASSERT(*actual_size_cross_axis <= *constraint_max_cross_axis);
					WZ_ASSERT(*actual_size_main_axis <= screen_size_main_axis);
					WZ_ASSERT(*actual_size_cross_axis <= screen_size_cross_axis);

					// Give positions to children
					unsigned int offset = 0;
					for (int i = 0; i < parent->children_count; ++i)
					{
						child = &widgets[parent->children[i]];
						child_rect = &rects[parent->children[i]];

						if (child->free_from_parent_horizontally)
						{
							continue;
						}

						if (parent->layout == WZ_LAYOUT_HORIZONTAL)
						{
							actual_size_main_axis = &child_rect->w;
							child_rect->x = offset;
							child_rect->y = 0;
						}
						else if (parent->layout == WZ_LAYOUT_VERTICAL)
						{
							actual_size_main_axis = &child_rect->h;
							child_rect->y = offset;
							child_rect->x = 0;
						}
						else
						{
							WZ_ASSERT(0);
						}

						// Position padding
						child_rect->x += parent->pad_left;
						child_rect->y += parent->pad_top;

						offset += *actual_size_main_axis;
						offset += parent->gap;

						wz_log(log_messages, &log_messages_count, "Child widget (%u %s, %d) will have the raltive position %u %u\n",
							parent->children[i], child->file, child->line,
							child_rect->x, child_rect->y);

						WZ_ASSERT(child_rect->x <= root_w);
						WZ_ASSERT(child_rect->y <= root_h);
					}

					widgets_stack_count--;
				}
			}
			else if (parent->layout == WZ_LAYOUT_NONE)
			{
				if (widgets_visits[parent_index] == 0)
				{
					for (int i = 0; i < parent->children_count; ++i)
					{
						child = &widgets[parent->children[i]];
						child_rect = &rects[parent->children[i]];

						constraint_max_w = parent->constraint_max_w - (parent->pad_left + parent->pad_right);
						constraint_max_h = parent->constraint_max_h - (parent->pad_top + parent->pad_bottom);

						if (constraint_max_w < child->constraint_max_w)
						{
							child->constraint_max_w = constraint_max_h;
						}
						if (constraint_max_h < child->constraint_max_h)
						{
							child->constraint_max_h = constraint_max_h;
						}

						child_rect->x = parent->pad_left;
						child_rect->y = parent->pad_right;

						widgets_stack[widgets_stack_count] = parent->children[i];
						widgets_stack_count++;
						widgets_visits[parent_index] = 1;
						wz_log(log_messages, &log_messages_count, "Non-layout widget (%s, %d) passes to child constraints (%u, %u) and position (%u %u)\n",
							child->file, child->line,
							child->constraint_max_w, child->constraint_max_h, child_rect->x, child_rect->y);
					}
				}
				else if (widgets_visits[parent_index] == 1)
				{

					parent_rect->w = parent->constraint_max_w;
					parent_rect->h = parent->constraint_max_h;

#if 1
					wz_log(log_messages, &log_messages_count,
						"Non-layout widget (%s, %d) with constraints (%u, %u) determined its size (%u, %u)\n",
						parent->file, parent->line,
						parent->constraint_max_w, parent->constraint_max_h, parent_rect->w, parent_rect->h);
#endif

					WZ_ASSERT(parent_rect->w <= parent->constraint_max_w);
					WZ_ASSERT(parent_rect->h <= parent->constraint_max_h);
					WZ_ASSERT(parent_rect->w <= root_w);
					WZ_ASSERT(parent_rect->h <= root_h);

					widgets_stack_count--;
				}
			}
			else
			{
				WZ_ASSERT(0);
			}
		}
	}

	// Final stage. Calculate the widgets non-relative final position 
	for (int i = 1; i < count; ++i)
	{
		parent = &widgets[i];
		parent_rect = &rects[i];

		for (int j = 0; j < parent->children_count; ++j)
		{
			child = &widgets[parent->children[j]];
			child_rect = &rects[parent->children[j]];

			child_rect->x = parent_rect->x + child_rect->x;
			child_rect->y = parent_rect->y + child_rect->y;

			// Check the widgets size doesnt exceeds its parents
			WZ_ASSERT(child_rect->x >= 0);
			WZ_ASSERT(child_rect->y >= 0);
			WZ_ASSERT(child_rect->x <= root_w);
			WZ_ASSERT(child_rect->y <= root_h);
			WZ_ASSERT(child_rect->y >= 0);
			WZ_ASSERT(child_rect->w);
			WZ_ASSERT(child_rect->h);
			WZ_ASSERT(child_rect->w);
			WZ_ASSERT(child_rect->h);
			WZ_ASSERT(child_rect->x + child_rect->w <= parent_rect->x + parent_rect->w);
			WZ_ASSERT(child_rect->y + child_rect->h <= parent_rect->y + parent_rect->h);
		}
	}

	wz_log(log_messages, &log_messages_count, "---------------------------\n");
	wz_log(log_messages, &log_messages_count, "Final Layout:\n");
	for (int i = 0; i < count; ++i)
	{
		parent = &widgets[i];
		parent_rect = &rects[i];

		wz_log(log_messages, &log_messages_count, "(%s %u) : (%u %u %u %u)\n",
			parent->file, parent->line, parent_rect->x, parent_rect->y, parent_rect->w, parent_rect->h);
	}
	wz_log(log_messages, &log_messages_count, "---------------------------\n");

	free(widgets_visits);
	free(widgets_stack);
	free(log_messages);
}
