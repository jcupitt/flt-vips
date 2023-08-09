#include <vips/vips.h>

#include "fltload.h"

/* This is called on module load.
 */
const gchar *
g_module_check_init(GModule *module)
{
	printf("vips_fltload: module init\n"); 

	vips_foreign_load_flt_get_type();
	vips_format_flt_get_type();

	/* We can't be unloaded, there would be chaos.
	 */
	g_module_make_resident(module);

	return NULL; 
}

