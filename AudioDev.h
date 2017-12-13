#ifndef AudioDevH
#define AudioDevH

#include <stdio.h>

#include <gtk/gtk.h>
#include <alsa/asoundlib.h>

void populate_card_list(GtkWidget *comboinputdev, GtkWidget *combooutputdev);

#endif
