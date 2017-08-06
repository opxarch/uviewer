#include "uview/html.h"

static fz_font *
fz_load_html_default_font(fz_context *ctx, fz_html_font_set *set, const char *family, int is_bold, int is_italic)
{
	int is_mono = !strcmp(family, "monospace");
	int is_sans = !strcmp(family, "sans-serif");
	const char *real_family = is_mono ? "Courier" : is_sans ? "Helvetica" : "Charis SIL";
	const char *backup_family = is_mono ? "Courier" : is_sans ? "Helvetica" : "Times";
	int idx = (is_mono ? 8 : is_sans ? 4 : 0) + is_bold * 2 + is_italic;
	if (!set->fonts[idx])
	{
		const char *data;
		int size;

		data = fz_lookup_builtin_font(ctx, real_family, is_bold, is_italic, &size);
		if (!data)
			data = fz_lookup_builtin_font(ctx, backup_family, is_bold, is_italic, &size);
		if (!data)
			fz_throw(ctx, FZ_ERROR_GENERIC, "cannot load html font: %s", real_family);
		set->fonts[idx] = fz_new_font_from_memory(ctx, NULL, data, size, 0, 1);
		set->fonts[idx]->is_serif = !is_sans;
	}
	return set->fonts[idx];
}

void
fz_add_html_font_face(fz_context *ctx, fz_html_font_set *set,
	const char *family, int is_bold, int is_italic, const char *src,
	fz_font *font)
{
	fz_html_font_face *custom = fz_malloc_struct(ctx, fz_html_font_face);
	custom->font = fz_keep_font(ctx, font);
	custom->src = fz_strdup(ctx, src);
	custom->family = fz_strdup(ctx, family);
	custom->is_bold = is_bold;
	custom->is_italic = is_italic;
	custom->next = set->custom;
	set->custom = custom;
}

fz_font *
fz_load_html_font(fz_context *ctx, fz_html_font_set *set, const char *family, int is_bold, int is_italic)
{
	fz_html_font_face *custom;
	const char *data;
	int size;

	for (custom = set->custom; custom; custom = custom->next)
	{
		if (!strcmp(family, custom->family) &&
				is_bold == custom->is_bold &&
				is_italic == custom->is_italic)
		{
			return custom->font;
		}
	}

	data = fz_lookup_builtin_font(ctx, family, is_bold, is_italic, &size);
	if (data)
	{
		fz_font *font = fz_new_font_from_memory(ctx, NULL, data, size, 0, 0);
		if (is_bold && !font->is_bold)
			font->fake_bold = 1;
		if (is_italic && !font->is_italic)
			font->fake_italic = 1;
		fz_add_html_font_face(ctx, set, family, is_bold, is_italic, "<builtin>", font);
		fz_drop_font(ctx, font);
		return font;
	}

	if (!strcmp(family, "monospace") || !strcmp(family, "sans-serif") || !strcmp(family, "serif"))
		return fz_load_html_default_font(ctx, set, family, is_bold, is_italic);

	return NULL;
}

fz_html_font_set *fz_new_html_font_set(fz_context *ctx)
{
	return fz_malloc_struct(ctx, fz_html_font_set);
}

void fz_drop_html_font_set(fz_context *ctx, fz_html_font_set *set)
{
	fz_html_font_face *font, *next;
	int i;

	font = set->custom;
	while (font)
	{
		next = font->next;
		fz_drop_font(ctx, font->font);
		fz_free(ctx, font->src);
		fz_free(ctx, font->family);
		fz_free(ctx, font);
		font = next;
	}

	for (i = 0; i < nelem(set->fonts); ++i)
		fz_drop_font(ctx, set->fonts[i]);

	fz_free(ctx, set);
}
