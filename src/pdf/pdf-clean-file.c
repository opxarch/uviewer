#include "uview/pdf.h"

typedef struct globals_s
{
	pdf_document *doc;
	fz_context *ctx;
} globals;

static int
string_in_names_list(fz_context *ctx, pdf_obj *p, pdf_obj *names_list)
{
	int n = pdf_array_len(ctx, names_list);
	int i;
	char *str = pdf_to_str_buf(ctx, p);

	for (i = 0; i < n ; i += 2)
	{
		if (!strcmp(pdf_to_str_buf(ctx, pdf_array_get(ctx, names_list, i)), str))
			return 1;
	}
	return 0;
}

/*
 * Recreate page tree to only retain specified pages.
 */

static void retainpage(fz_context *ctx, pdf_document *doc, pdf_obj *parent, pdf_obj *kids, int page)
{
	pdf_obj *pageref = pdf_lookup_page_obj(ctx, doc, page-1);
	pdf_obj *pageobj = pdf_resolve_indirect(ctx, pageref);

	pdf_dict_put(ctx, pageobj, PDF_NAME_Parent, parent);

	/* Store page object in new kids array */
	pdf_array_push(ctx, kids, pageref);
}

static int dest_is_valid_page(fz_context *ctx, pdf_obj *obj, int *page_object_nums, int pagecount)
{
	int i;
	int num = pdf_to_num(ctx, obj);

	if (num == 0)
		return 0;
	for (i = 0; i < pagecount; i++)
	{
		if (page_object_nums[i] == num)
			return 1;
	}
	return 0;
}

static int dest_is_valid(fz_context *ctx, pdf_obj *o, int page_count, int *page_object_nums, pdf_obj *names_list)
{
	pdf_obj *p;

	p = pdf_dict_get(ctx, o, PDF_NAME_A);
	if (pdf_name_eq(ctx, pdf_dict_get(ctx, p, PDF_NAME_S), PDF_NAME_GoTo) &&
		!string_in_names_list(ctx, pdf_dict_get(ctx, p, PDF_NAME_D), names_list))
		return 0;

	p = pdf_dict_get(ctx, o, PDF_NAME_Dest);
	if (p == NULL)
	{}
	else if (pdf_is_string(ctx, p))
	{
		return string_in_names_list(ctx, p, names_list);
	}
	else if (!dest_is_valid_page(ctx, pdf_array_get(ctx, p, 0), page_object_nums, page_count))
		return 0;

	return 1;
}

static int strip_outlines(fz_context *ctx, pdf_document *doc, pdf_obj *outlines, int page_count, int *page_object_nums, pdf_obj *names_list);

static int strip_outline(fz_context *ctx, pdf_document *doc, pdf_obj *outlines, int page_count, int *page_object_nums, pdf_obj *names_list, pdf_obj **pfirst, pdf_obj **plast)
{
	pdf_obj *prev = NULL;
	pdf_obj *first = NULL;
	pdf_obj *current;
	int count = 0;

	for (current = outlines; current != NULL; )
	{
		int nc;

		/* Strip any children to start with. This takes care of
		 * First/Last/Count for us. */
		nc = strip_outlines(ctx, doc, current, page_count, page_object_nums, names_list);

		if (!dest_is_valid(ctx, current, page_count, page_object_nums, names_list))
		{
			if (nc == 0)
			{
				/* Outline with invalid dest and no children. Drop it by
				 * pulling the next one in here. */
				pdf_obj *next = pdf_dict_get(ctx, current, PDF_NAME_Next);
				if (next == NULL)
				{
					/* There is no next one to pull in */
					if (prev != NULL)
						pdf_dict_del(ctx, prev, PDF_NAME_Next);
				}
				else if (prev != NULL)
				{
					pdf_dict_put(ctx, prev, PDF_NAME_Next, next);
					pdf_dict_put(ctx, next, PDF_NAME_Prev, prev);
				}
				else
				{
					pdf_dict_del(ctx, next, PDF_NAME_Prev);
				}
				current = next;
			}
			else
			{
				/* Outline with invalid dest, but children. Just drop the dest. */
				pdf_dict_del(ctx, current, PDF_NAME_Dest);
				pdf_dict_del(ctx, current, PDF_NAME_A);
				current = pdf_dict_get(ctx, current, PDF_NAME_Next);
			}
		}
		else
		{
			/* Keep this one */
			if (first == NULL)
				first = current;
			prev = current;
			current = pdf_dict_get(ctx, current, PDF_NAME_Next);
			count++;
		}
	}

	*pfirst = first;
	*plast = prev;

	return count;
}

static int strip_outlines(fz_context *ctx, pdf_document *doc, pdf_obj *outlines, int page_count, int *page_object_nums, pdf_obj *names_list)
{
	int nc;
	pdf_obj *first;
	pdf_obj *last;

	first = pdf_dict_get(ctx, outlines, PDF_NAME_First);
	if (first == NULL)
		nc = 0;
	else
		nc = strip_outline(ctx, doc, first, page_count, page_object_nums, names_list, &first, &last);

	if (nc == 0)
	{
		pdf_dict_del(ctx, outlines, PDF_NAME_First);
		pdf_dict_del(ctx, outlines, PDF_NAME_Last);
		pdf_dict_del(ctx, outlines, PDF_NAME_Count);
	}
	else
	{
		int old_count = pdf_to_int(ctx, pdf_dict_get(ctx, outlines, PDF_NAME_Count));
		pdf_dict_put(ctx, outlines, PDF_NAME_First, first);
		pdf_dict_put(ctx, outlines, PDF_NAME_Last, last);
		pdf_dict_put(ctx, outlines, PDF_NAME_Count, pdf_new_int(ctx, doc, old_count > 0 ? nc : -nc));
	}

	return nc;
}

static void retainpages(fz_context *ctx, globals *glo, int argc, char **argv)
{
	pdf_obj *oldroot, *root, *pages, *kids, *countobj, *parent, *olddests;
	pdf_document *doc = glo->doc;
	int argidx = 0;
	pdf_obj *names_list = NULL;
	pdf_obj *outlines;
	int pagecount;
	int i;
	int *page_object_nums;

	/* Keep only pages/type and (reduced) dest entries to avoid
	 * references to unretained pages */
	oldroot = pdf_dict_get(ctx, pdf_trailer(ctx, doc), PDF_NAME_Root);
	pages = pdf_dict_get(ctx, oldroot, PDF_NAME_Pages);
	olddests = pdf_load_name_tree(ctx, doc, PDF_NAME_Dests);
	outlines = pdf_dict_get(ctx, oldroot, PDF_NAME_Outlines);

	root = pdf_new_dict(ctx, doc, 3);
	pdf_dict_put(ctx, root, PDF_NAME_Type, pdf_dict_get(ctx, oldroot, PDF_NAME_Type));
	pdf_dict_put(ctx, root, PDF_NAME_Pages, pdf_dict_get(ctx, oldroot, PDF_NAME_Pages));
	pdf_dict_put(ctx, root, PDF_NAME_Outlines, outlines);

	pdf_update_object(ctx, doc, pdf_to_num(ctx, oldroot), root);

	/* Create a new kids array with only the pages we want to keep */
	parent = pdf_new_indirect(ctx, doc, pdf_to_num(ctx, pages), pdf_to_gen(ctx, pages));
	kids = pdf_new_array(ctx, doc, 1);

	/* Retain pages specified */
	while (argc - argidx)
	{
		int page, spage, epage;
		char *spec, *dash;
		char *pagelist = argv[argidx];

		pagecount = pdf_count_pages(ctx, doc);
		spec = fz_strsep(&pagelist, ",");
		while (spec)
		{
			dash = strchr(spec, '-');

			if (dash == spec)
				spage = epage = pagecount;
			else
				spage = epage = atoi(spec);

			if (dash)
			{
				if (strlen(dash) > 1)
					epage = atoi(dash + 1);
				else
					epage = pagecount;
			}

			spage = fz_clampi(spage, 1, pagecount);
			epage = fz_clampi(epage, 1, pagecount);

			if (spage < epage)
				for (page = spage; page <= epage; ++page)
					retainpage(ctx, doc, parent, kids, page);
			else
				for (page = spage; page >= epage; --page)
					retainpage(ctx, doc, parent, kids, page);

			spec = fz_strsep(&pagelist, ",");
		}

		argidx++;
	}

	pdf_drop_obj(ctx, parent);

	/* Update page count and kids array */
	countobj = pdf_new_int(ctx, doc, pdf_array_len(ctx, kids));
	pdf_dict_put(ctx, pages, PDF_NAME_Count, countobj);
	pdf_drop_obj(ctx, countobj);
	pdf_dict_put(ctx, pages, PDF_NAME_Kids, kids);
	pdf_drop_obj(ctx, kids);

	/* Force the next call to pdf_count_pages to recount */
	glo->doc->page_count = 0;

	pagecount = pdf_count_pages(ctx, doc);
	page_object_nums = fz_calloc(ctx, pagecount, sizeof(*page_object_nums));
	for (i = 0; i < pagecount; i++)
	{
		pdf_obj *pageref = pdf_lookup_page_obj(ctx, doc, i);
		page_object_nums[i] = pdf_to_num(ctx, pageref);
	}

	/* If we had an old Dests tree (now reformed as an olddests
	 * dictionary), keep any entries in there that point to
	 * valid pages. This may mean we keep more than we need, but
	 * it's safe at least. */
	if (olddests)
	{
		pdf_obj *names = pdf_new_dict(ctx, doc, 1);
		pdf_obj *dests = pdf_new_dict(ctx, doc, 1);
		int len = pdf_dict_len(ctx, olddests);

		names_list = pdf_new_array(ctx, doc, 32);

		for (i = 0; i < len; i++)
		{
			pdf_obj *key = pdf_dict_get_key(ctx, olddests, i);
			pdf_obj *val = pdf_dict_get_val(ctx, olddests, i);
			pdf_obj *dest = pdf_dict_get(ctx, val, PDF_NAME_D);

			dest = pdf_array_get(ctx, dest ? dest : val, 0);
			if (dest_is_valid_page(ctx, dest, page_object_nums, pagecount))
			{
				pdf_obj *key_str = pdf_new_string(ctx, doc, pdf_to_name(ctx, key), strlen(pdf_to_name(ctx, key)));
				pdf_array_push(ctx, names_list, key_str);
				pdf_array_push(ctx, names_list, val);
				pdf_drop_obj(ctx, key_str);
			}
		}

		pdf_dict_put(ctx, dests, PDF_NAME_Names, names_list);
		pdf_dict_put(ctx, names, PDF_NAME_Dests, dests);
		pdf_dict_put(ctx, root, PDF_NAME_Names, names);

		pdf_drop_obj(ctx, names);
		pdf_drop_obj(ctx, dests);
		pdf_drop_obj(ctx, olddests);
	}

	/* Edit each pages /Annot list to remove any links that point to
	 * nowhere. */
	for (i = 0; i < pagecount; i++)
	{
		pdf_obj *pageref = pdf_lookup_page_obj(ctx, doc, i);
		pdf_obj *pageobj = pdf_resolve_indirect(ctx, pageref);

		pdf_obj *annots = pdf_dict_get(ctx, pageobj, PDF_NAME_Annots);

		int len = pdf_array_len(ctx, annots);
		int j;

		for (j = 0; j < len; j++)
		{
			pdf_obj *o = pdf_array_get(ctx, annots, j);

			if (!pdf_name_eq(ctx, pdf_dict_get(ctx, o, PDF_NAME_Subtype), PDF_NAME_Link))
				continue;

			if (!dest_is_valid(ctx, o, pagecount, page_object_nums, names_list))
			{
				/* Remove this annotation */
				pdf_array_delete(ctx, annots, j);
				j--;
			}
		}
	}

	if (strip_outlines(ctx, doc, outlines, pagecount, page_object_nums, names_list) == 0)
	{
		pdf_dict_del(ctx, root, PDF_NAME_Outlines);
	}

	fz_free(ctx, page_object_nums);
	pdf_drop_obj(ctx, names_list);
	pdf_drop_obj(ctx, root);
}

void pdf_clean_file(fz_context *ctx, char *infile, char *outfile, char *password, pdf_write_options *opts, char *argv[], int argc)
{
	globals glo = { 0 };

	glo.ctx = ctx;

	fz_try(ctx)
	{
		glo.doc = pdf_open_document(ctx, infile);
		if (pdf_needs_password(ctx, glo.doc))
			if (!pdf_authenticate_password(ctx, glo.doc, password))
				fz_throw(glo.ctx, FZ_ERROR_GENERIC, "cannot authenticate password: %s", infile);

		/* Only retain the specified subset of the pages */
		if (argc)
			retainpages(ctx, &glo, argc, argv);

		pdf_save_document(ctx, glo.doc, outfile, opts);
	}
	fz_always(ctx)
	{
		pdf_drop_document(ctx, glo.doc);
	}
	fz_catch(ctx)
	{
		if (opts && opts->errors)
			*opts->errors = *opts->errors+1;
	}
}
