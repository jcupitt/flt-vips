/* a vips7 wrapper for fltload
 */

#include <vips/vips.h>
#include <vips/vips7compat.h>

#include "fltload.h"

int
im_flt2vips(const char *filename, IMAGE *out)
{
	VipsSource *source;
	VipsImage *t;

	if (!(source = vips_source_new_from_file(filename)))
		return -1;

	if (vips_fltload_source(source, &t, NULL)) {
		g_object_unref(source);
		return -1;
	}

	g_object_unref(source);

	if (vips_image_write(t, out)) {
		g_object_unref(t);
		return -1;
	}

	g_object_unref(t);

	return 0;
}

static VipsFormatFlags
flt_flags(const char *filename)
{
	return (VipsFormatFlags) vips_foreign_flags("fltload", filename);
}

typedef VipsFormat VipsFormatFlt;
typedef VipsFormatClass VipsFormatFltClass;

static const char *flt_suffs[] = {
	".flt",
	NULL
};

static void
vips_format_flt_class_init(VipsFormatFltClass *class)
{
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsFormatClass *format_class = (VipsFormatClass *) class;

	object_class->nickname = "flt";
	object_class->description = _("load FLT volume");

	format_class->load = im_flt2vips;
	format_class->get_flags = flt_flags;
	format_class->suffs = flt_suffs;
}

static void
vips_format_flt_init(VipsFormatFlt *object)
{
}

G_DEFINE_TYPE(VipsFormatFlt, vips_format_flt, VIPS_TYPE_FORMAT);
