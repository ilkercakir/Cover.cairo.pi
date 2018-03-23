/*
 * VideoPlayerWidgets.c
 * 
 * Copyright 2017  <pi@raspberrypi>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#include "VideoPlayerWidgets.h"

char* strreplace(char *src, char *search, char *replace)
{
	char *p;
	char *front;
	char *dest;

	dest = (char*)malloc(1);
	dest[0] = '\0';
	for(front = p = src; (p = strstr(p, search)); front = p)
	{
		p[0] = '\0';
		//printf("%s\n", front);
		dest = (char*)realloc(dest, strlen(dest)+strlen(front)+1);
		strcat(dest, front);

		dest = (char*)realloc(dest, strlen(dest)+strlen(replace)+1);
		strcat(dest, replace);

		p += strlen(search);
	}
	//printf("%s\n", front);
	dest = (char*)realloc(dest, strlen(dest)+strlen(front)+1);
	strcat(dest, front);
	
	//printf("\n%s\n", dest);
	return dest;
}

char* strlastpart(char *src, char *search, int lowerupper)
{
	char *p;
	char *q;
	int i;

	q = &src[strlen(src)];
	for(p = src; (p = strstr(p, search)); p += strlen(search))
	{
		q = p;
	}
	switch (lowerupper)
	{
		case 0:
			break;
		case 1:
			for(i=0;q[i];i++) q[i] = tolower(q[i]);
			break;
		case 2:
			for(i=0;q[i];i++) q[i] = toupper(q[i]);
			break;
	}
	
	return q;
}

static int nomediafile(char *filepath)
{
	return
	(
		strcmp(strlastpart(filepath, ".", 1), ".mp3") &&
		strcmp(strlastpart(filepath, ".", 1), ".m4a") &&
		strcmp(strlastpart(filepath, ".", 1), ".wav") &&

		strcmp(strlastpart(filepath, ".", 1), ".mp4") &&
		strcmp(strlastpart(filepath, ".", 1), ".mov") &&
		strcmp(strlastpart(filepath, ".", 1), ".mkv") &&
		strcmp(strlastpart(filepath, ".", 1), ".webm")
	);
}

void init_playlistparams(playlistparams *plparams, vpwidgets *vpw, microphone *mic, speaker *spk, int vqMaxLength, int aqMaxLength, int spk_samplingrate, int thread_count)
{
	plparams->vpw = vpw;
	plparams->mic = mic;
	plparams->spk = spk;
	plparams->vqMaxLength = vqMaxLength;
	plparams->aqMaxLength = aqMaxLength;
	plparams->thread_count = thread_count;
	plparams->spk_samplingrate = spk_samplingrate;
}

void close_playlistparams(playlistparams *plparams)
{
}

gboolean focus_iter_idle(gpointer data)
{
	GtkTreeModel *model;
	GtkTreePath *tp;
	vpwidgets *vpw = (vpwidgets *)data;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(vpw->listview));
	tp = gtk_tree_model_get_path(model, &(vpw->iter));
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(vpw->listview), tp, NULL, FALSE);
	gtk_tree_path_free(tp);

	return(FALSE);
}

gboolean play_next(vpwidgets *vpw)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(vpw->listview));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(vpw->listview));

	gtk_tree_selection_get_selected(selection, &model, &(vpw->iter));
	if (gtk_tree_model_iter_next(model, &(vpw->iter)))
	{
		//gtk_tree_selection_select_iter(selection, &(vpw->iter));
		gdk_threads_add_idle(focus_iter_idle, (void*)vpw);
	}
	else
	{
		if (gtk_tree_model_iter_nth_child(model, &(vpw->iter), NULL, 0))
		{
			//gtk_tree_selection_select_iter(selection, &(vpw->iter));
			gdk_threads_add_idle(focus_iter_idle, (void*)vpw);
		}
		else
		{
			printf("no entries\n");
			return FALSE;
		}
	}

	if (vpw->vp.now_playing)
	{
		g_free(vpw->vp.now_playing);
		vpw->vp.now_playing = NULL;
	}
	gtk_tree_model_get(model, &(vpw->iter), COL_FILEPATH, &(vpw->vp.now_playing), -1);
	//g_print("Next %s\n", now_playing);

	return TRUE;
}

gboolean play_prev(vpwidgets *vpw)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(vpw->listview));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(vpw->listview));

	gtk_tree_selection_get_selected(selection, &model, &(vpw->iter));
	if (gtk_tree_model_iter_previous(model, &(vpw->iter)))
	{
		//gtk_tree_selection_select_iter(selection, &(vpw->iter));
		gdk_threads_add_idle(focus_iter_idle, (void*)vpw);
	}
	else
	{
		gint nodecount = gtk_tree_model_iter_n_children(model, NULL);
		if (nodecount)
		{
			if (gtk_tree_model_iter_nth_child(model, &(vpw->iter), NULL, nodecount-1))
			{
				//gtk_tree_selection_select_iter(selection, &(vpw->iter));
				gdk_threads_add_idle(focus_iter_idle, (void*)vpw);
			}
			else
			{
				printf("no entries\n");
				return FALSE;
			}
		}
	}

	if (vpw->vp.now_playing)
	{
		g_free(vpw->vp.now_playing);
		vpw->vp.now_playing = NULL;
	}
	gtk_tree_model_get(model, &(vpw->iter), COL_FILEPATH, &(vpw->vp.now_playing), -1);
	//g_print("Next %s\n", now_playing);

	return TRUE;
}

gboolean set_upper(gpointer data)
{
	vpwidgets *vpw = (vpwidgets *)data;
	videoplayer *v = &(vpw->vp);

	gtk_adjustment_set_value(vpw->hadjustment, 0);
	if (v->videoduration)
		gtk_adjustment_set_upper(vpw->hadjustment, v->videoduration);
	else
		gtk_adjustment_set_upper(vpw->hadjustment, v->audioduration);
//printf("%f, %f, %f\n", gtk_adjustment_get_value(vpw->hadjustment), gtk_adjustment_get_lower(vpw->hadjustment), gtk_adjustment_get_upper(vpw->hadjustment));
	return FALSE;
}

gboolean statusbar_msg(gpointer data)
{
	vpwidgets *vpw = (vpwidgets *)data;
	videoplayer *v = &(vpw->vp);

	char msgtext[256];
	char *txt;
	int pos;
	gtk_statusbar_pop(GTK_STATUSBAR(vpw->statusbar), vpw->context_id);
	if (v->videoduration)
	{
		txt = strlastpart(v->now_playing, "/", 0);
		pos = strlen(txt);
		strcpy(msgtext, txt);
		if (pos>70)
		{
			msgtext[30] = '\0';
			strcat(msgtext, "...");
			pos -= 30;
			strcat(msgtext, txt+pos);
		}
		sprintf(vpw->msg, "%2.2f fps, %d*%d, %s (%02i:%02i)", v->frame_rate, v->pCodecCtx->width, v->pCodecCtx->height, msgtext, ((int)(v->videoduration/v->frame_rate))/60, ((int)(v->videoduration/v->frame_rate)%60));
	}
	else
	{
		txt = strlastpart(v->now_playing, "/", 0);
		pos = strlen(txt);
		strcpy(msgtext, txt);
		if (pos>100)
		{
			msgtext[45] = '\0';
			strcat(msgtext, "...");
			pos -= 45;
			strcat(msgtext, txt+pos);
		}
		sprintf(vpw->msg, "%s (%02i:%02i)", msgtext, ((int)(v->audioduration/v->sample_rate))/60, ((int)(v->audioduration/v->sample_rate)%60));
	}
//printf("%s\n", msg);
	gchar *buff = g_strdup_printf("%s", vpw->msg);
	gtk_statusbar_push(GTK_STATUSBAR(vpw->statusbar), vpw->context_id, buff);
	g_free(buff);

	return FALSE;
}

int create_thread0_videoplayer(vpwidgets *vpw, int vqMaxLength, int aqMaxLength, int thread_count)
{
	videoplayer *v = &(vpw->vp);
	v->aeq = &(vpw->ae);

	int err;

	init_videoplayer(v, vpw->playerWidth, vpw->playerHeight, vqMaxLength, aqMaxLength, vpw->ax, thread_count);

	gint pageno;
	g_object_get((gpointer)vpw->notebook, "page", &pageno, NULL);
//printf("Selected id %d\n", pageno);
	v->decodevideo = !pageno;

	if ((err=open_now_playing(v))<0)
	{
		printf("open_now_playing() error %d\n", err);
		return(err);
	}

	err = pthread_create(&(v->tid[0]), NULL, &thread0_videoplayer, (void *)v);
	if (err)
	{}
/*
//printf("thread0_videoplayer->%d\n", 0);
	if ((err=pthread_setaffinity_np(v->tid[0], sizeof(cpu_set_t), &(v->cpu[0]))))
		printf("pthread_setaffinity_np error %d\n", err);
*/

	gdk_threads_add_idle(statusbar_msg, (void*)vpw);

	int i;
	if ((i=pthread_join(v->tid[0], NULL)))
		printf("pthread_join error, v->tid[0], %d\n", i);

	return(v->stoprequested);
}

static gpointer playlist_thread(gpointer args)
{
	int ctype = PTHREAD_CANCEL_ASYNCHRONOUS;
	int ctype_old;
	pthread_setcanceltype(ctype, &ctype_old);

	playlistparams *plp = (playlistparams*)args;

	while(1)
	{
		if (create_thread0_videoplayer(plp->vpw, plp->vqMaxLength, plp->aqMaxLength, plp->thread_count))
			break;
		if (!play_next(plp->vpw))
			break;
	}

//printf("exiting playlist_thread\n");
	plp->vpw->retval0 = 0;
	pthread_exit(&(plp->vpw->retval0));
}

int create_playlist_thread(playlistparams *plp)
{
	int err;

	err = pthread_create(&(plp->vpw->tid), NULL, &playlist_thread, (void *)plp);
	if (err)
	{}
/*
//printf("playlist_thread->%d\n", 0);
	if ((err=pthread_setaffinity_np(plp->vpw->tid, sizeof(cpu_set_t), &(plp->vpw->cpu[0]))))
		printf("pthread_setaffinity_np error %d\n", err);
*/
	return(0);
}

int select_callback(void *data, int argc, char **argv, char **azColName) 
{
	vpwidgets *vpw = (vpwidgets*)data;
//    NotUsed = 0;
//    for (int i = 0; i < argc; i++)
//    {
//     printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
//     if (!strcmp(azColName[i],"response"))
//     {
//      gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combobox), argv[i], argv[i]);
//     }
//    }
//    printf("\n");
	//gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combobox), argv[0], argv[1]);

	gtk_list_store_append(vpw->store, &(vpw->iter));
	gtk_list_store_set(vpw->store, &(vpw->iter), COL_ID, atoi(argv[0]), COL_FILEPATH, argv[1], -1);

//g_print("%s, %s\n", argv[0], argv[1]);
	return 0;
}

static GtkTreeModel* create_and_fill_model(vpwidgets *vpw, int mode, int argc, char** argv)
{
	sqlite3 *db;
	char *err_msg = NULL;
	char *sql = NULL;
	int rc;

	vpw->store = gtk_list_store_new(NUM_COLS, G_TYPE_UINT, G_TYPE_STRING);

	switch(mode)
	{
		case 0:
			break;
		case 1:
			if((rc = sqlite3_open("/var/sqlite3DATA/mediaplayer.db", &db)))
			{
				printf("Can't open database: %s\n", sqlite3_errmsg(db));
			}
			else
			{
//printf("Opened database successfully\n");
				sql = "SELECT * FROM mediafiles order by id;";
				if((rc = sqlite3_exec(db, sql, select_callback, (void*)vpw, &err_msg)) != SQLITE_OK)
				{
					printf("Failed to select data, %s\n", err_msg);
					sqlite3_free(err_msg);
				}
				else
				{
// success
				}
			}
			sqlite3_close(db);
			break;
		case 2:
			for(rc=1;rc<argc;rc++)
			{
				gtk_list_store_append(vpw->store, &(vpw->iter));
				gtk_list_store_set(vpw->store, &(vpw->iter), COL_ID, rc-1, COL_FILEPATH, argv[rc], -1);
			}
		default:
			break;
	}

	return GTK_TREE_MODEL(vpw->store);
}

gboolean search_equal_func(GtkTreeModel *model, gint column, const gchar *key, GtkTreeIter *iter, gpointer search_data)
{
	gboolean valid;
	gchararray gca;
	vpwidgets *vpw = (vpwidgets *)search_data;

	//printf("column %d, key %s\n", column, key);
    for(valid=gtk_tree_model_get_iter_first(model, &(vpw->iter));valid;valid=gtk_tree_model_iter_next(model, &(vpw->iter)))
	{
		gtk_tree_model_get(model, &(vpw->iter), COL_FILEPATH, &gca, -1);
		if (strstr((char*)gca, (char*)key))
		{
//printf("%s, %s\n", gca, key);
			gdk_threads_add_idle(focus_iter_idle, (void*)vpw);
			break;
		}
		g_free(gca);
    }

	return(FALSE); // found
}

void search_destroy(gpointer data)
{
	//g_free(data);
}

static GtkWidget* create_view_and_model(vpwidgets *vpw, int argc, char** argv)
{
	GtkCellRenderer *renderer;
	GtkTreeModel *model;
	GtkWidget *view;

	view = gtk_tree_view_new();

	// Column 1
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "ID", renderer, "text", COL_ID, NULL);

	// Column 2
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW(view), -1, "File Path", renderer, "text", COL_FILEPATH, NULL);

	if (argc>1)
		model = create_and_fill_model(vpw, 2, argc, argv); // insert rows from command line
	else
		model = create_and_fill_model(vpw, 0, 0, NULL); // do not insert rows

	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);

	gtk_tree_view_set_enable_search (GTK_TREE_VIEW(view), TRUE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(view), COL_FILEPATH);
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(view), search_equal_func, (void*)vpw, search_destroy);

/* The tree view has acquired its own reference to the model, so we can drop ours. That way the model will
   be freed automatically when the tree view is destroyed */
//g_object_unref(model);

	return view;
}

static void button1_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;
	videoplayer *vpp = &(vpw->vp);
	int ret;

//g_print("Button 1 clicked\n");
	if (!(vpp->now_playing))
		return;

	if((ret=create_playlist_thread(plp))<0)
	{
		printf("create_playlist_thread error %d\n", ret);
		return;
	}

	gtk_widget_set_sensitive(vpw->button1, FALSE);
	gtk_widget_set_sensitive(vpw->button2, TRUE);
	gtk_widget_set_sensitive(vpw->button8, TRUE);
	gtk_widget_set_sensitive(vpw->button9, TRUE);
	gtk_widget_set_sensitive(vpw->button10, TRUE);

}

static void button2_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;
	videoplayer *vp = &(vpw->vp);

//g_print("Button 2 clicked\n");
	request_stop_frame_reader(vp);

	int i;
	if ((i=pthread_join(vpw->tid, NULL)))
		printf("pthread_join error, vpw->tid, %d\n", i);

	gtk_widget_set_sensitive(vpw->button2, FALSE);
	gtk_widget_set_sensitive(vpw->button8, FALSE);
	gtk_widget_set_sensitive(vpw->button9, FALSE);
	gtk_widget_set_sensitive(vpw->button10, FALSE);
	gtk_widget_set_sensitive(vpw->button1, TRUE);
}

/*
static void buttonParameters_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;

	if (gtk_widget_get_visible(vpw->windowparm))
		gtk_widget_hide(vpw->windowparm);
	else
		gtk_widget_show_all(vpw->windowparm);
}
*/

/*
void buttonTest_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;
	videoplayer *v = &(vpw->vp);

	oglidle *oi = (oglidle*)&(v->oi);
	oglstate *ogl = (oglstate*)&(oi->ogl);

	int imgsize = ogl->linewidth * ogl->height * 4; // w*h RGBA
	char* buf = malloc(imgsize); 

//printf("reading w*h %d %d\n", ogl->linewidth, ogl->height);
	FILE *f = fopen("x.data", "rb");
	fread(buf, 4, ogl->linewidth*ogl->height, f);
	fclose(f);

	oglidle_thread_tex2d_ogl(oi, buf);
	oglidle_thread_draw_widget(oi);
	oglidle_thread_readpixels_ogl(oi);

	GdkWindow *dawin = gtk_widget_get_window(v->drawingarea);
	cairo_region_t *region = gdk_window_get_clip_region(dawin);
	gdk_window_invalidate_region (dawin, region, TRUE);
	//gdk_window_process_updates (dawin, TRUE);
	cairo_region_destroy(region);

	free(buf);
}
*/

static void button3_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;

	GtkTreeModel *model;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(vpw->listview));
	gtk_list_store_clear((GtkListStore*)model);

	gtk_tree_view_set_model(GTK_TREE_VIEW(vpw->listview), NULL); /* Detach model from view */
	g_object_unref(model);
	model = create_and_fill_model(vpw, 0, 0, NULL); // insert rows
	gtk_tree_view_set_model(GTK_TREE_VIEW(vpw->listview), model); /* Re-attach model to view */

	gtk_tree_view_set_enable_search (GTK_TREE_VIEW(vpw->listview), TRUE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(vpw->listview), COL_FILEPATH);
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(vpw->listview), search_equal_func, (void*)vpw, search_destroy);
}

static void button4_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;

	GtkTreeModel *model;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(vpw->listview));
	gtk_list_store_clear((GtkListStore*)model);

	gtk_tree_view_set_model(GTK_TREE_VIEW(vpw->listview), NULL); /* Detach model from view */
	g_object_unref(model);
	model = create_and_fill_model(vpw, 1, 0, NULL); // insert rows
	gtk_tree_view_set_model(GTK_TREE_VIEW(vpw->listview), model); /* Re-attach model to view */

	gtk_tree_view_set_enable_search (GTK_TREE_VIEW(vpw->listview), TRUE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(vpw->listview), COL_FILEPATH);
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(vpw->listview), search_equal_func, (void*)vpw, search_destroy);
}

void listdir(const char *name, sqlite3 *db, int *id)
{
	char *err_msg = NULL;
	char sql[1024];
	int rc;
	char sid[10];
	char path[1024];
	char *strname;
	char *strdest;

	DIR *dir;
	struct dirent *entry;

	if (!(dir = opendir(name)))
		return;
	if (!(entry = readdir(dir)))
		return;

	do
	{
		if (entry->d_type == DT_DIR)
		{
			int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
			path[len] = 0;
			if ((!strcmp(entry->d_name, ".")) || (!strcmp(entry->d_name, "..")))
				continue;
//printf("%*s[%s]\n", level*2, "", entry->d_name);
			listdir(path, db, id);
		}
		else
		{
//printf("%*s- %s/%s\n", level*2, "", name, entry->d_name);
			if (nomediafile(entry->d_name))
				continue;

			(*id)++;
			sprintf(sid, "%d", *id);
			sql[0] = '\0';
			strcat(sql, "INSERT INTO mediafiles VALUES(");
			strcat(sql, sid);
			strcat(sql, ", '");
			strcat(sql, name);
			strcat(sql, "/");
				//strcat(sql, entry->d_name);
				strname=(char*)malloc(sizeof(entry->d_name)+1);
				strcpy(strname, entry->d_name);
				strdest = strreplace(strname, "'", "''");
				free(strname);
				strcat(sql, strdest);
				free(strdest);
			strcat(sql, "');");
//printf("%s\n", sql);
			if((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK)
			{
				printf("Failed to insert data, %s\n", err_msg);
				sqlite3_free(err_msg);
			}
			else
			{
// success
			}
		}
	}
	while((entry = readdir(dir)));
	closedir(dir);
}

static void button6_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;
	videoplayer *vpp = &(vpw->vp);

	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;

	char *err_msg = NULL;
	sqlite3 *db;
	char *sql;
	int rc;
	int id;

	if (!vpp->catalog_folder)
	{
		g_free(vpp->catalog_folder);
		vpp->catalog_folder = NULL;
	}

	dialog = gtk_file_chooser_dialog_new("Open Folder for Catalog", GTK_WINDOW(vpw->vpwindow), action, "Cancel", GTK_RESPONSE_CANCEL, "Select Folder", GTK_RESPONSE_ACCEPT, NULL);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		vpp->catalog_folder = gtk_file_chooser_get_filename(chooser);

		if ((rc = sqlite3_open("/var/sqlite3DATA/mediaplayer.db", &db)))
		{
			printf("Can't open database: %s\n", sqlite3_errmsg(db));
		}
		else
		{
//printf("Opened database successfully\n");
			sql = "DELETE FROM mediafiles;";
			if ((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK)
			{
				printf("Failed to delete data, %s\n", err_msg);
				sqlite3_free(err_msg);
			}
			else
			{
// success
			}
			id = 0;
			listdir(vpp->catalog_folder, db, &id);
		}
		sqlite3_close(db);

	}
	gtk_widget_destroy (dialog);
}

int select_add_callback(void *data, int argc, char **argv, char **azColName) 
{
	vpwidgets *vpw = (vpwidgets*)data;

	vpw->last_id = atoi(argv[0]);
	return 0;
}

int select_add_lastid(vpwidgets *vpw)
{
	sqlite3 *db;
	char *err_msg = NULL;
	char *sql = NULL;
	int rc;

	vpw->last_id = 0;
	if((rc = sqlite3_open("/var/sqlite3DATA/mediaplayer.db", &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");
		sql = "SELECT max(id) as id FROM mediafiles;";
		if((rc = sqlite3_exec(db, sql, select_add_callback, (void*)vpw, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to select data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}
	}
	sqlite3_close(db);

	return(vpw->last_id);
}

static void button7_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;

	char *err_msg = NULL;
	sqlite3 *db;
	int rc;
	int id;
	char sql[1024];
	char sid[10];
	char *strname;
	char *strdest;

	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	GSList *chosenfile;

	dialog = gtk_file_chooser_dialog_new("Add File", GTK_WINDOW(vpw->vpwindow), action, "Cancel", GTK_RESPONSE_CANCEL, "Open", GTK_RESPONSE_ACCEPT, NULL);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	gtk_file_chooser_set_select_multiple(chooser, TRUE);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		if ((rc = sqlite3_open("/var/sqlite3DATA/mediaplayer.db", &db)))
		{
			printf("Can't open database: %s\n", sqlite3_errmsg(db));
		}
		else
		{
			id = select_add_lastid(vpw);
			GSList *filelist = gtk_file_chooser_get_filenames(chooser);
			for(chosenfile=filelist;chosenfile;chosenfile=chosenfile->next)
			{
//printf("%s\n", (char*)chosenfile->data);
				if (nomediafile((char*)chosenfile->data))
					continue;

				id++;
				sprintf(sid, "%d", id);
				sql[0] = '\0';
				strcat(sql, "INSERT INTO mediafiles VALUES(");
				strcat(sql, sid);
				strcat(sql, ", '");
					//strcat(sql, (char*)chosenfile->data));
					strname=(char*)malloc(1024);
					strcpy(strname, (char*)chosenfile->data);
					strdest = strreplace(strname, "'", "''");
					free(strname);
					strcat(sql, strdest);
					free(strdest);
				strcat(sql, "');");
//printf("%s\n", sql);
				if ((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK)
				{
					printf("Failed to insert data, %s\n", err_msg);
					sqlite3_free(err_msg);
				}
				else
				{
// success
				}
			}
			button3_clicked(vpw->button3, plp);
			button4_clicked(vpw->button4, plp);
		}
		sqlite3_close(db);
	}
	gtk_widget_destroy(dialog);
}

static void button8_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;
//g_print("Button 8 clicked\n");
	button2_clicked(vpw->button2, data);
	if (play_next(vpw))
	{
		button1_clicked(vpw->button1, data);
		gdk_threads_add_idle(focus_iter_idle, (void*)vpw);
	}
}

static void button9_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;
//g_print("Button 9 clicked\n");
	button2_clicked(vpw->button2, data);
	if (play_prev(vpw))
	{
		button1_clicked(vpw->button1, data);
		gdk_threads_add_idle(focus_iter_idle, (void*)vpw);
	}
}

static void button10_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;
	videoplayer *vpp = &(vpw->vp);
//g_print("Button 10 clicked\n");
	if (vpp->vpq.playerstatus==PLAYING)
	{
		pthread_mutex_lock(&(vpp->seekmutex));
		vpp->vpq.playerstatus = PAUSED;
		pthread_mutex_unlock(&(vpp->seekmutex));

		gtk_button_set_label(GTK_BUTTON(vpw->button10), "Resume");
		gtk_widget_set_sensitive(vpw->button2, FALSE);
		gtk_widget_set_sensitive(vpw->button8, FALSE);
		gtk_widget_set_sensitive(vpw->button9, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive(vpw->button2, TRUE);
		gtk_widget_set_sensitive(vpw->button8, TRUE);
		gtk_widget_set_sensitive(vpw->button9, TRUE);
		gtk_button_set_label(GTK_BUTTON(vpw->button10), "Pause");

		pthread_mutex_lock(&(vpp->seekmutex));
		vpp->vpq.playerstatus = PLAYING;
		pthread_cond_signal(&(vpp->pausecond)); // Should wake up *one* thread
		pthread_mutex_unlock(&(vpp->seekmutex));
	}
}

static void button11_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;

	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	GSList *chosenfile;

	int i = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(vpw->store), NULL); // rows

	dialog = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(vpw->vpwindow), action, "Cancel", GTK_RESPONSE_CANCEL, "Open", GTK_RESPONSE_ACCEPT, NULL);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	gtk_file_chooser_set_select_multiple(chooser, TRUE);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		//button3_clicked(vpw->button3, plp); // Clear list
		GSList *filelist = gtk_file_chooser_get_filenames(chooser);
		for(chosenfile=filelist;chosenfile;chosenfile=chosenfile->next)
		{
			if (nomediafile((char*)chosenfile->data))
				continue;
//printf("%s\n", (char*)chosenfile->data);
			gtk_list_store_append(vpw->store, &(vpw->iter));
			gtk_list_store_set(vpw->store, &(vpw->iter), COL_ID, i++, COL_FILEPATH, chosenfile->data, -1);
		}
	}
	gtk_widget_destroy(dialog);
}

static void vscale0(GtkWidget *widget, gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;

	float value = 0.0 - gtk_range_get_value(GTK_RANGE(widget));
//    printf("Adjustment value: %f\n", value);
	AudioEqualizer_setGain(&(vpw->ae), 0, value);
	if (vpw->ae.autoleveling)
		gtk_range_set_value(GTK_RANGE(vpw->vscaleeqA), 4.0-vpw->ae.effgain);
}

static void vscale1(GtkWidget *widget, gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;

	float value = 0.0 - gtk_range_get_value(GTK_RANGE(widget));
//    printf("Adjustment value: %f\n", value);
	AudioEqualizer_setGain(&(vpw->ae), 1, value);
	if (vpw->ae.autoleveling)
		gtk_range_set_value(GTK_RANGE(vpw->vscaleeqA), 4.0-vpw->ae.effgain);
}

static void vscale2(GtkWidget *widget, gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;

	float value = 0.0 - gtk_range_get_value(GTK_RANGE(widget));
//    printf("Adjustment value: %f\n", value);
	AudioEqualizer_setGain(&(vpw->ae), 2, value);
	if (vpw->ae.autoleveling)
		gtk_range_set_value(GTK_RANGE(vpw->vscaleeqA), 4.0-vpw->ae.effgain);
}

static void vscale3(GtkWidget *widget, gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;

	float value = 0.0 - gtk_range_get_value(GTK_RANGE(widget));
//    printf("Adjustment value: %f\n", value);
	AudioEqualizer_setGain(&(vpw->ae), 3, value);
	if (vpw->ae.autoleveling)
		gtk_range_set_value(GTK_RANGE(vpw->vscaleeqA), 4.0-vpw->ae.effgain);
}

static void vscale4(GtkWidget *widget, gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;

	float value = 0.0 - gtk_range_get_value(GTK_RANGE(widget));
//    printf("Adjustment value: %f\n", value);
	AudioEqualizer_setGain(&(vpw->ae), 4, value);
	if (vpw->ae.autoleveling)
		gtk_range_set_value(GTK_RANGE(vpw->vscaleeqA), 4.0-vpw->ae.effgain);
}

static void vscale5(GtkWidget *widget, gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;

	float value = 0.0 - gtk_range_get_value(GTK_RANGE(widget));
//    printf("Adjustment value: %f\n", value);
	AudioEqualizer_setGain(&(vpw->ae), 5, value);
	if (vpw->ae.autoleveling)
		gtk_range_set_value(GTK_RANGE(vpw->vscaleeqA), 4.0-vpw->ae.effgain);
}

static void vscale6(GtkWidget *widget, gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;

	float value = 0.0 - gtk_range_get_value(GTK_RANGE(widget));
//    printf("Adjustment value: %f\n", value);
	AudioEqualizer_setGain(&(vpw->ae), 6, value);
	if (vpw->ae.autoleveling)
		gtk_range_set_value(GTK_RANGE(vpw->vscaleeqA), 4.0-vpw->ae.effgain);
}

static void vscale7(GtkWidget *widget, gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;

	float value = 0.0 - gtk_range_get_value(GTK_RANGE(widget));
//    printf("Adjustment value: %f\n", value);
	AudioEqualizer_setGain(&(vpw->ae), 7, value);
	if (vpw->ae.autoleveling)
		gtk_range_set_value(GTK_RANGE(vpw->vscaleeqA), 4.0-vpw->ae.effgain);
}

static void vscale8(GtkWidget *widget, gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;

	float value = 0.0 - gtk_range_get_value(GTK_RANGE(widget));
//    printf("Adjustment value: %f\n", value);
	AudioEqualizer_setGain(&(vpw->ae), 8, value);
	if (vpw->ae.autoleveling)
		gtk_range_set_value(GTK_RANGE(vpw->vscaleeqA), 4.0-vpw->ae.effgain);
}

static void vscale9(GtkWidget *widget, gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;

	float value = 0.0 - gtk_range_get_value(GTK_RANGE(widget));
//    printf("Adjustment value: %f\n", value);
	AudioEqualizer_setGain(&(vpw->ae), 9, value);
	if (vpw->ae.autoleveling)
		gtk_range_set_value(GTK_RANGE(vpw->vscaleeqA), 4.0-vpw->ae.effgain);
}

static void vscaleA(GtkWidget *widget, gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;

	float value = 4.0 - gtk_range_get_value(GTK_RANGE(widget));
//	printf("Adjustment value: %f\n", value);
	AudioEqualizer_setEffectiveGain(&(vpw->ae), value);
}

static void vscaleV(GtkWidget *widget, gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;

	float value = 1.0 - gtk_range_get_value(GTK_RANGE(widget));
//	printf("Adjustment value: %f\n", value);
	AudioEqualizer_setVolume(&(vpw->ae), value);
}

gchar* scale_valueformat(GtkScale *scale, gdouble value, gpointer user_data)
{
	return g_strdup_printf("%0.1f", -value);
}

gchar* scale_valuepreamp(GtkScale *scale, gdouble value, gpointer user_data)
{
	return g_strdup_printf("%0.2f", 4.0-value);
}

gchar* scale_valuevolume(GtkScale *scale, gdouble value, gpointer user_data)
{
	return g_strdup_printf("%0.2f", 1.0-value);
}

static void eq_toggled(GtkWidget *togglebutton, gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;

	AudioEqualizer_setEnabled(&(vpw->ae), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)));
	//printf("toggle state %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(eqenable)));
}

static void eq_autotoggled(GtkWidget *togglebutton, gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;

	AudioEqualizer_setAutoLeveling(&(vpw->ae), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)));
	//printf("toggle state %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(eqenable)));
}

int select_eqpresetnames_callback(void *data, int argc, char **argv, char **azColName) 
{
	vpwidgets *vpw = (vpwidgets*)data;

	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(vpw->combopreset), argv[0], argv[12]);
	return 0;
}

void select_eqpreset_names(vpwidgets *vpw)
{
	sqlite3 *db;
	char *err_msg = NULL;
	char* sql;
	int rc;

	if((rc = sqlite3_open("/var/sqlite3DATA/mediaplayer.db", &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");
		sql = "SELECT * FROM eqpresets order by id;";
		if ((rc = sqlite3_exec(db, sql, select_eqpresetnames_callback, vpw, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to select data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}
	}
	sqlite3_close(db);
}

void point2comma(char *c)
{
	int i;

	for(i=0;c[i];i++)
		if (c[i]=='.')
			c[i]=',';
}

int select_eqpreset_callback(void *data, int argc, char **argv, char **azColName) 
{
	vpwidgets *vpw = (vpwidgets*)data;

	char* errCheck;
	double f;

	point2comma(argv[1]);
	f=strtod(argv[1], &errCheck);
	if (errCheck == argv[1])
	{
		f=0.0;
		printf("Conversion error %s %s\n", azColName[1], argv[1]);
	}
	gtk_adjustment_set_value(vpw->vadj0, -f);

	point2comma(argv[2]);
	f=strtod(argv[2], &errCheck);
	if (errCheck == argv[2])
	{
		f=0.0;
		printf("Conversion error %s %s\n", azColName[2], argv[2]);
	}
	gtk_adjustment_set_value(vpw->vadj1, -f);

	point2comma(argv[3]);
	f=strtod(argv[3], &errCheck);
	if (errCheck == argv[3])
	{
		f=0.0;
		printf("Conversion error %s %s\n", azColName[3], argv[3]);
	}
	gtk_adjustment_set_value(vpw->vadj2, -f);

	point2comma(argv[4]);
	f=strtod(argv[4], &errCheck);
	if (errCheck == argv[4])
	{
		f=0.0;
		printf("Conversion error %s %s\n", azColName[4], argv[4]);
	}
	gtk_adjustment_set_value(vpw->vadj3, -f);

	point2comma(argv[5]);
	f=strtod(argv[5], &errCheck);
	if (errCheck == argv[5])
	{
		f=0.0;
		printf("Conversion error %s %s\n", azColName[5], argv[5]);
	}
	gtk_adjustment_set_value(vpw->vadj4, -f);

	point2comma(argv[6]);
	f=strtod(argv[6], &errCheck);
	if (errCheck == argv[6])
	{
		f=0.0;
		printf("Conversion error %s %s\n", azColName[6], argv[6]);
	}
	gtk_adjustment_set_value(vpw->vadj5, -f);

	point2comma(argv[7]);
	f=strtod(argv[7], &errCheck);
	if (errCheck == argv[7])
	{
		f=0.0;
		printf("Conversion error %s %s\n", azColName[7], argv[7]);
	}
	gtk_adjustment_set_value(vpw->vadj6, -f);

	point2comma(argv[8]);
	f=strtod(argv[8], &errCheck);
	if (errCheck == argv[8])
	{
		f=0.0;
		printf("Conversion error %s %s\n", azColName[8], argv[8]);
	}
	gtk_adjustment_set_value(vpw->vadj7, -f);

	point2comma(argv[9]);
	f=strtod(argv[9], &errCheck);
	if (errCheck == argv[9])
	{
		f=0.0;
		printf("Conversion error %s %s\n", azColName[9], argv[9]);
	}
	gtk_adjustment_set_value(vpw->vadj8, -f);

	point2comma(argv[10]);
	f=strtod(argv[10], &errCheck);
	if (errCheck == argv[10])
	{
		f=0.0;
		printf("Conversion error %s %s\n", azColName[10], argv[10]);
	}
	gtk_adjustment_set_value(vpw->vadj9, -f);

	if (vpw->ae.autoleveling)
	{
		gtk_range_set_value(GTK_RANGE(vpw->vscaleeqA), 4.0-vpw->ae.effgain);
	}

	return 0;
}

void select_eqpreset(vpwidgets *vpw, char* id)
{
	sqlite3 *db;
	char *err_msg = NULL;
	char sql[256];
	int rc;

	if((rc = sqlite3_open("/var/sqlite3DATA/mediaplayer.db", &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");
		sql[0] = '\0';
		strcat(sql, "SELECT * FROM eqpresets where id=");
		strcat(sql, id);
		strcat(sql, ";");
		if((rc = sqlite3_exec(db, sql, select_eqpreset_callback, vpw, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to select data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}
	}
	sqlite3_close(db);
}

static void preset_changed(GtkWidget *combo, gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;

	gchar *strval;

	g_object_get((gpointer)combo, "active-id", &strval, NULL);
	//printf("Selected id %s\n", strval);
	select_eqpreset(vpw, strval);
	g_free(strval);
}

gboolean scale_pressed(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	vpwidgets *vpw = (vpwidgets*)user_data;
	videoplayer *vpp = &(vpw->vp);

	if (vpp->vpq.playerstatus==PLAYING)
	{
		pthread_mutex_lock(&(vpp->seekmutex));
		vpp->vpq.playerstatus = PAUSED;
		pthread_mutex_unlock(&(vpp->seekmutex));

		vpw->sliding = 1;
		vpw->hscaleupd = FALSE;
	}
	return FALSE;
}

gboolean scale_released(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	vpwidgets *vpw = (vpwidgets*)user_data;
	videoplayer *vpp = &(vpw->vp);
	int64_t seek_ts;

	double value = gtk_range_get_value(GTK_RANGE(widget));
//printf("Released value: %f\n", value);

	if (vpw->sliding)
	{
		if (vpp->videoduration)
		{
			AVStream *st = vpp->pFormatCtx->streams[vpp->videoStream];
			double seektime = value / vpp->frame_rate; // seconds
//printf("Seek time %5.2f\n", seektime);
			int flgs = AVSEEK_FLAG_ANY;
			seek_ts = (seektime * (st->time_base.den)) / (st->time_base.num);
			if (av_seek_frame(vpp->pFormatCtx, vpp->videoStream, seek_ts, flgs) < 0)
			{
				printf("Failed to seek Video\n");
			}
			else
			{
				avcodec_flush_buffers(vpp->pCodecCtx);
				if (vpp->audioStream!=-1)
					avcodec_flush_buffers(vpp->pCodecCtxA);

				//vq_drain(&vq);
				//aq_drain(&aq);
			}

			pthread_mutex_lock(&(vpp->framemutex));
			vpp->now_decoding_frame = (int64_t)value;
			pthread_mutex_unlock(&(vpp->framemutex));
		}
		else
		{
			AVStream *st = vpp->pFormatCtx->streams[vpp->audioStream];
			double seektime = value / vpp->sample_rate; // seconds
//printf("Seek time %5.2f\n", seektime);
			int flgs = AVSEEK_FLAG_ANY;
			seek_ts = (seektime * (st->time_base.den)) / (st->time_base.num);
//printf("seek_ts %lld\n", seek_ts);
			if (av_seek_frame(vpp->pFormatCtx, vpp->audioStream, seek_ts, flgs) < 0)
			{
				printf("Failed to seek Audio\n");
			}
			else
			{
				avcodec_flush_buffers(vpp->pCodecCtxA);
				//vq_drain(&vq);
				//aq_drain(&aq);
			}
			pthread_mutex_lock(&(vpp->framemutex));
			vpp->now_playing_frame = (int64_t)value;
			pthread_mutex_unlock(&(vpp->framemutex));
		}

		vpw->sliding = 0;
		vpw->hscaleupd = TRUE;

		pthread_mutex_lock(&(vpp->seekmutex));
		vpp->vpq.playerstatus = PLAYING;
		pthread_cond_signal(&(vpp->pausecond)); // Should wake up *one* thread
		pthread_mutex_unlock(&(vpp->seekmutex));
	}
	return FALSE;
}

void listview_onRowActivated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer userdata)
{
	playlistparams *plp = (playlistparams*)userdata;
	vpwidgets *vpw = plp->vpw;
	videoplayer *vpp = &(vpw->vp);
	GtkTreeModel *model;
	GtkTreeIter iter;
	//gchar *s;

	if (vpp->vpq.playerstatus==PAUSED)
	{
		requeststop_videoplayer(&(vpw->vp));
		vpp->vpq.playerstatus = PLAYING;
		gtk_button_set_label(GTK_BUTTON(vpw->button10), "Pause");
		pthread_mutex_unlock(&(vpp->seekmutex));
	}

//g_print("double-clicked\n");
	model = gtk_tree_view_get_model(treeview);
	if (gtk_tree_model_get_iter(model, &iter, path))
	{
		//s=gtk_tree_path_to_string(path);
		//printf("%s\n",s);
		//g_free(s);
		if (!(vpp->vpq.playerstatus == IDLE))
		{
			button2_clicked(vpw->button2, userdata);
		}
		if (vpp->now_playing)
		{
			g_free(vpp->now_playing);
			vpp->now_playing = NULL;
		}
		gtk_tree_model_get(model, &iter, COL_FILEPATH, &(vpp->now_playing), -1);
//g_print ("Double-clicked path %s\n", vpp->now_playing);

		if (vpp->vpq.playerstatus!=IDLE)
			button2_clicked(vpw->button2, userdata);
		button1_clicked(vpw->button1, userdata);
	}
}

void resize_containers(vpwidgets *vpw)
{
/*
	GtkAllocation *alloc = g_new(GtkAllocation, 1);
	gtk_widget_get_allocation(vpw->hscale, alloc);
	int scaleheight = alloc->height;
	g_free(alloc);
	//printf("Scale height %d\n", scaleheight);
	//gtk_widget_set_size_request(vpw->scrolled_window, vpw->playerWidth, vpw->playerHeight+scaleheight);

	gtk_widget_set_size_request(vpw->vscaleeq0, vpw->playerWidth/13, vpw->playerHeight);
	gtk_widget_set_size_request(vpw->vscaleeq1, vpw->playerWidth/13, vpw->playerHeight);
	gtk_widget_set_size_request(vpw->vscaleeq2, vpw->playerWidth/13, vpw->playerHeight);
	gtk_widget_set_size_request(vpw->vscaleeq3, vpw->playerWidth/13, vpw->playerHeight);
	gtk_widget_set_size_request(vpw->vscaleeq4, vpw->playerWidth/13, vpw->playerHeight);
	gtk_widget_set_size_request(vpw->vscaleeq5, vpw->playerWidth/13, vpw->playerHeight);
	gtk_widget_set_size_request(vpw->vscaleeq6, vpw->playerWidth/13, vpw->playerHeight);
	gtk_widget_set_size_request(vpw->vscaleeq7, vpw->playerWidth/13, vpw->playerHeight);
	gtk_widget_set_size_request(vpw->vscaleeq8, vpw->playerWidth/13, vpw->playerHeight);
	gtk_widget_set_size_request(vpw->vscaleeq9, vpw->playerWidth/13, vpw->playerHeight);
	gtk_widget_set_size_request(vpw->vscaleeqA, vpw->playerWidth/13, vpw->playerHeight);
	gtk_widget_set_size_request(vpw->vscaleeqV, vpw->playerWidth/13, vpw->playerHeight);
*/
}

/* Called when the windows are realized */
static void vp_realize_cb(GtkWidget *widget, gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;
	resize_containers(vpw);
	g_object_set((gpointer)vpw->combopreset, "active-id", "0", NULL);
}

static gboolean vp_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    /* If you return FALSE in the "delete-event" signal handler,
     * GTK will emit the "destroy" signal. Returning TRUE means
     * you don't want the window to be destroyed.
     * This is useful for popping up 'are you sure you want to quit?'
     * type dialogs. */
//g_print ("delete event occurred\n");
    return TRUE;
}

static void vp_destroy(GtkWidget *widget, gpointer data)
{
//printf("vp_destroy\n");
    //gtk_main_quit();
}

void toggle_vp(vpwidgets *vpw, GtkWidget *togglebutton)
{
	vpw->vpvisible = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton));
	if (vpw->vpvisible)
		gtk_widget_show_all(vpw->vpwindow);
	else
		gtk_widget_hide(vpw->vpwindow);
}

void page_switched(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data)
{
	vpwidgets *vpw = (vpwidgets *)user_data;

//printf("switched to page %d\n", page_num);
	vpw->vp.decodevideo = !page_num;
}

gboolean update_hscale(gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;
	videoplayer *vp = &(vpw->vp);

	if (vp->hscaleupd)
		gtk_adjustment_set_value(vpw->hadjustment, vp->now_playing_frame);
	return FALSE;
}

static void micnull_toggled(GtkWidget *togglebutton, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	microphone *mic = plp->mic;

	mic->nullsamples = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton));
	//printf("toggle state %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)));
}

/*
gboolean setLevel1(gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;
	videoplayer *vp = &(vpw->vp);
	char leveltext[10];

	gtk_level_bar_set_value(GTK_LEVEL_BAR(vpw->levelbar1), vp->diff1);
	sprintf(leveltext, "%ld", vp->diff1);
	gtk_label_set_text(GTK_LABEL(vpw->label1), leveltext);
	return FALSE;
}

gboolean setLevel2(gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;
	videoplayer *vp = &(vpw->vp);
	char leveltext[10];

	gtk_level_bar_set_value(GTK_LEVEL_BAR(vpw->levelbar2), vp->diff2);
	sprintf(leveltext, "%ld", vp->diff2);
	gtk_label_set_text(GTK_LABEL(vpw->label2), leveltext);
	return FALSE;
}

gboolean setLevel3(gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;
	videoplayer *vp = &(vpw->vp);
	char leveltext[10];

	gtk_level_bar_set_value(GTK_LEVEL_BAR(vpw->levelbar3), vp->diff3);
	sprintf(leveltext, "%ld", vp->diff3);
	gtk_label_set_text(GTK_LABEL(vpw->label3), leveltext);
	return FALSE;
}

gboolean setLevel4(gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;
	videoplayer *vp = &(vpw->vp);
	char leveltext[10];

	gtk_level_bar_set_value(GTK_LEVEL_BAR(vpw->levelbar4), vp->diff4);
	sprintf(leveltext, "%ld", vp->diff4);
	gtk_label_set_text(GTK_LABEL(vpw->label4), leveltext);
	return FALSE;
}

gboolean setLevel7(gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;
	videoplayer *vp = &(vpw->vp);
	char leveltext[10];

	gtk_level_bar_set_value(GTK_LEVEL_BAR(vpw->levelbar7), vp->diff7);
	sprintf(leveltext, "%ld %%", vp->diff7);
	gtk_label_set_text(GTK_LABEL(vpw->label7), leveltext);
	return FALSE;
}

gboolean setLevel8(gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;
	videoplayer *vp = &(vpw->vp);
	char leveltext[10];

	sprintf(leveltext, "%d", vp->aqLength);
	gtk_label_set_text(GTK_LABEL(vpw->parmlabel12), leveltext);
	return FALSE;
}

gboolean setLevel9(gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;
	videoplayer *vp = &(vpw->vp);
	char leveltext[10];

	sprintf(leveltext, "%d", vp->vpq.vqLength);
	gtk_label_set_text(GTK_LABEL(vpw->parmlabel22), leveltext);
	return FALSE;
}

void resetLevels(videoplayer *v)
{
	vpwidgets *vpw = (vpwidgets*)(v->vpwp);

	v->diff1 = v->diff2 = v->diff3 = v->diff4 = 0;
	v->diff7 = 0.0;

	gdk_threads_add_idle(setLevel1, (void*)vpw);
	gdk_threads_add_idle(setLevel2, (void*)vpw);
	gdk_threads_add_idle(setLevel3, (void*)vpw);
	gdk_threads_add_idle(setLevel4, (void*)vpw);
	gdk_threads_add_idle(setLevel7, (void*)vpw);
}

gboolean delete_event_parm(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;

    // If you return FALSE in the "delete-event" signal handler,
    // GTK will emit the "destroy" signal. Returning TRUE means
    // you don't want the window to be destroyed.
    // This is useful for popping up 'are you sure you want to quit?'
    // type dialogs.
//g_print ("delete event occurred\n");
	buttonParameters_clicked(vpw->buttonParameters, data);
    return TRUE;
}

// Called when the windows are realized
void realize_parm(GtkWidget *widget, gpointer data)
{
}

void aqmaxlength_changed(GtkWidget *widget, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;

	plp->aqMaxLength = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
}

void vqmaxlength_changed(GtkWidget *widget, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;

	plp->vqMaxLength = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
}

static void threadcount_changed(GtkWidget *widget, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;

	plp->thread_count = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)); 
}
*/

void drag_data_received_da_event(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, guint info, guint time, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;
	videoplayer *vp = &(vpw->vp);
	videoplayerqueue *vpq = &(vp->vpq);
	int i, itemcount, currentpage;

//printf("drag_data_received\n");		
	guchar *str = gtk_selection_data_get_text(selection_data);

	if (!(currentpage = gtk_notebook_get_current_page(GTK_NOTEBOOK(vpw->notebook))))
		button3_clicked(vpw->button3, plp); // Clear list

	itemcount = i = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(vpw->store), NULL); // rows
	char *q;
	char *p = (char *)str;
	while(*p)
	{
		q = strchr(p, '\n');
		*q = '\0';
//printf("%s\n", p);
		gchar *path = g_filename_from_uri(p, NULL, NULL);
		int j = strlen(path);
		if (path[j-1]=='\r') path[j-1] = '\0';
//printf("%s\n", path);
		if (nomediafile((char*)path))
		{}
		else
		{
			gtk_list_store_append(vpw->store, &(vpw->iter));
			gtk_list_store_set(vpw->store, &(vpw->iter), COL_ID, i++, COL_FILEPATH, path, -1);
		}
		g_free(path);
		p = q + 1;
	}
	g_free(str);

	if (!itemcount)
	{
		if (vpq->playerstatus==PLAYING)
			button2_clicked(vpw->button2, (void*)plp);
		if (vpq->playerstatus==IDLE)
		{
			if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(vpw->store), &(vpw->iter), NULL, 0))
			{
				if (vp->now_playing)
				{
					g_free(vp->now_playing);
					vp->now_playing = NULL;
				}
				gtk_tree_model_get(GTK_TREE_MODEL(vpw->store), &(vpw->iter), COL_FILEPATH, &(vp->now_playing), -1);
				gdk_threads_add_idle(focus_iter_idle, (void*)vpw);
				gtk_notebook_set_current_page(GTK_NOTEBOOK(vpw->notebook), 0);
				button1_clicked(vpw->button1, (void*)plp);
			}
		}
	}
}

gboolean drag_drop_da_event(GtkWidget *widget, GdkDragContext*context, gint x, gint y, guint time, gpointer data)
{
//printf("drag_drop\n");
	return FALSE;
}

void drag_motion_da_event(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer data)
{
//printf("drag_motion\n");
}

void drag_leave_da_event(GtkWidget *widget, GdkDragContext *context, guint time, gpointer data)
{
//printf("drag_leave\n");
}

/*
static void buttonTest_clicked(GtkWidget *button, gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;
	//videoplayer *vp = &(vpw->vp);
	int width = 0, height = 0;

	GtkAllocation* alloc = g_new(GtkAllocation, 1);

	width+=2*gtk_container_get_border_width(GTK_CONTAINER(vpw->vpwindow));

	height+=2*gtk_container_get_border_width(GTK_CONTAINER(vpw->vpwindow));
	gtk_widget_get_allocation(vpw->hscale, alloc);
	height+=alloc->height;
	gtk_widget_get_allocation(vpw->statusbar, alloc);
	height+=alloc->height;
	height+=82;
	g_free(alloc);

	width+=1280;
	height+=720;

	gtk_window_resize(GTK_WINDOW(vpw->vpwindow), 1284, height);
}
*/

void new_drawingarea(playlistparams *plp)
{
	vpwidgets *vpw = plp->vpw;
	oglidle *oi = (oglidle*)&(vpw->vp.oi);
	oglstate *ogl = &(oi->ogl);

	vpw->drawingarea = gtk_drawing_area_new();
	vpw->vp.playerWidth = vpw->playerWidth;
	vpw->vp.playerHeight = vpw->playerHeight;
	vpw->vp.drawingarea = vpw->drawingarea;

	ogl->fmt = RGBA;
	ogl->width = vpw->playerWidth;
	ogl->height = vpw->playerHeight;
	ogl->linewidth = vpw->playerWidth;
	ogl->codecheight = vpw->playerHeight;
//printf("new drawing area %d * %d\n", ogl->width, ogl->height);
	oi->widget = vpw->drawingarea;

	//gtk_widget_set_double_buffered(vpw->drawingarea, FALSE);
	gtk_widget_set_size_request(vpw->drawingarea, vpw->playerWidth, vpw->playerHeight);
	g_signal_connect(vpw->drawingarea, "realize", G_CALLBACK(realize_da_event), (void*)oi);
	g_signal_connect(vpw->drawingarea, "draw", G_CALLBACK(draw_da_event), (void*)oi);
	//g_signal_connect(vpw->drawingarea, "size-allocate", G_CALLBACK(size_allocate_da_event), (void*)oi);
	g_signal_connect(vpw->drawingarea, "destroy", G_CALLBACK(destroy_da_event), (void*)oi);

	gtk_drag_dest_set(vpw->drawingarea, GTK_DEST_DEFAULT_ALL, vpw->target_entries, G_N_ELEMENTS(vpw->target_entries), GDK_ACTION_COPY | GDK_ACTION_MOVE );
	g_signal_connect(vpw->drawingarea, "drag_data_received", G_CALLBACK(drag_data_received_da_event), (void*)plp);
	g_signal_connect(vpw->drawingarea, "drag_drop", G_CALLBACK(drag_drop_da_event), NULL);
	g_signal_connect(vpw->drawingarea, "drag_motion", G_CALLBACK(drag_motion_da_event), NULL);
	g_signal_connect(vpw->drawingarea, "drag_leave", G_CALLBACK(drag_leave_da_event), NULL);
}

void init_target_list(vpwidgets *vpw)
{
	int i;

	GtkTargetEntry target_entries[2] = 
	{
		{ "text/html", GTK_TARGET_OTHER_APP, text_html },
		{ "STRING", GTK_TARGET_OTHER_APP, string }
	};

	for(i=0;i<G_N_ELEMENTS(target_entries);i++)
		vpw->target_entries[i] = target_entries[i];
}

void init_videoplayerwidgets(playlistparams *plp, int argc, char** argv, int playWidth, int playHeight, audiomixer *x)
{
	vpwidgets *vpw = plp->vpw;
	vpw->ax = x; // Audio Mixer

	init_target_list(vpw);
	
	vpw->vp.now_playing = NULL; // Video Player
	vpw->vp.vpq.playerstatus = IDLE;
	vpw->vp.vpwp = (void*)vpw;
	vpw->vp.spk_samplingrate = plp->spk_samplingrate;
	vpw->vp.thread_count = plp->thread_count;

	vpw->playerWidth = playWidth;
	vpw->playerHeight = playHeight;
	vpw->sliding = 0;

	int i;
	for(i=0;i<4;i++)
	{
		CPU_ZERO(&(vpw->cpu[i]));
		CPU_SET(i, &(vpw->cpu[i]));
	}

    /* create a new window */
	vpw->vpwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(vpw->vpwindow), GTK_WIN_POS_CENTER);
	/* Sets the border width of the window. */
	gtk_container_set_border_width(GTK_CONTAINER(vpw->vpwindow), 2);
	//gtk_widget_set_size_request(vpw->vpwindow, 100, 100);
	gtk_window_set_title(GTK_WINDOW(vpw->vpwindow), "Video Player");
	//gtk_window_set_resizable(GTK_WINDOW(vpw->vpwindow), FALSE);

	/* When the window is given the "delete-event" signal (this is given
     * by the window manager, usually by the "close" option, or on the
     * titlebar), we ask it to call the delete_event () function
     * as defined above. The data passed to the callback
     * function is NULL and is ignored in the callback function. */
	g_signal_connect(vpw->vpwindow, "delete-event", G_CALLBACK(vp_delete_event), (void*)vpw);
    
	/* Here we connect the "destroy" event to a signal handler.  
     * This event occurs when we call gtk_widget_destroy() on the window,
     * or if we return FALSE in the "delete-event" callback. */
	g_signal_connect(vpw->vpwindow, "destroy", G_CALLBACK(vp_destroy), (void*)vpw);

	g_signal_connect(vpw->vpwindow, "realize", G_CALLBACK(vp_realize_cb), (void*)vpw);
//printf("realized\n");

// vertical box
	vpw->playerbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_add(GTK_CONTAINER(vpw->vpwindow), vpw->playerbox);

// box1 contents begin
// vertical box
	vpw->box1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

// gl cairo drawing area
	new_drawingarea(plp);
	gtk_box_pack_start(GTK_BOX(vpw->box1), vpw->drawingarea, TRUE, TRUE, 0);

// horizontal scale
	vpw->hadjustment = gtk_adjustment_new(50, 0, 100, 1, 10, 0);
	vpw->hscale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, GTK_ADJUSTMENT(vpw->hadjustment));
	gtk_scale_set_digits(GTK_SCALE(vpw->hscale), 0);
	//g_signal_connect(hscale, "value-changed", G_CALLBACK(hscale_adjustment), NULL);
	g_signal_connect(vpw->hscale, "button-press-event", G_CALLBACK(scale_pressed), (void*)vpw);
	g_signal_connect(vpw->hscale, "button-release-event", G_CALLBACK(scale_released), (void*)vpw);
	gtk_container_add(GTK_CONTAINER(vpw->box1), vpw->hscale);

// horizontal box
	vpw->horibox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(vpw->box1), vpw->horibox);

// horizontal button box
    vpw->button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout((GtkButtonBox *)vpw->button_box, GTK_BUTTONBOX_START);
    gtk_container_add(GTK_CONTAINER(vpw->horibox), vpw->button_box);

// button prev
	vpw->button9 = gtk_button_new_with_label("Prev");
	gtk_widget_set_sensitive (vpw->button9, FALSE);
	g_signal_connect(GTK_BUTTON(vpw->button9), "clicked", G_CALLBACK(button9_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box), vpw->button9);

// button play
	vpw->button1 = gtk_button_new_with_label("Play");
	g_signal_connect(GTK_BUTTON(vpw->button1), "clicked", G_CALLBACK(button1_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box), vpw->button1);

// button pause/resume
	vpw->button10 = gtk_button_new_with_label("Pause");
	gtk_widget_set_sensitive (vpw->button10, FALSE);
	g_signal_connect(GTK_BUTTON(vpw->button10), "clicked", G_CALLBACK(button10_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box), vpw->button10);

// button next
	vpw->button8 = gtk_button_new_with_label("Next");
	gtk_widget_set_sensitive (vpw->button8, FALSE);
	g_signal_connect(GTK_BUTTON(vpw->button8), "clicked", G_CALLBACK(button8_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box), vpw->button8);

// button stop
	vpw->button2 = gtk_button_new_with_label("Stop");
	gtk_widget_set_sensitive(vpw->button2, FALSE);
	g_signal_connect(GTK_BUTTON(vpw->button2), "clicked", G_CALLBACK(button2_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box), vpw->button2);

/*
// button Parameters
	vpw->buttonParameters = gtk_button_new_with_label("AV");
	g_signal_connect(GTK_BUTTON(vpw->buttonParameters), "clicked", G_CALLBACK(buttonParameters_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box), vpw->buttonParameters);
*/
/*
// test button
	vpw->buttonTest = gtk_button_new_with_label("Test");
	g_signal_connect(GTK_BUTTON(vpw->buttonTest), "clicked", G_CALLBACK(buttonTest_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box), vpw->buttonTest);
*/

// box1 contents end

// box2 contents begin
	vpw->box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);

	vpw->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(vpw->scrolled_window), 10);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(vpw->scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	//gtk_widget_set_size_request(vpw->scrolled_window, vpw->playerWidth, vpw->playerHeight);
	//gtk_container_add(GTK_CONTAINER(vpw->box2), vpw->scrolled_window);
	gtk_box_pack_start(GTK_BOX(vpw->box2), vpw->scrolled_window, TRUE, TRUE, 0);

	vpw->listview = create_view_and_model(vpw, argc, argv);
	g_signal_connect(vpw->listview, "row-activated", (GCallback)listview_onRowActivated, (void*)plp);
	gtk_drag_dest_set(vpw->listview, GTK_DEST_DEFAULT_ALL, vpw->target_entries, G_N_ELEMENTS(vpw->target_entries), GDK_ACTION_COPY | GDK_ACTION_MOVE );
	g_signal_connect(vpw->listview, "drag_data_received", G_CALLBACK(drag_data_received_da_event), (void*)plp);
	g_signal_connect(vpw->listview, "drag_drop", G_CALLBACK(drag_drop_da_event), NULL);
	g_signal_connect(vpw->listview, "drag_motion", G_CALLBACK(drag_motion_da_event), NULL);
	g_signal_connect(vpw->listview, "drag_leave", G_CALLBACK(drag_leave_da_event), NULL);
	gtk_container_add(GTK_CONTAINER(vpw->scrolled_window), vpw->listview);

	vpw->button_box2 = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_button_box_set_layout((GtkButtonBox*)vpw->button_box2, GTK_BUTTONBOX_START);
	gtk_container_add(GTK_CONTAINER(vpw->box2), vpw->button_box2);

	vpw->button3 = gtk_button_new_with_label("Clear");
	g_signal_connect(GTK_BUTTON(vpw->button3), "clicked", G_CALLBACK(button3_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box2), vpw->button3);

	vpw->button6 = gtk_button_new_with_label("Catalog");
	g_signal_connect(GTK_BUTTON(vpw->button6), "clicked", G_CALLBACK(button6_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box2), vpw->button6);

	vpw->button7 = gtk_button_new_with_label("Add File");
	g_signal_connect(GTK_BUTTON(vpw->button7), "clicked", G_CALLBACK(button7_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box2), vpw->button7);

	vpw->button4 = gtk_button_new_with_label("Load");
	g_signal_connect(GTK_BUTTON(vpw->button4), "clicked", G_CALLBACK(button4_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box2), vpw->button4);

	vpw->button11 = gtk_button_new_with_label("Open Files");
	g_signal_connect(GTK_BUTTON(vpw->button11), "clicked", G_CALLBACK(button11_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box2), vpw->button11);

// box2 contents end


	set_eqdefaults(&(vpw->aedef));
	AudioEqualizer_init(&(vpw->ae), 10, 1.0, 1, 1, SND_PCM_FORMAT_S16, vpw->vp.spk_samplingrate, 2, &(vpw->aedef));
// eqvbox contents begin
// vertical box
	vpw->eqvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

// horizontal box    
	vpw->eqbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	//gtk_container_add(GTK_CONTAINER(vpw->eqvbox), vpw->eqbox);
	gtk_box_pack_start(GTK_BOX(vpw->eqvbox), vpw->eqbox, TRUE, TRUE, 0);

// vertical scale 0
	vpw->eqbox0 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox), vpw->eqbox0);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox), vpw->eqbox0, TRUE, TRUE, 0);
	vpw->vadj0 = gtk_adjustment_new(vpw->aedef.dbGains[0], -12, 12, 0.1, 1, 0);
	vpw->vscaleeq0 = gtk_scale_new(GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT(vpw->vadj0));
	gtk_scale_set_digits(GTK_SCALE(vpw->vscaleeq0), 1);
	g_signal_connect(vpw->vscaleeq0, "value-changed", G_CALLBACK(vscale0), (void*)vpw);
	g_signal_connect(vpw->vscaleeq0, "format-value", G_CALLBACK(scale_valueformat), (void*)vpw);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox0), vpw->vscaleeq0);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox0), vpw->vscaleeq0, TRUE, TRUE, 0);
	vpw->eqlabel0 = gtk_label_new(vpw->aedef.eqlabels[0]);
	gtk_container_add(GTK_CONTAINER(vpw->eqbox0), vpw->eqlabel0);
	//gtk_box_pack_start(GTK_BOX(vpw->eqbox0), vpw->eqlabel0, TRUE, TRUE, 0);

// vertical scale 1
	vpw->eqbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox), vpw->eqbox1);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox), vpw->eqbox1, TRUE, TRUE, 0);
	vpw->vadj1 = gtk_adjustment_new(vpw->aedef.dbGains[1], -12, 12, 0.1, 1, 0);
	vpw->vscaleeq1 = gtk_scale_new(GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT(vpw->vadj1));
	gtk_scale_set_digits(GTK_SCALE(vpw->vscaleeq1), 1);
	g_signal_connect(vpw->vscaleeq1, "value-changed", G_CALLBACK(vscale1), (void*)vpw);
	g_signal_connect(vpw->vscaleeq1, "format-value", G_CALLBACK(scale_valueformat), (void*)vpw);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox1), vpw->vscaleeq1);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox1), vpw->vscaleeq1, TRUE, TRUE, 0);
	vpw->eqlabel1 = gtk_label_new(vpw->aedef.eqlabels[1]);
	gtk_container_add(GTK_CONTAINER(vpw->eqbox1), vpw->eqlabel1);
	//gtk_box_pack_start(GTK_BOX(vpw->eqbox1), vpw->eqlabel1, TRUE, TRUE, 0);

// vertical scale 2
	vpw->eqbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox), vpw->eqbox2);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox), vpw->eqbox2, TRUE, TRUE, 0);
	vpw->vadj2 = gtk_adjustment_new(vpw->aedef.dbGains[2], -12, 12, 0.1, 1, 0);
	vpw->vscaleeq2 = gtk_scale_new(GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT(vpw->vadj2));
	gtk_scale_set_digits(GTK_SCALE(vpw->vscaleeq2), 1);
	g_signal_connect(vpw->vscaleeq2, "value-changed", G_CALLBACK(vscale2), (void*)vpw);
	g_signal_connect(vpw->vscaleeq2, "format-value", G_CALLBACK(scale_valueformat), (void*)vpw);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox2), vpw->vscaleeq2);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox2), vpw->vscaleeq2, TRUE, TRUE, 0);
	vpw->eqlabel2 = gtk_label_new(vpw->aedef.eqlabels[2]);
	gtk_container_add(GTK_CONTAINER(vpw->eqbox2), vpw->eqlabel2);
	//gtk_box_pack_start(GTK_BOX(vpw->eqbox2), vpw->eqlabel2, TRUE, TRUE, 0);

// vertical scale 3
	vpw->eqbox3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox), vpw->eqbox3);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox), vpw->eqbox3, TRUE, TRUE, 0);
	vpw->vadj3 = gtk_adjustment_new(vpw->aedef.dbGains[3], -12, 12, 0.1, 1, 0);
	vpw->vscaleeq3 = gtk_scale_new(GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT(vpw->vadj3));
	gtk_scale_set_digits(GTK_SCALE(vpw->vscaleeq3), 1);
	g_signal_connect(vpw->vscaleeq3, "value-changed", G_CALLBACK(vscale3), (void*)vpw);
	g_signal_connect(vpw->vscaleeq3, "format-value", G_CALLBACK(scale_valueformat), (void*)vpw);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox3), vpw->vscaleeq3);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox3), vpw->vscaleeq3, TRUE, TRUE, 0);
	vpw->eqlabel3 = gtk_label_new(vpw->aedef.eqlabels[3]);
	gtk_container_add(GTK_CONTAINER(vpw->eqbox3), vpw->eqlabel3);
	//gtk_box_pack_start(GTK_BOX(vpw->eqbox3), vpw->eqlabel3, TRUE, TRUE, 0);

// vertical scale 4
	vpw->eqbox4 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox), vpw->eqbox4);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox), vpw->eqbox4, TRUE, TRUE, 0);
	vpw->vadj4 = gtk_adjustment_new(vpw->aedef.dbGains[4], -12, 12, 0.1, 1, 0);
	vpw->vscaleeq4 = gtk_scale_new(GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT(vpw->vadj4));
	gtk_scale_set_digits(GTK_SCALE(vpw->vscaleeq4), 1);
	g_signal_connect(vpw->vscaleeq4, "value-changed", G_CALLBACK(vscale4), (void*)vpw);
	g_signal_connect(vpw->vscaleeq4, "format-value", G_CALLBACK(scale_valueformat), (void*)vpw);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox4), vpw->vscaleeq4);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox4), vpw->vscaleeq4, TRUE, TRUE, 0);
	vpw->eqlabel4 = gtk_label_new(vpw->aedef.eqlabels[4]);
	gtk_container_add(GTK_CONTAINER(vpw->eqbox4), vpw->eqlabel4);
	//gtk_box_pack_start(GTK_BOX(vpw->eqbox4), vpw->eqlabel4, TRUE, TRUE, 0);

// vertical scale 5
	vpw->eqbox5 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox), vpw->eqbox5);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox), vpw->eqbox5, TRUE, TRUE, 0);
	vpw->vadj5 = gtk_adjustment_new(vpw->aedef.dbGains[5], -12, 12, 0.1, 1, 0);
	vpw->vscaleeq5 = gtk_scale_new(GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT(vpw->vadj5));
	gtk_scale_set_digits(GTK_SCALE(vpw->vscaleeq5), 1);
	g_signal_connect(vpw->vscaleeq5, "value-changed", G_CALLBACK(vscale5), (void*)vpw);
	g_signal_connect(vpw->vscaleeq5, "format-value", G_CALLBACK(scale_valueformat), (void*)vpw);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox5), vpw->vscaleeq5);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox5), vpw->vscaleeq5, TRUE, TRUE, 0);
	vpw->eqlabel5 = gtk_label_new(vpw->aedef.eqlabels[5]);
	gtk_container_add(GTK_CONTAINER(vpw->eqbox5), vpw->eqlabel5);
	//gtk_box_pack_start(GTK_BOX(vpw->eqbox5), vpw->eqlabel5, TRUE, TRUE, 0);

// vertical scale 6
	vpw->eqbox6 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox), vpw->eqbox6);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox), vpw->eqbox6, TRUE, TRUE, 0);
	vpw->vadj6 = gtk_adjustment_new(vpw->aedef.dbGains[6], -12, 12, 0.1, 1, 0);
	vpw->vscaleeq6 = gtk_scale_new(GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT(vpw->vadj6));
	gtk_scale_set_digits(GTK_SCALE(vpw->vscaleeq6), 1);
	g_signal_connect(vpw->vscaleeq6, "value-changed", G_CALLBACK(vscale6), (void*)vpw);
	g_signal_connect(vpw->vscaleeq6, "format-value", G_CALLBACK(scale_valueformat), (void*)vpw);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox6), vpw->vscaleeq6);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox6), vpw->vscaleeq6, TRUE, TRUE, 0);
	vpw->eqlabel6 = gtk_label_new(vpw->aedef.eqlabels[6]);
	gtk_container_add(GTK_CONTAINER(vpw->eqbox6), vpw->eqlabel6);
	//gtk_box_pack_start(GTK_BOX(vpw->eqbox6), vpw->eqlabel6, TRUE, TRUE, 0);

// vertical scale 7
	vpw->eqbox7 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox), vpw->eqbox7);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox), vpw->eqbox7, TRUE, TRUE, 0);
	vpw->vadj7 = gtk_adjustment_new(vpw->aedef.dbGains[7], -12, 12, 0.1, 1, 0);
	vpw->vscaleeq7 = gtk_scale_new(GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT(vpw->vadj7));
	gtk_scale_set_digits(GTK_SCALE(vpw->vscaleeq7), 1);
	g_signal_connect(vpw->vscaleeq7, "value-changed", G_CALLBACK(vscale7), (void*)vpw);
	g_signal_connect(vpw->vscaleeq7, "format-value", G_CALLBACK(scale_valueformat), (void*)vpw);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox7), vpw->vscaleeq7);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox7), vpw->vscaleeq7, TRUE, TRUE, 0);
	vpw->eqlabel7 = gtk_label_new(vpw->aedef.eqlabels[7]);
	gtk_container_add(GTK_CONTAINER(vpw->eqbox7), vpw->eqlabel7);
	//gtk_box_pack_start(GTK_BOX(vpw->eqbox7), vpw->eqlabel7, TRUE, TRUE, 0);

// vertical scale 8
	vpw->eqbox8 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox), vpw->eqbox8);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox), vpw->eqbox8, TRUE, TRUE, 0);
	vpw->vadj8 = gtk_adjustment_new(vpw->aedef.dbGains[8], -12, 12, 0.1, 1, 0);
	vpw->vscaleeq8 = gtk_scale_new(GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT(vpw->vadj8));
	gtk_scale_set_digits(GTK_SCALE(vpw->vscaleeq8), 1);
	g_signal_connect(vpw->vscaleeq8, "value-changed", G_CALLBACK(vscale8), (void*)vpw);
	g_signal_connect(vpw->vscaleeq8, "format-value", G_CALLBACK(scale_valueformat), (void*)vpw);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox8), vpw->vscaleeq8);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox8), vpw->vscaleeq8, TRUE, TRUE, 0);
	vpw->eqlabel8 = gtk_label_new(vpw->aedef.eqlabels[8]);
	gtk_container_add(GTK_CONTAINER(vpw->eqbox8), vpw->eqlabel8);
	//gtk_box_pack_start(GTK_BOX(vpw->eqbox8), vpw->eqlabel8, TRUE, TRUE, 0);

// vertical scale 9
	vpw->eqbox9 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox), vpw->eqbox9);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox), vpw->eqbox9, TRUE, TRUE, 0);
	vpw->vadj9 = gtk_adjustment_new(vpw->aedef.dbGains[9], -12, 12, 0.1, 1, 0);
	vpw->vscaleeq9 = gtk_scale_new(GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT(vpw->vadj9));
	gtk_scale_set_digits(GTK_SCALE(vpw->vscaleeq9), 1);
	g_signal_connect(vpw->vscaleeq9, "value-changed", G_CALLBACK(vscale9), (void*)vpw);
	g_signal_connect(vpw->vscaleeq9, "format-value", G_CALLBACK(scale_valueformat), (void*)vpw);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox9), vpw->vscaleeq9);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox9), vpw->vscaleeq9, TRUE, TRUE, 0);
	vpw->eqlabel9 = gtk_label_new(vpw->aedef.eqlabels[9]);
	gtk_container_add(GTK_CONTAINER(vpw->eqbox9), vpw->eqlabel9);
	//gtk_box_pack_start(GTK_BOX(vpw->eqbox9), vpw->eqlabel9, TRUE, TRUE, 0);

// vertical scale preamp
	vpw->eqboxA = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox), vpw->eqboxA);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox), vpw->eqboxA, TRUE, TRUE, 0);
	vpw->vadjA = gtk_adjustment_new(4.0-vpw->ae.effgain, 0, 4.0, 0.01, 0.1, 0);
	vpw->vscaleeqA = gtk_scale_new(GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT(vpw->vadjA));
	gtk_scale_set_digits(GTK_SCALE(vpw->vscaleeqA), 2);
	g_signal_connect(vpw->vscaleeqA, "value-changed", G_CALLBACK(vscaleA), (void*)vpw);
	g_signal_connect(vpw->vscaleeqA, "format-value", G_CALLBACK(scale_valuepreamp), (void*)vpw);
	//gtk_container_add(GTK_CONTAINER(vpw->eqboxA), vpw->vscaleeqA);
	gtk_box_pack_start(GTK_BOX(vpw->eqboxA), vpw->vscaleeqA, TRUE, TRUE, 0);
	vpw->eqlabelA = gtk_label_new("Preamp");
	gtk_container_add(GTK_CONTAINER(vpw->eqboxA), vpw->eqlabelA);
	//gtk_box_pack_start(GTK_BOX(vpw->eqboxA), vpw->eqlabelA, TRUE, TRUE, 0);

// vertical scale volume
	vpw->eqboxV = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	//gtk_container_add(GTK_CONTAINER(vpw->eqbox), vpw->eqboxV);
	gtk_box_pack_start(GTK_BOX(vpw->eqbox), vpw->eqboxV, TRUE, TRUE, 0);
	vpw->vadjV = gtk_adjustment_new(1.0-vpw->ae.volume, 0, 1.0, 0.01, 0.1, 0);
	vpw->vscaleeqV = gtk_scale_new(GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT(vpw->vadjV));
	gtk_scale_set_digits(GTK_SCALE(vpw->vscaleeqV), 2);
	g_signal_connect(vpw->vscaleeqV, "value-changed", G_CALLBACK(vscaleV), (void*)vpw);
	g_signal_connect(vpw->vscaleeqV, "format-value", G_CALLBACK(scale_valuevolume), (void*)vpw);
	//gtk_container_add(GTK_CONTAINER(vpw->eqboxV), vpw->vscaleeqV);
	gtk_box_pack_start(GTK_BOX(vpw->eqboxV), vpw->vscaleeqV, TRUE, TRUE, 0);
	vpw->eqlabelV = gtk_label_new("Volume");
	gtk_container_add(GTK_CONTAINER(vpw->eqboxV), vpw->eqlabelV);
	//gtk_box_pack_start(GTK_BOX(vpw->eqboxV), vpw->eqlabelV, TRUE, TRUE, 0);

// horizontal box
	vpw->eqbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(vpw->eqvbox), vpw->eqbox2);

// checkbox
	vpw->eqenable = gtk_check_button_new_with_label("EQ Enable");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(vpw->eqenable), vpw->ae.enabled);
	g_signal_connect(GTK_TOGGLE_BUTTON(vpw->eqenable), "toggled", G_CALLBACK(eq_toggled), (void*)vpw);
	gtk_container_add(GTK_CONTAINER(vpw->eqbox2), vpw->eqenable);

// checkbox
	vpw->eqautolevel = gtk_check_button_new_with_label("EQ Auto Level");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(vpw->eqautolevel), vpw->ae.autoleveling);
	g_signal_connect(GTK_TOGGLE_BUTTON(vpw->eqautolevel), "toggled", G_CALLBACK(eq_autotoggled), (void*)vpw);
	gtk_container_add(GTK_CONTAINER(vpw->eqbox2), vpw->eqautolevel);

// combobox
    vpw->combopreset = gtk_combo_box_text_new();
    select_eqpreset_names(vpw);
    g_signal_connect(GTK_COMBO_BOX(vpw->combopreset), "changed", G_CALLBACK(preset_changed), (void*)vpw);
    gtk_container_add(GTK_CONTAINER(vpw->eqbox2), vpw->combopreset);

// eqvbox contents end

// Mixer box contents
// vertical box
	vpw->mixvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

// horizontal box    
	vpw->mixhbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	//gtk_container_add(GTK_CONTAINER(vpw->mixvbox), vpw->mixhbox);
	gtk_box_pack_start(GTK_BOX(vpw->mixvbox), vpw->mixhbox, TRUE, TRUE, 0);

// checkbox
	plp->mic->nullsamples = FALSE;
	vpw->micnull = gtk_check_button_new_with_label("Mute Microphone");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(vpw->micnull), plp->mic->nullsamples);
	g_signal_connect(GTK_TOGGLE_BUTTON(vpw->micnull), "toggled", G_CALLBACK(micnull_toggled), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->mixhbox), vpw->micnull);
// Mixer box contents end

/*
// stack switcher
    vpw->stack = gtk_stack_new();
    vpw->stackswitcher = gtk_stack_switcher_new();
    gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(vpw->stackswitcher), GTK_STACK(vpw->stack));
    gtk_container_add(GTK_CONTAINER(vpw->playerbox), vpw->stackswitcher);
    gtk_container_add(GTK_CONTAINER(vpw->playerbox), vpw->stack);

    gtk_stack_add_titled(GTK_STACK(vpw->stack), vpw->box1, "box1", "Player");
    gtk_stack_add_titled(GTK_STACK(vpw->stack), vpw->box2, "box2", "Playlist");
    gtk_stack_add_titled(GTK_STACK(vpw->stack), vpw->eqvbox, "eqvbox", "Equalizer");
*/
	vpw->notebook = gtk_notebook_new();
	vpw->nbpage1 = gtk_label_new("Player");
	gtk_notebook_append_page(GTK_NOTEBOOK(vpw->notebook), vpw->box1, vpw->nbpage1);
	vpw->nbpage2 = gtk_label_new("Playlist");
	gtk_notebook_append_page(GTK_NOTEBOOK(vpw->notebook), vpw->box2, vpw->nbpage2);
	vpw->nbpage3 = gtk_label_new("Equalizer");
	gtk_notebook_append_page(GTK_NOTEBOOK(vpw->notebook), vpw->eqvbox, vpw->nbpage3);
	vpw->nbpage4 = gtk_label_new("Mixer");
	gtk_notebook_append_page(GTK_NOTEBOOK(vpw->notebook), vpw->mixvbox, vpw->nbpage4);
	g_signal_connect(GTK_NOTEBOOK(vpw->notebook), "switch-page", G_CALLBACK(page_switched), (void*)vpw);
	//gtk_container_add(GTK_CONTAINER(vpw->playerbox), vpw->notebook);
	gtk_box_pack_start(GTK_BOX(vpw->playerbox), vpw->notebook, TRUE, TRUE, 0);

    vpw->statusbar = gtk_statusbar_new();
    vpw->context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(vpw->statusbar), "statusbar context");
    gtk_container_add(GTK_CONTAINER(vpw->playerbox), vpw->statusbar);
    gtk_statusbar_push(GTK_STATUSBAR(vpw->statusbar), vpw->context_id, "");

	if (vpw->vpvisible)
		gtk_widget_show_all(vpw->vpwindow);
	else
		gtk_widget_hide(vpw->vpwindow);

/*
// create a new window for AV Parameters
    vpw->windowparm = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(vpw->windowparm), GTK_WIN_POS_CENTER);
    g_signal_connect(vpw->windowparm, "delete-event", G_CALLBACK(delete_event_parm), (void*)plp);
	g_signal_connect(vpw->windowparm, "realize", G_CALLBACK(realize_parm), (void*)plp);

    gtk_container_set_border_width(GTK_CONTAINER(vpw->windowparm), 2);
	//gtk_widget_set_size_request(windowparm, 100, 100);
	gtk_window_set_title(GTK_WINDOW(vpw->windowparm), "AV Parameters");
	gtk_window_set_resizable(GTK_WINDOW(vpw->windowparm), FALSE);

// vertical box
    vpw->parmvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(vpw->windowparm), vpw->parmvbox);

// horizontal box 1
	vpw->parmhbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(vpw->parmvbox), vpw->parmhbox1);
// audio queue max length
	vpw->parmlabel11 = gtk_label_new("AQ Length");
	gtk_widget_set_size_request(vpw->parmlabel11, 170, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhbox1), vpw->parmlabel11);
	vpw->parmlabel12 = gtk_label_new("0");
	gtk_widget_set_size_request(vpw->parmlabel12, 30, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhbox1), vpw->parmlabel12);
	vpw->spinbutton1 = gtk_spin_button_new_with_range(10.0, 100.0, 1.0);
	gtk_widget_set_size_request(vpw->spinbutton1, 120, 30);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(vpw->spinbutton1), plp->aqMaxLength);
	g_signal_connect(GTK_SPIN_BUTTON(vpw->spinbutton1), "value-changed", G_CALLBACK(aqmaxlength_changed), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->parmhbox1), vpw->spinbutton1);

// horizontal box 2
	vpw->parmhbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(vpw->parmvbox), vpw->parmhbox2);
// video queue max length
	vpw->parmlabel21 = gtk_label_new("VQ Length");
	gtk_widget_set_size_request(vpw->parmlabel21, 170, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhbox2), vpw->parmlabel21);
	vpw->parmlabel22 = gtk_label_new("0");
	gtk_widget_set_size_request(vpw->parmlabel22, 30, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhbox2), vpw->parmlabel22);
	vpw->spinbutton2 = gtk_spin_button_new_with_range(5.0, 50.0, 1.0);
	gtk_widget_set_size_request(vpw->spinbutton2, 120, 30);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(vpw->spinbutton2), plp->vqMaxLength);
	g_signal_connect(GTK_SPIN_BUTTON(vpw->spinbutton2), "value-changed", G_CALLBACK(vqmaxlength_changed), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->parmhbox2), vpw->spinbutton2);

// horizontal box 3
	vpw->parmhbox3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(vpw->parmvbox), vpw->parmhbox3);
// thread count
	vpw->parmlabel3 = gtk_label_new("Thread Count");
	gtk_widget_set_size_request(vpw->parmlabel3, 200, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhbox3), vpw->parmlabel3);
	vpw->spinbutton3 = gtk_spin_button_new_with_range (1.0, 8.0, 1.0);
	gtk_widget_set_size_request(vpw->spinbutton3, 120, 30);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(vpw->spinbutton3), plp->thread_count);
	g_signal_connect(GTK_SPIN_BUTTON(vpw->spinbutton3), "value-changed", G_CALLBACK(threadcount_changed), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->parmhbox3), vpw->spinbutton3);


//// horizontal box 4
//	parmhbox4 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
//	gtk_container_add(GTK_CONTAINER(parmvbox), parmhbox4);
//// player width
//	parmlabel4 = gtk_label_new("Player Width");
//	gtk_widget_set_size_request(parmlabel4, 200, 30);
//	gtk_container_add(GTK_CONTAINER(parmhbox4), parmlabel4);
//	spinbutton4 = gtk_spin_button_new_with_range (320.0, 1920.0, 80.0);
//	gtk_widget_set_size_request(spinbutton4, 120, 30);
//	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton4), playerWidth);
//	g_signal_connect(GTK_SPIN_BUTTON(spinbutton4), "value-changed", G_CALLBACK(playerwidth_changed), NULL);
//	gtk_container_add(GTK_CONTAINER(parmhbox4), spinbutton4);

// levelbars
	vpw->parmhlevel1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(vpw->parmvbox), vpw->parmhlevel1);
	vpw->levellabel1 = gtk_label_new("Read Frame");
	gtk_widget_set_size_request(vpw->levellabel1, 200, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhlevel1), vpw->levellabel1);
	vpw->levelbar1 = gtk_level_bar_new_for_interval(0.0, 1000.0);
	gtk_level_bar_set_value(GTK_LEVEL_BAR(vpw->levelbar1), 0.0);
	gtk_widget_set_size_request(vpw->levelbar1, 100, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhlevel1), vpw->levelbar1);
	vpw->label1 = gtk_label_new("0");
	gtk_widget_set_size_request(vpw->label1, 50, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhlevel1), vpw->label1);

	vpw->parmhlevel2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(vpw->parmvbox), vpw->parmhlevel2);
	vpw->levellabel2 = gtk_label_new("Decode");
	gtk_widget_set_size_request(vpw->levellabel2, 200, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhlevel2), vpw->levellabel2);
	vpw->levelbar2 = gtk_level_bar_new_for_interval(0.0, 10000.0);
	gtk_level_bar_set_value(GTK_LEVEL_BAR(vpw->levelbar2), 0.0);
	gtk_widget_set_size_request(vpw->levelbar2, 100, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhlevel2), vpw->levelbar2);
	vpw->label2 = gtk_label_new("0");
	gtk_widget_set_size_request(vpw->label2, 50, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhlevel2), vpw->label2);

	vpw->parmhlevel3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(vpw->parmvbox), vpw->parmhlevel3);
	vpw->levellabel3 = gtk_label_new("TexImage2D");
	gtk_widget_set_size_request(vpw->levellabel3, 200, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhlevel3), vpw->levellabel3);
	vpw->levelbar3 = gtk_level_bar_new_for_interval(0.0, 30000.0);
	gtk_level_bar_set_value(GTK_LEVEL_BAR(vpw->levelbar3), 0.0);
	gtk_widget_set_size_request(vpw->levelbar3, 100, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhlevel3), vpw->levelbar3);
	vpw->label3 = gtk_label_new("0");
	gtk_widget_set_size_request(vpw->label3, 50, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhlevel3), vpw->label3);

	vpw->parmhlevel4 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(vpw->parmvbox), vpw->parmhlevel4);
	vpw->levellabel4 = gtk_label_new("Draw");
	gtk_widget_set_size_request(vpw->levellabel4, 200, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhlevel4), vpw->levellabel4);
	vpw->levelbar4 = gtk_level_bar_new_for_interval(0.0, 1000.0);
	gtk_level_bar_set_value(GTK_LEVEL_BAR(vpw->levelbar4), 0.0);
	gtk_widget_set_size_request(vpw->levelbar4, 100, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhlevel4), vpw->levelbar4);
	vpw->label4 = gtk_label_new("0");
	gtk_widget_set_size_request(vpw->label4, 50, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhlevel4), vpw->label4);

////
//	parmhlevel5 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
//	gtk_container_add(GTK_CONTAINER(parmvbox), parmhlevel5);
//	levellabel5 = gtk_label_new("Read Pixels");
//	gtk_widget_set_size_request(levellabel5, 200, 30);
//	gtk_container_add(GTK_CONTAINER(parmhlevel5), levellabel5);
//	levelbar5 = gtk_level_bar_new_for_interval(0.0, 20000.0);
//	gtk_level_bar_set_value(GTK_LEVEL_BAR(levelbar5), 0.0);
//	gtk_widget_set_size_request(levelbar5, 100, 30);
//	gtk_container_add(GTK_CONTAINER(parmhlevel5), levelbar5);
//	label5 = gtk_label_new("0");
//	gtk_widget_set_size_request(label5, 50, 30);
//	gtk_container_add(GTK_CONTAINER(parmhlevel5), label5);

//	parmhlevel6 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
//	gtk_container_add(GTK_CONTAINER(parmvbox), parmhlevel6);
//	levellabel6 = gtk_label_new("Cairo");
//	gtk_widget_set_size_request(levellabel6, 200, 30);
//	gtk_container_add(GTK_CONTAINER(parmhlevel6), levellabel6);
//	levelbar6 = gtk_level_bar_new_for_interval(0.0, 20000.0);
//	gtk_level_bar_set_value(GTK_LEVEL_BAR(levelbar6), 0.0);
//	gtk_widget_set_size_request(levelbar6, 100, 30);
//	gtk_container_add(GTK_CONTAINER(parmhlevel6), levelbar6);
//	label6 = gtk_label_new("0");
//	gtk_widget_set_size_request(label6, 50, 30);
//	gtk_container_add(GTK_CONTAINER(parmhlevel6), label6);

	vpw->parmhlevel7 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(vpw->parmvbox), vpw->parmhlevel7);
	vpw->levellabel7 = gtk_label_new("Frame Drop");
	gtk_widget_set_size_request(vpw->levellabel7, 200, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhlevel7), vpw->levellabel7);
	vpw->levelbar7 = gtk_level_bar_new_for_interval(0.0, 100.0);
	gtk_level_bar_set_value(GTK_LEVEL_BAR(vpw->levelbar7), 0.0);
	gtk_widget_set_size_request(vpw->levelbar7, 100, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhlevel7), vpw->levelbar7);
	vpw->label7 = gtk_label_new("0 %");
	gtk_widget_set_size_request(vpw->label7, 50, 30);
	gtk_container_add(GTK_CONTAINER(vpw->parmhlevel7), vpw->label7);
*/
}

void close_videoplayerwidgets(vpwidgets *vpw)
{
	AudioEqualizer_close(&(vpw->ae));
	gtk_widget_destroy(vpw->vpwindow);
}

void press_vp_stop_button(playlistparams *plp)
{
	vpwidgets *vpw = plp->vpw;
	videoplayer *vp = &(vpw->vp);

	if (vp->vpq.playerstatus!=IDLE)
	{
		button2_clicked(vpw->button2, plp);
	}
}

void press_vp_resume_button(playlistparams *plp)
{
	vpwidgets *vpw = plp->vpw;
	videoplayer *vp = &(vpw->vp);

	if (vp->vpq.playerstatus==PAUSED)
	{
		button10_clicked(vpw->button10, plp);
	}
}

gboolean setnotebooktab1(gpointer data)
{
	vpwidgets *vpw = (vpwidgets *)data;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(vpw->notebook), 1);
	return FALSE;
}

void vpw_commandline(playlistparams *plp, int argcount)
{
	vpwidgets *vpw = plp->vpw;

	if (argcount>1)
	{
		GtkTreePath *tp = gtk_tree_path_new_from_string("0");
		GtkTreeSelection *ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(vpw->listview));
		gtk_tree_selection_select_path(ts, tp);
		listview_onRowActivated(GTK_TREE_VIEW(vpw->listview), tp, NULL, plp);
		gtk_tree_path_free(tp);

		//gdk_threads_add_idle(setnotebooktab1, vpw);
	}
}

gpointer singleinstancethread(gpointer args)
{
	int ctype = PTHREAD_CANCEL_ASYNCHRONOUS;
	int ctype_old;
	pthread_setcanceltype(ctype, &ctype_old);

	playlistparams *plp = (playlistparams *)args;
	vpwidgets *vpw = plp->vpw;
	videoplayer *vp = &(vpw->vp);
	videoplayerqueue *vpq = &(vp->vpq);
	instancesock *s = &(vpw->sisocket);

	struct ifaddrs *addrs, *tmp;
	char *buffer;
	wchar_t *wcharbuf;
	wchar_t *filename;
	int readpos, writepos, bytesread, bytestoread, totalbytesread, wcharsread, err;
	int itemcount, i;
	char *path;

	s->sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (s->sockfd < 0)
	{
		wprintf(L"Error opening socket\n");
	}
	else
	{
		memset((char *)&(s->serv_addr), 0, sizeof(s->serv_addr));

		s->serv_addr.sin_family = AF_INET;
		s->serv_addr.sin_addr.s_addr = INADDR_ANY;
		s->serv_addr.sin_port = htons(s->portno);
		if (bind(s->sockfd, (struct sockaddr *)&(s->serv_addr), sizeof(s->serv_addr)) < 0)
		{
			// Multiple instance
			s->single = 0;

			pthread_mutex_lock(s->simutex);
			s->binddone = 1;
			pthread_cond_signal(s->sicond);
			pthread_mutex_unlock(s->simutex);

			// Pass parameters to the other instance
			getifaddrs(&addrs);
			tmp = addrs;
			while (tmp) 
			{
				if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
				{
					struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
					//wprintf(L"%s: %s\n", tmp->ifa_name, inet_ntoa(pAddr->sin_addr));

					bzero((char *)&(s->serv_addr), sizeof(s->serv_addr));
					s->serv_addr.sin_family = AF_INET;
					bcopy((char *)&(pAddr->sin_addr), (char *)&(s->serv_addr.sin_addr.s_addr), sizeof(pAddr->sin_addr));
					s->serv_addr.sin_port = htons(s->portno);
					if (connect(s->sockfd, (struct sockaddr *)&(s->serv_addr), sizeof(s->serv_addr)) < 0)
					{
						perror("connect error");
						//wprintf(L"Error connecting to %s: %s\n", tmp->ifa_name, inet_ntoa(s->serv_addr.sin_addr));
					}
					else // send
					{
						//wprintf(L"connected to %s: %s port %d\n", tmp->ifa_name, inet_ntoa(s->serv_addr.sin_addr), ntohs(s->serv_addr.sin_port));

						//wprintf(L"argc %d\n", s->argc);
						filename = malloc(s->pathlength*sizeof(wchar_t));

						for(readpos=1;readpos<s->argc;readpos++)
						{
							wcharsread = mbstowcs(filename, s->argv[readpos], s->pathlength);
							//wprintf(L"%ls converted %d length %d\n", filename, wcharsread, (int)wcslen(filename));

							if ((bytesread=write(s->sockfd, filename, ((int)wcslen(filename) + 1)*sizeof(wchar_t)))>0)
							{
								//wprintf(L"written %d bytes\n", bytesread);
							}
							else
							{
								wprintf(L"Error writing to socket\n");
							}
						}

						free(filename);
						break;
					}

				}
				tmp = tmp->ifa_next;
			}
			freeifaddrs(addrs);

			close(s->sockfd);
		}
		else
		{
			// Single instance
			s->single = 1;

			pthread_mutex_lock(s->simutex);
			s->binddone = 1;
			pthread_cond_signal(s->sicond);
			pthread_mutex_unlock(s->simutex);

			listen(s->sockfd, s->num_pending_connections);
			s->clilen = sizeof(s->cli_addr);

			while (1)
			{
				s->newsockfd = accept(s->sockfd, (struct sockaddr *)&(s->cli_addr), &(s->clilen));
				if (s->newsockfd < 0)
				{
					wprintf(L"Error on accept\n");
				}
				else
				{
					//getpeername(s->newsockfd, (struct sockaddr *)&(s->cli_addr), &(s->clilen));
					//wprintf(L"client connected %s port %d\n", (char *)inet_ntoa(s->cli_addr.sin_addr), ntohs(s->serv_addr.sin_port));
					path = malloc(s->pathlength);
					filename = malloc(s->pathlength*sizeof(wchar_t));
					buffer = malloc(s->buffersize);
					wcharbuf = (wchar_t *)buffer;
					writepos = readpos = totalbytesread = 0;
					err = 0;
					do
					{
						bytestoread = s->buffersize - totalbytesread;
						if ((bytesread=read(s->newsockfd, buffer+totalbytesread, bytestoread))>0)
						{
							//wprintf(L"read %d bytes\n", bytesread);
							totalbytesread += bytesread;
							for(wcharsread = (totalbytesread - readpos*sizeof(wchar_t)) / sizeof(wchar_t);wcharsread;wcharsread--)
							{
								//wprintf(L"writing to pos %d\n", writepos);
								if ((filename[writepos++]=wcharbuf[readpos++]))
								{
									if (writepos>=s->pathlength)
									{
										//wprintf(L"path longer than %d \n", pathlength);
										bytesread = -1; writepos = readpos = totalbytesread = 0;
										err = 1;
										break;
									}
								}
								else
								{
									//wprintf(L"%ls\n", filename);
									i = wcstombs(path, filename, s->pathlength);
									itemcount = i = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(vpw->store), NULL); // rows
									if (nomediafile(path))
									{}
									else
									{
										gtk_list_store_append(vpw->store, &(vpw->iter));
										gtk_list_store_set(vpw->store, &(vpw->iter), COL_ID, i++, COL_FILEPATH, path, -1);
									}

									if (!itemcount)
									{
										if (vpq->playerstatus==PLAYING)
											button2_clicked(vpw->button2, (void*)plp);
										if (vpq->playerstatus==IDLE)
										{
											if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(vpw->store), &(vpw->iter), NULL, 0))
											{
												if (vp->now_playing)
												{
													g_free(vp->now_playing);
													vp->now_playing = NULL;
												}
												gtk_tree_model_get(GTK_TREE_MODEL(vpw->store), &(vpw->iter), COL_FILEPATH, &(vp->now_playing), -1);
												gdk_threads_add_idle(focus_iter_idle, (void*)vpw);
												gtk_notebook_set_current_page(GTK_NOTEBOOK(vpw->notebook), 0);
												button1_clicked(vpw->button1, (void*)plp);
											}
										}
									}

									writepos = 0;
								}
							}
							readpos %= ( s->buffersize / sizeof(wchar_t));
							totalbytesread %= s->buffersize;
						}
						else
							break;
					}while(!err);

					free(buffer);
					free(filename);
					free(path);
				}
				close(s->newsockfd);
			}
		}
	}

	s->retval = 0;
	pthread_exit(&(s->retval));
}

int singleinstance(playlistparams *plp, int portno, int argc, char **argv)
{
	vpwidgets *vpw = plp->vpw;
	instancesock *s = &(vpw->sisocket);
	int ret;

	s->portno = portno;
	s->argc = argc;
	s->argv = argv;
	s->binddone = 0;
	s->num_pending_connections = 5;
	s->buffersize = 256;
	s->pathlength = 2048;
	s->simutex = malloc(sizeof(pthread_mutex_t));
	if((ret=pthread_mutex_init(s->simutex, NULL))!=0 )
		wprintf(L"single instance mutex init failed, %d\n", ret);
	s->sicond = malloc(sizeof(pthread_cond_t));
	if((ret=pthread_cond_init(s->sicond, NULL))!=0 )
		wprintf(L"single instance cond init failed, %d\n", ret);

	ret = pthread_create(&(s->tid), NULL, &singleinstancethread, plp);
	if (ret)
	{}

	pthread_mutex_lock(s->simutex);
	while (!s->binddone)
		pthread_cond_wait(s->sicond, s->simutex);
	pthread_mutex_unlock(s->simutex);

	if (s->single)
	{
	}
	else
	{
		if ((ret=pthread_join(s->tid, NULL)))
			wprintf(L"pthread_join error, %d\n", ret);
	}

	pthread_mutex_destroy(s->simutex);
	free(s->simutex);

	pthread_cond_destroy(s->sicond);
	free(s->sicond);

	return(s->single);
}
