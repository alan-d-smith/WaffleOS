#pragma once

#include "../ui/ui_widget.h"
#include "../ui/window_manager.h"

void paging_visualiser_callback(UIWidget *widget, Window *parent);
void update_paging_visualiser(void); // Called every frame
void close_paging_visualiser(void);

