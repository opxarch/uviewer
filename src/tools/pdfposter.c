/*
 * PDF posteriser; split pages within a PDF file into smaller lumps.
 */

#include "uview/pdf.h"

static int x_factor = 0;
static int y_factor = 0;

static void usage(void)
{
	fprintf(stderr,
		"usage: mutool poster [options] input.pdf [output.pdf]\n"
		"\t-p -\tpassword\n"
		"\t-x\tx decimation factor\n"
		"\t-y\ty decimation factor\n");
	exit(1);
}

static void
intersect_box(fz_context *ctx, pdf_document *doc, pdf_obj *page, pdf_obj *box_name, const fz_rect *mb)
{
	pdf_obj *box = pdf_dict_get(ctx, page, box_name);
	pdf_obj *newbox;
	fz_rect old_rect;

	if (box == NULL)
		return;

	old_rect.x0 = pdf_to_real(ctx, pdf_array_get(ctx, box, 0));
	old_rect.y0 = pdf_to_real(ctx, pdf_array_get(ctx, box, 1));
	old_rect.x1 = pdf_to_real(ctx, pdf_array_get(ctx, box, 2));
	old_rect.y1 = pdf_to_real(ctx, pdf_array_get(ctx, box, 3));

	if (old_rect.x0 < mb->x0)
		old_rect.x0 = mb->x0;
	if (old_rect.y0 < mb->y0)
		old_rect.y0 = mb->y0;
	if (old_rect.x1 > mb->x1)
		old_rect.x1 = mb->x1;
	if (old_rect.y1 > mb->y1)
		old_rect.y1 = mb->y1;

	newbox = pdf_new_array(ctx, doc, 4);
	pdf_array_push(ctx, newbox, pdf_new_real(ctx, doc, old_rect.x0));
	pdf_array_push(ctx, newbox, pdf_new_real(ctx, doc, old_rect.y0));
	pdf_array_push(ctx, newbox, pdf_new_real(ctx, doc, old_rect.x1));
	pdf_array_push(ctx, newbox, pdf_new_real(ctx, doc, old_rect.y1));
	pdf_dict_put(ctx, page, box_name, newbox);
}

/*
 * Recreate page tree with our posterised pages in.
 */

static void decimatepages(fz_context *ctx, pdf_document *doc)
{
	pdf_obj *oldroot, *root, *pages, *kids, *parent;
	int num_pages = pdf_count_pages(ctx, doc);
	int page, kidcount;

	oldroot = pdf_dict_get(ctx, pdf_trailer(ctx, doc), PDF_NAME_Root);
	pages = pdf_dict_get(ctx, oldroot, PDF_NAME_Pages);

	root = pdf_new_dict(ctx, doc, 2);
	pdf_dict_put(ctx, root, PDF_NAME_Type, pdf_dict_get(ctx, oldroot, PDF_NAME_Type));
	pdf_dict_put(ctx, root, PDF_NAME_Pages, pdf_dict_get(ctx, oldroot, PDF_NAME_Pages));

	pdf_update_object(ctx, doc, pdf_to_num(ctx, oldroot), root);

	pdf_drop_obj(ctx, root);

	/* Create a new kids array with our new pages in */
	parent = pdf_new_indirect(ctx, doc, pdf_to_num(ctx, pages), pdf_to_gen(ctx, pages));
	kids = pdf_new_array(ctx, doc, 1);

	kidcount = 0;
	for (page=0; page < num_pages; page++)
	{
		pdf_page *page_details = pdf_load_page(ctx, doc, page);
		int xf = x_factor, yf = y_factor;
		int x, y;
		float w = page_details->mediabox.x1 - page_details->mediabox.x0;
		float h = page_details->mediabox.y1 - page_details->mediabox.y0;

		if (xf == 0 && yf == 0)
		{
			/* Nothing specified, so split along the long edge */
			if (w > h)
				xf = 2, yf = 1;
			else
				xf = 1, yf = 2;
		}
		else if (xf == 0)
			xf = 1;
		else if (yf == 0)
			yf = 1;

		for (y = yf-1; y >= 0; y--)
		{
			for (x = 0; x < xf; x++)
			{
				pdf_obj *newpageobj, *newpageref, *newmediabox;
				fz_rect mb;
				int num;

				newpageobj = pdf_copy_dict(ctx, pdf_lookup_page_obj(ctx, doc, page));
				num = pdf_create_object(ctx, doc);
				pdf_update_object(ctx, doc, num, newpageobj);
				newpageref = pdf_new_indirect(ctx, doc, num, 0);

				newmediabox = pdf_new_array(ctx, doc, 4);

				mb.x0 = page_details->mediabox.x0 + (w/xf)*x;
				if (x == xf-1)
					mb.x1 = page_details->mediabox.x1;
				else
					mb.x1 = page_details->mediabox.x0 + (w/xf)*(x+1);
				mb.y0 = page_details->mediabox.y0 + (h/yf)*y;
				if (y == yf-1)
					mb.y1 = page_details->mediabox.y1;
				else
					mb.y1 = page_details->mediabox.y0 + (h/yf)*(y+1);

				pdf_array_push(ctx, newmediabox, pdf_new_real(ctx, doc, mb.x0));
				pdf_array_push(ctx, newmediabox, pdf_new_real(ctx, doc, mb.y0));
				pdf_array_push(ctx, newmediabox, pdf_new_real(ctx, doc, mb.x1));
				pdf_array_push(ctx, newmediabox, pdf_new_real(ctx, doc, mb.y1));

				pdf_dict_put(ctx, newpageobj, PDF_NAME_Parent, parent);
				pdf_dict_put(ctx, newpageobj, PDF_NAME_MediaBox, newmediabox);

				intersect_box(ctx, doc, newpageobj, PDF_NAME_CropBox, &mb);
				intersect_box(ctx, doc, newpageobj, PDF_NAME_BleedBox, &mb);
				intersect_box(ctx, doc, newpageobj, PDF_NAME_TrimBox, &mb);
				intersect_box(ctx, doc, newpageobj, PDF_NAME_ArtBox, &mb);

				/* Store page object in new kids array */
				pdf_array_push(ctx, kids, newpageref);

				kidcount++;
			}
		}
	}

	pdf_drop_obj(ctx, parent);

	/* Update page count and kids array */
	pdf_dict_put(ctx, pages, PDF_NAME_Count, pdf_new_int(ctx, doc, kidcount));
	pdf_dict_put(ctx, pages, PDF_NAME_Kids, kids);
	pdf_drop_obj(ctx, kids);
}

int pdfposter_main(int argc, char **argv)
{
	char *infile;
	char *outfile = "out.pdf";
	char *password = "";
	int c;
	pdf_write_options opts = { 0 };
	pdf_document *doc;
	fz_context *ctx;

	while ((c = fz_getopt(argc, argv, "x:y:")) != -1)
	{
		switch (c)
		{
		case 'p': password = fz_optarg; break;
		case 'x': x_factor = atoi(fz_optarg); break;
		case 'y': y_factor = atoi(fz_optarg); break;
		default: usage(); break;
		}
	}

	if (argc - fz_optind < 1)
		usage();

	infile = argv[fz_optind++];

	if (argc - fz_optind > 0 &&
		(strstr(argv[fz_optind], ".pdf") || strstr(argv[fz_optind], ".PDF")))
	{
		outfile = argv[fz_optind++];
	}

	ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
	if (!ctx)
	{
		fprintf(stderr, "cannot initialise context\n");
		exit(1);
	}

	doc = pdf_open_document(ctx, infile);
	if (pdf_needs_password(ctx, doc))
		if (!pdf_authenticate_password(ctx, doc, password))
			fz_throw(ctx, FZ_ERROR_GENERIC, "cannot authenticate password: %s", infile);

	decimatepages(ctx, doc);

	pdf_save_document(ctx, doc, outfile, &opts);

	pdf_drop_document(ctx, doc);
	fz_drop_context(ctx);
	return 0;
}
