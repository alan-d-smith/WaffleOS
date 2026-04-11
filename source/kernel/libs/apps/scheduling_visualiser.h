#pragma once

#include "../ui/ui_widget.h"
#include "../ui/window_manager.h"

void scheduling_visualiser_callback(UIWidget *widget, Window *parent);
void update_scheduling_visualiser(void); // called every frame
void close_scheduling_visualiser(void);
