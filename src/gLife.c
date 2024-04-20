/*
 ============================================================================
 Name        : gLife.c
 Author      : gremlin
 Version     :
 Copyright   :
 Description : Hello World in GTK+
 ============================================================================
 */

#include <stdint.h>
#include <stdbool.h>
#include <omp.h>
#include <gtk/gtk.h>

#define WINSIZE 1250
#define WORLDSIZE 250
#define PAINTERS 5

GtkApplication *app;
uint8_t w[2][WORLDSIZE][WORLDSIZE];
unsigned long gen = 0;
volatile int done = 0;

GTimer *treal = NULL;
GTimer *tcalc = NULL;

struct mydata {
    GtkWidget *d;
    int n;
};

static gboolean tick(gpointer data)
{
    //static unsigned int count = 0;
    //static double timesum = 0.0;
    int i, j;
    int g = gen % 2;
    struct mydata *p = data;

    if (done > 0)
        return true;

    g_timer_start(tcalc);

#pragma omp target map(tofrom: w)
//#pragma omp parallel
    {
#pragma omp parallel for
//#pragma omp parallel for shared(w)
        for (j = 0; j < WORLDSIZE; ++j) {
            for (i = 0; i < WORLDSIZE; ++i) {
                uint16_t c = 0;

                c += w[g][(j - 1 + WORLDSIZE) % WORLDSIZE][(i - 1 + WORLDSIZE) % WORLDSIZE];
                c += w[g][(j + 0 + WORLDSIZE) % WORLDSIZE][(i - 1 + WORLDSIZE) % WORLDSIZE];
                c += w[g][(j + 1 + WORLDSIZE) % WORLDSIZE][(i - 1 + WORLDSIZE) % WORLDSIZE];
                c += w[g][(j + 1 + WORLDSIZE) % WORLDSIZE][(i + 0 + WORLDSIZE) % WORLDSIZE];
                c += w[g][(j + 1 + WORLDSIZE) % WORLDSIZE][(i + 1 + WORLDSIZE) % WORLDSIZE];
                c += w[g][(j + 0 + WORLDSIZE) % WORLDSIZE][(i + 1 + WORLDSIZE) % WORLDSIZE];
                c += w[g][(j - 1 + WORLDSIZE) % WORLDSIZE][(i + 1 + WORLDSIZE) % WORLDSIZE];
                c += w[g][(j - 1 + WORLDSIZE) % WORLDSIZE][(i + 0 + WORLDSIZE) % WORLDSIZE];

                switch (c) {
                case 0:
                case 1:
                    w[(gen+1) % 2][j][i] = 0;
                    break;
                case 2:
                    w[(gen+1) % 2][j][i] = w[g][j][i];
                    break;
                case 3:
                    w[(gen+1) % 2][j][i] = 1;
                    break;
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                    w[(gen+1) % 2][j][i] = 0;
                }
            }
        }
    }

    done = PAINTERS;
    for (int n=0; n < PAINTERS; ++n)
        gtk_widget_queue_draw((GtkWidget*) p[n].d);

    /*{
        /#if (omp_get_thread_num() == 0)#/ {
            cairo_region_t *cairoRegion = cairo_region_create();
            GdkDrawingContext *gdk_dc = gdk_window_begin_draw_frame(gtk_widget_get_window(p[0].d), cairoRegion);
            cairo_t *cr = gdk_drawing_context_get_cairo_context(gdk_dc);
            gtk_widget_draw(p[0].d, cr);
            gdk_window_end_draw_frame(gtk_widget_get_window(p[0].d), gdk_dc);
            cairo_region_destroy(cairoRegion);
        } /#else#/ {
            cairo_region_t *cairoRegion = cairo_region_create();
            GdkDrawingContext *gdk_dc = gdk_window_begin_draw_frame(gtk_widget_get_window(p[1].d), cairoRegion);
            cairo_t *cr = gdk_drawing_context_get_cairo_context(gdk_dc);
            gtk_widget_draw(p[1].d, cr);
            gdk_window_end_draw_frame(gtk_widget_get_window(p[1].d), gdk_dc);
            cairo_region_destroy(cairoRegion);
        }
    }
    ++gen;
    */

    //timesum += g_timer_elapsed(tcalc, NULL) * 1000.0;
    //++count;
    //if (g_timer_elapsed(treal, NULL) > 1.0) {
    //    printf("[%3lu] draw = %.3f\n", gen, timesum / count);
    //    timesum = 0.0;
    //    count = 0;
    //    g_timer_start(treal);
    //}
    //if (done) {
    //    ++gen;
    //    done = 0;
    //}
    if (gen > 1000) {
        g_message("bye bye");
        g_application_quit(G_APPLICATION(app));
        return false;
    } else {
        return true;
    }
}

cairo_surface_t *c1, *c0;

gboolean draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    static unsigned int count = 0;
    static double timesum = 0.0;
    struct mydata *p = data;
    guint i, j;
    guint width, /*height,*/ size, x, y, l;
    uint8_t *wo = (uint8_t*) w+(((gen + 1)% 2)*WORLDSIZE*WORLDSIZE);    // old
    uint8_t *wn = (uint8_t*) w+((gen % 2)*WORLDSIZE*WORLDSIZE);         // new

    //g_timer_start(tcalc);

    width = gtk_widget_get_allocated_width(widget);
    //height = gtk_widget_get_allocated_height(widget);

    GtkStyleContext *context;
    context = gtk_widget_get_style_context(widget);
    gtk_render_background(context, cr, 0, 0, width, width);

    size = width;
    l = size / WORLDSIZE;

    guint offset = p->n * (WORLDSIZE / PAINTERS) * (WORLDSIZE);

    for (i = 0; i < WORLDSIZE; ++i) {
        for (j = 0; j < WORLDSIZE / PAINTERS; ++j) {
            x = l * i;
            y = l * j;
            //if (wn[offset + j * WORLDSIZE + i] != wo[offset + j * WORLDSIZE + i]){
                if (wn[offset + j * WORLDSIZE + i])
                    cairo_set_source_surface (cr, c1, x, y);
                else
                    cairo_set_source_surface (cr, c0, x, y);
                cairo_paint(cr);
            //}
        }
    }
    if ((--done) == 0) {
        timesum += g_timer_elapsed(tcalc, NULL) * 1000.0;
        ++count;
        if (g_timer_elapsed(treal, NULL) > 1.0) {
            printf("[%3lu] draw = %.3f\n", gen, timesum / count);
            timesum = 0.0;
            count = 0;
            g_timer_start(treal);
        }

        ++gen;
    }
    return TRUE;
}

static void start(GtkWidget* self, gpointer user_data)
{
    int width, /*height,*/ l, stride;
    int iw, ih;
    unsigned char *data;
    cairo_t *cr;
    cairo_surface_t *temp;

    width = gtk_widget_get_allocated_width(GTK_WIDGET(self));
    //height = gtk_widget_get_allocated_height(GTK_WIDGET(self));
    l = width / WORLDSIZE;
    printf("cell size %d / %d = %d\n", width, WORLDSIZE, l);
    stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, l);
    printf("cell stride = %d\n", stride);

    data = malloc(stride * l);
    c1 = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_RGB24, l, l, stride);
    cr = cairo_create(c1);
    temp = cairo_image_surface_create_from_png("1.png");
    iw = cairo_image_surface_get_width(temp);
    ih = cairo_image_surface_get_height(temp);
    cairo_scale(cr, 1.0 * l / iw, 1.0 * l / ih);
    cairo_set_source_surface(cr, temp, 0.0, 0.0);
    cairo_paint(cr);
    cairo_surface_flush(c1);
    cairo_surface_destroy(temp);
    cairo_destroy(cr);

    data = malloc(stride * l);
    c0 = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_RGB24, l, l, stride);
    cr = cairo_create(c0);
    temp = cairo_image_surface_create_from_png("0.png");
    iw = cairo_image_surface_get_width(temp);
    ih = cairo_image_surface_get_height(temp);
    cairo_scale(cr, 1.0 * l / iw, 1.0 * l / ih);
    cairo_set_source_surface(cr, temp, 0.0, 0.0);
    cairo_paint(cr);
    cairo_surface_flush(c0);
    cairo_surface_destroy(temp);
    cairo_destroy(cr);

    treal = g_timer_new();
    tcalc = g_timer_new();
    g_timeout_add_full(G_PRIORITY_HIGH, 10, &tick, user_data, NULL);
}

static void activate(GtkApplication *app, __attribute__ ((unused)) gpointer user_data)
{
    GtkWidget *window;
    struct mydata *p;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "OpenMP Life");
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    p = malloc(sizeof(struct mydata) * PAINTERS);


    for (int n=0; n < PAINTERS; ++n) {
        GtkWidget *drawing_area = gtk_drawing_area_new();
        //gtk_widget_set_name(drawing_area, "d0");
        p[n].d = drawing_area;
        p[n].n = n;
        gtk_widget_set_size_request(drawing_area, WINSIZE, WINSIZE / PAINTERS);
        g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK (draw_callback), &(p[n]));
        g_signal_connect(G_OBJECT(drawing_area), "map", G_CALLBACK(start), p);
        gtk_box_pack_start(GTK_BOX(box), drawing_area, false, true, 0);
    }
//    drawing_area = gtk_drawing_area_new();
//    gtk_widget_set_name(drawing_area, "d1");
//    p[1].d = drawing_area;
//    p[1].n = 1;
//    gtk_widget_set_size_request(drawing_area, WORLDSIZE, WORLDSIZE / PAINTERS);
//    g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK (draw_callback), &(p[1]));
//    g_signal_connect(G_OBJECT(drawing_area), "map", G_CALLBACK(start), p);
//    gtk_box_pack_start(GTK_BOX(box), drawing_area, false, true, 0);
//
//    drawing_area = gtk_drawing_area_new();
//    gtk_widget_set_name(drawing_area, "d2");
//    p[2].d = drawing_area;
//    p[2].n = 2;
//    gtk_widget_set_size_request(drawing_area, WORLDSIZE, WORLDSIZE / PAINTERS);
//    g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK (draw_callback), &(p[2]));
//    g_signal_connect(G_OBJECT(drawing_area), "map", G_CALLBACK(start), p);
//    gtk_box_pack_start(GTK_BOX(box), drawing_area, false, true, 0);

//    drawing_area = gtk_drawing_area_new();
//    gtk_widget_set_name(drawing_area, "two");
//    p[3].d = drawing_area;
//    p[3].n = 3;
//    gtk_widget_set_size_request(drawing_area, 1200, 1200 / 4);
//    g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK (draw_callback), &(p[3]));
//    g_signal_connect(G_OBJECT(drawing_area), "map", G_CALLBACK(start), p);
//    gtk_box_pack_start(GTK_BOX(box), drawing_area, false, true, 0);

    gtk_container_add(GTK_CONTAINER(window), box);
    gtk_widget_show_all(window);
}

int main(int argc, char *argv[])
{
    int status;
    int i, j;

#pragma omp target
//#pragma omp parallel
    {
#pragma omp master
        {
            printf("Parallel %d\n", omp_get_max_threads());
        }
    }
    for (i = 0; i < WORLDSIZE; ++i) {
        for (j = 0; j < WORLDSIZE; ++j) {
            w[0][i][j] = (rand() % 2);
            w[1][i][j] = 99;
        }
    }

    //w[0][0][1] = 1;
    //w[0][0][2] = 1;
    //w[0][0][3] = 1;
    //w[0][1][0] = 1;
    //w[0][1][1] = 1;
    //w[0][1][2] = 1;

    app = gtk_application_new("it.gremlin.gopenmplife", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
