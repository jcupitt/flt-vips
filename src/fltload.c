/* load a FLT volume
 *
 * 9/8/23
 * 	- from fltload.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <vips/vips.h>

#include "fltload.h"

typedef struct _VipsForeignLoadFlt {
	VipsForeignLoad parent_object;

	VipsSource *source;

	/* The directory we load from.
	 */
	char *dirname;

	/* Metadata we read from the ini file.
	 */
	gboolean ini_found;
	int width;
	int height;
	VipsBandFormat format;

	/* All the filenames we've found, sorted by slice number.
	 */
	GSList *slice_names;
	int n_slices;

} VipsForeignLoadFlt;

typedef VipsForeignLoadClass VipsForeignLoadFltClass;

G_DEFINE_TYPE(VipsForeignLoadFlt, vips_foreign_load_flt,
	VIPS_TYPE_FOREIGN_LOAD);

static void
vips_foreign_load_flt_dispose(GObject *gobject)
{
	VipsForeignLoadFlt *flt = (VipsForeignLoadFlt *) gobject;

	VIPS_FREE(flt->dirname);
	g_slist_free_full(flt->slice_names, g_free);

	G_OBJECT_CLASS(vips_foreign_load_flt_parent_class)->dispose(gobject);
}

static gboolean
vips_foreign_load_flt_is_a_source(VipsSource *source)
{
    VipsConnection *connection = VIPS_CONNECTION(source);

    const char *filename;

    return vips_source_is_file(source) &&
        (filename = vips_connection_filename(connection)) &&
		vips_iscasepostfix(filename, ".flt");
}

static VipsForeignFlags
vips_foreign_load_flt_get_flags(VipsForeignLoad *load)
{
	return VIPS_FOREIGN_PARTIAL;
}

static int
vips_foreign_load_flt_ini(VipsForeignLoadFlt *flt, const char *name) 
{
	char *path = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s", 
		flt->dirname, name);

	GKeyFile *key_file;
	GError *error = NULL;

	flt->ini_found = TRUE;

	key_file = g_key_file_new();
	if (!g_key_file_load_from_file(key_file, path, 0, &error)) {
		vips_g_error(&error);
		g_key_file_unref(key_file);
        return -1;
	}

	flt->width = g_key_file_get_integer(key_file, "main", "width", &error);
	if (error) {
		vips_g_error(&error);
		g_key_file_unref(key_file);
        return -1;
	}

	flt->height = g_key_file_get_integer(key_file, "main", "height", &error);
	if (error) {
		vips_g_error(&error);
		g_key_file_unref(key_file);
        return -1;
	}

	char *format;
	format = g_key_file_get_string(key_file, "main", "format", &error);
	if (error) {
		vips_g_error(&error);
		g_key_file_unref(key_file);
        return -1;
	}
	flt->format = vips_enum_from_nick("vips", VIPS_TYPE_BAND_FORMAT, format);
	g_free(format);

	if (flt->format < 0) {
		g_key_file_unref(key_file);
        return -1;
	}

	g_key_file_unref(key_file);

	return 0;
}

static int
vips_foreign_load_flt_header(VipsForeignLoad *load)
{
	VipsForeignLoadFlt *flt = (VipsForeignLoadFlt *) load;
    const char *filename = 
		vips_connection_filename(VIPS_CONNECTION(flt->source));

	GError *error = NULL;
	GDir *dir;
	const char *name;

	printf("vips_foreign_load_flt_header: loading %s ...\n", filename);

	flt->dirname = g_path_get_dirname(filename);
	if (!(dir = g_dir_open(flt->dirname, 0, &error))) {
		vips_g_error(&error);
        return -1;
	}

    while ((name = g_dir_read_name(dir)))
		if (vips_iscasepostfix(name, "info.flt")) {
			if (vips_foreign_load_flt_ini(flt, name))
				return -1;
		}
		else if (vips_iscasepostfix(name, ".flt")) {
			char *path = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s", 
				flt->dirname, name);

			flt->slice_names = g_slist_prepend(flt->slice_names, path);
		}

    g_dir_close(dir);

	if (!flt->ini_found) {
		vips_error("fltload", "%s", _("no ini file"));
		return -1;
	}

	flt->n_slices = g_slist_length(flt->slice_names);
	if (flt->n_slices == 0) {
		vips_error("fltload", "%s", _("no slices found"));
		return -1;
	}

	printf("vips_foreign_load_flt_header: found %d slices\n", flt->n_slices);

	flt->slice_names = g_slist_sort(flt->slice_names, 
		(GCompareFunc) g_ascii_strcasecmp);

	vips_image_init_fields(load->out,
		flt->width, 
		flt->height * flt->n_slices, 
		1,
		flt->format,
		VIPS_CODING_NONE, VIPS_INTERPRETATION_B_W, 1.0, 1.0);

	// these are mmaped files, so sequential read works best
	if (vips_image_pipelinev(load->out, VIPS_DEMAND_STYLE_THINSTRIP, NULL))
		return -1;

	// this will let eg. tiffsave save this as a many-page tiff
	vips_image_set_int(load->out, VIPS_META_PAGE_HEIGHT, flt->height);

	VIPS_SETSTR(load->out->filename, filename);

	return 0;
}

static int
vips_foreign_load_flt_load(VipsForeignLoad *load)
{
	VipsForeignLoadFlt *flt = (VipsForeignLoadFlt *) load;
	VipsImage **x = (VipsImage **) 
		vips_object_local_array(VIPS_OBJECT(load), flt->n_slices);
	VipsImage **t = (VipsImage **) 
		vips_object_local_array(VIPS_OBJECT(load), 2);

	GSList *p;
	int i;

	for (i = 0, p = flt->slice_names; i < flt->n_slices; i++, p = p->next) {
		const char *slice_name = (const char *) p->data;

		if (!(x[i] = vips_image_new_from_file_raw(slice_name, 
			flt->width, flt->height, vips_format_sizeof(flt->format), 0)))
			return -1;

		// force an immediate load (this will just mmap the file)
		if (vips_image_wio_input(x[i]))
			return -1;
	}

	// join vertically to make the volume
	if (vips_arrayjoin(x, &t[0], flt->n_slices, "across", 1, NULL))
		return -1;

	// turn into a 1 band image of the correct format
	if (vips_copy(t[0], &t[1],
            "format", flt->format,
            "bands", 1,
            NULL)) 
        return -1;

	if (vips_image_write(t[1], load->real)) 
		return -1;

	return 0;
}

static void
vips_foreign_load_flt_class_init(VipsForeignLoadFltClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(class);
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsOperationClass *operation_class = VIPS_OPERATION_CLASS(class);
	VipsForeignClass *foreign_class = (VipsForeignClass *) class;
	VipsForeignLoadClass *load_class = (VipsForeignLoadClass *) class;

	gobject_class->dispose = vips_foreign_load_flt_dispose;
	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	object_class->nickname = "fltload_source";
	object_class->description = _("load FLT volume");

	load_class->is_a_source = vips_foreign_load_flt_is_a_source;
	load_class->get_flags = vips_foreign_load_flt_get_flags;
	load_class->header = vips_foreign_load_flt_header;
	load_class->load = vips_foreign_load_flt_load;

	operation_class->flags |= VIPS_OPERATION_NOCACHE;

	VIPS_ARG_OBJECT(class, "source", 1,
        _("Source"),
        _("Source to load from"),
        VIPS_ARGUMENT_REQUIRED_INPUT,
        G_STRUCT_OFFSET(VipsForeignLoadFlt, source),
        VIPS_TYPE_SOURCE);

}

static void
vips_foreign_load_flt_init(VipsForeignLoadFlt *flt)
{
}

/**
 * vips_fltload_source:
 * @source: source to load from
 * @out: (out): output image
 * @...: %NULL-terminated list of optional named arguments
 *
 * Load a FLT volume as a vips image.
 *
 * Returns: 0 on success, -1 on error.
 */
int
vips_fltload_source(VipsSource *source, VipsImage **out, ...)
{
	va_list ap;
	int result;

	va_start(ap, out);
	result = vips_call_split("fltload_source", ap, source, out);
	va_end(ap);

	return result;
}
