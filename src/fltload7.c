/* a vips7 wrapper for fltload
 */

#include <vips/vips.h>
#include <vips/vips7compat.h>

#include "fltload.h"

int
im_flt2vips(const char *filename, IMAGE *out)
{
	VipsImage *t;

	if (vips_fltload(filename, &t, NULL))
		return -1;
	if (vips_image_write(t, out)) {
		g_object_unref(t);
		return -1;
	}
	g_object_unref(t);

	return 0;
}

static int
isflt(const char *filename)
{
	return vips_foreign_is_a("fltload", filename);
}

static VipsFormatFlags
flt_flags(const char *filename)
{
	return (VipsFormatFlags) vips_foreign_flags("fltload", filename);
}

typedef VipsFormat VipsFormatFlt;
typedef VipsFormatClass VipsFormatFltClass;

static void
vips_format_flt_class_init(VipsFormatFltClass *class)
{
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsFormatClass *format_class = (VipsFormatClass *) class;

	object_class->nickname = "flt";
	object_class->description = _("load FLT volume");

	format_class->is_a = isflt;
	format_class->load = im_flt2vips;
	format_class->get_flags = flt_flags;
	format_class->suffs = vips_foreign_load_flt_suffs;
}

static void
vips_format_flt_init(VipsFormatFlt *object)
{
}

G_DEFINE_TYPE(VipsFormatFlt, vips_format_flt, VIPS_TYPE_FORMAT);
