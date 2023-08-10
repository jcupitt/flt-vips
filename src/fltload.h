// i18n stub
#define _(S) (S)

extern const char *vips_foreign_load_flt_suffs[];

int vips_fltload_source(VipsSource *source, VipsImage **out, ...);

// the types of our loaders
GType vips_foreign_load_flt_get_type(void);
GType vips_format_flt_get_type(void);

extern const char *vips_foreign_load_flt_suffs[];
