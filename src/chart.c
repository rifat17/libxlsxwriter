/*****************************************************************************
 * chart - A library for creating Excel XLSX chart files.
 *
 * Used in conjunction with the libxlsxwriter library.
 *
 * Copyright 2014-2016, John McNamara, jmcnamara@cpan.org. See LICENSE.txt.
 *
 */

#include "xlsxwriter/xmlwriter.h"
#include "xlsxwriter/chart.h"
#include "xlsxwriter/utility.h"

/*
 * Forward declarations.
 */

/*****************************************************************************
 *
 * Private functions.
 *
 ****************************************************************************/

/*
 * Create a new chart object.
 */
lxw_chart *
lxw_chart_new()
{
    lxw_chart *chart = calloc(1, sizeof(lxw_chart));
    GOTO_LABEL_ON_MEM_ERROR(chart, mem_error);

    chart->series = calloc(1, sizeof(struct lxw_chart_series_list));
    GOTO_LABEL_ON_MEM_ERROR(chart->series, mem_error);

    STAILQ_INIT(chart->series);

    return chart;

mem_error:
    lxw_chart_free(chart);
    return NULL;
}

/*
 * Free a chart object.
 */
void
lxw_chart_free(lxw_chart *chart)
{
    lxw_chart_series *series;

    if (!chart)
        return;

    if (chart->series) {
        while (!STAILQ_EMPTY(chart->series)) {
            series = STAILQ_FIRST(chart->series);
            STAILQ_REMOVE_HEAD(chart->series, list_pointers);

            free(series->values.range);
            free(series->values.sheetname);

        }

        free(chart->series);
    }

    free(chart);
}

/*****************************************************************************
 *
 * XML functions.
 *
 ****************************************************************************/

/*
 * Write the XML declaration.
 */
STATIC void
_chart_xml_declaration(lxw_chart *self)
{
    lxw_xml_declaration(self->file);
}

/*
 * Write the <c:chartSpace> element.
 */
STATIC void
_chart_write_chart_space(lxw_chart *self)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;
    char xmlns_c[] = "http://schemas.openxmlformats.org/drawingml/2006/chart";
    char xmlns_a[] = "http://schemas.openxmlformats.org/drawingml/2006/main";
    char xmlns_r[] =
        "http://schemas.openxmlformats.org/officeDocument/2006/relationships";

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_STR("xmlns:c", xmlns_c);
    LXW_PUSH_ATTRIBUTES_STR("xmlns:a", xmlns_a);
    LXW_PUSH_ATTRIBUTES_STR("xmlns:r", xmlns_r);

    lxw_xml_start_tag(self->file, "c:chartSpace", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Write the <c:lang> element.
 */
STATIC void
_chart_write_lang(lxw_chart *self)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_STR("val", "en-US");

    lxw_xml_empty_tag(self->file, "c:lang", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Write the <c:layout> element.
 */
STATIC void
_chart_write_layout(lxw_chart *self)
{
    lxw_xml_empty_tag(self->file, "c:layout", NULL);
}

/*
 * Write the <c:grouping> element.
 */
STATIC void
_chart_write_grouping(lxw_chart *self)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;
    char val[] = "clustered";

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_STR("val", val);

    lxw_xml_empty_tag(self->file, "c:grouping", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Write the <c:idx> element.
 */
STATIC void
_chart_write_idx(lxw_chart *self, uint16_t index)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_INT("val", index);

    lxw_xml_empty_tag(self->file, "c:idx", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Write the <c:order> element.
 */
STATIC void
_chart_write_order(lxw_chart *self, uint16_t index)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_INT("val", index);

    lxw_xml_empty_tag(self->file, "c:order", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Add unique ids for primary or secondary axes.
 */
STATIC void
_chart_add_axis_ids(lxw_chart *self)
{
    uint32_t chart_id = 50010000 + self->id;
    uint32_t axis_count = 1;

    self->axis_id_1 = chart_id + axis_count;
    self->axis_id_2 = self->axis_id_1;
}

/*
 * Write the <c:axId> element.
 */
STATIC void
_chart_write_axis_id(lxw_chart *self, uint32_t axis_id)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_INT("val", axis_id);

    lxw_xml_empty_tag(self->file, "c:axId", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Write the <c:axId> element.
 */
STATIC void
_chart_write_axis_ids(lxw_chart *self)
{
    if (!self->axis_id_1)
        _chart_add_axis_ids(self);

    _chart_write_axis_id(self, self->axis_id_1);
    _chart_write_axis_id(self, self->axis_id_2);
}

/*
 * Write the <c:f> element.
 */
STATIC void
_chart_write_f(lxw_chart *self, char *range)
{
    lxw_xml_data_element(self->file, "c:f", range, NULL);
}

/*
 * Write the <c:numRef> element.
 */
STATIC void
_chart_write_num_ref(lxw_chart *self, char *range)
{

    lxw_xml_start_tag(self->file, "c:numRef", NULL);

    /* Write the c:f element. */
    _chart_write_f(self, range);

    lxw_xml_end_tag(self->file, "c:numRef");
}

/*
 * Write the <c:val> element.
 */
STATIC void
_chart_write_val(lxw_chart *self, lxw_chart_series *series)
{

    lxw_xml_start_tag(self->file, "c:val", NULL);

    /* Write the c:numRef element. */
    _chart_write_num_ref(self, series->values.range);

    lxw_xml_end_tag(self->file, "c:val");
}

/*
 * Write the <c:ser> element.
 */
STATIC void
_chart_write_ser(lxw_chart *self, lxw_chart_series *series)
{
    uint16_t index = self->series_index++;

    lxw_xml_start_tag(self->file, "c:ser", NULL);

    /* Write the c:idx element. */
    _chart_write_idx(self, index);

    /* Write the c:order element. */
    _chart_write_order(self, index);

    /* Write the c:val element. */
    _chart_write_val(self, series);

    lxw_xml_end_tag(self->file, "c:ser");
}

/*
 * Write the <c:orientation> element.
 */
STATIC void
_chart_write_orientation(lxw_chart *self)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;
    char val[] = "minMax";

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_STR("val", val);

    lxw_xml_empty_tag(self->file, "c:orientation", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Write the <c:scaling> element.
 */
STATIC void
_chart_write_scaling(lxw_chart *self)
{
    lxw_xml_start_tag(self->file, "c:scaling", NULL);

    /* Write the c:orientation element. */
    _chart_write_orientation(self);

    lxw_xml_end_tag(self->file, "c:scaling");
}

/*
 * Write the <c:axPos> element.
 */
STATIC void
_chart_write_axis_pos(lxw_chart *self, char *position)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_STR("val", position);

    lxw_xml_empty_tag(self->file, "c:axPos", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Write the <c:tickLblPos> element.
 */
STATIC void
_chart_write_tick_lbl_pos(lxw_chart *self)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;
    char val[] = "nextTo";

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_STR("val", val);

    lxw_xml_empty_tag(self->file, "c:tickLblPos", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Write the <c:crossAx> element.
 */
STATIC void
_chart_write_cross_axis(lxw_chart *self, uint32_t axis_id)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_INT("val", axis_id);

    lxw_xml_empty_tag(self->file, "c:crossAx", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Write the <c:crosses> element.
 */
STATIC void
_chart_write_crosses(lxw_chart *self)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;
    char val[] = "autoZero";

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_STR("val", val);

    lxw_xml_empty_tag(self->file, "c:crosses", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Write the <c:auto> element.
 */
STATIC void
_chart_write_auto(lxw_chart *self)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;
    char val[] = "1";

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_STR("val", val);

    lxw_xml_empty_tag(self->file, "c:auto", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Write the <c:lblAlgn> element.
 */
STATIC void
_chart_write_lbl_algn(lxw_chart *self)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;
    char val[] = "ctr";

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_STR("val", val);

    lxw_xml_empty_tag(self->file, "c:lblAlgn", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Write the <c:lblOffset> element.
 */
STATIC void
_chart_write_lbl_offset(lxw_chart *self)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;
    char val[] = "100";

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_STR("val", val);

    lxw_xml_empty_tag(self->file, "c:lblOffset", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Write the <c:majorGridlines> element.
 */
STATIC void
_chart_write_major_gridlines(lxw_chart *self)
{
    lxw_xml_empty_tag(self->file, "c:majorGridlines", NULL);
}

/*
 * Write the <c:numFmt> element.
 */
STATIC void
_chart_write_num_fmt(lxw_chart *self)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;
    char format_code[] = "General";
    char source_linked[] = "1";

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_STR("formatCode", format_code);
    LXW_PUSH_ATTRIBUTES_STR("sourceLinked", source_linked);

    lxw_xml_empty_tag(self->file, "c:numFmt", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Write the <c:crossBetween> element.
 */
STATIC void
_chart_write_cross_between(lxw_chart *self)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;
    char val[] = "between";

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_STR("val", val);

    lxw_xml_empty_tag(self->file, "c:crossBetween", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Write the <c:legendPos> element.
 */
STATIC void
_chart_write_legend_pos(lxw_chart *self)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;
    char val[] = "r";

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_STR("val", val);

    lxw_xml_empty_tag(self->file, "c:legendPos", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Write the <c:legend> element.
 */
STATIC void
_chart_write_legend(lxw_chart *self)
{
    lxw_xml_start_tag(self->file, "c:legend", NULL);

    /* Write the c:legendPos element. */
    _chart_write_legend_pos(self);

    /* Write the c:layout element. */
    _chart_write_layout(self);

    lxw_xml_end_tag(self->file, "c:legend");
}

/*
 * Write the <c:plotVisOnly> element.
 */
STATIC void
_chart_write_plot_vis_only(lxw_chart *self)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;
    char val[] = "1";

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_STR("val", val);

    lxw_xml_empty_tag(self->file, "c:plotVisOnly", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Write the <c:headerFooter> element.
 */
STATIC void
_chart_write_header_footer(lxw_chart *self)
{
    lxw_xml_empty_tag(self->file, "c:headerFooter", NULL);
}

/*
 * Write the <c:pageMargins> element.
 */
STATIC void
_chart_write_page_margins(lxw_chart *self)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;
    char b[] = "0.75";
    char l[] = "0.7";
    char r[] = "0.7";
    char t[] = "0.75";
    char header[] = "0.3";
    char footer[] = "0.3";

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_STR("b", b);
    LXW_PUSH_ATTRIBUTES_STR("l", l);
    LXW_PUSH_ATTRIBUTES_STR("r", r);
    LXW_PUSH_ATTRIBUTES_STR("t", t);
    LXW_PUSH_ATTRIBUTES_STR("header", header);
    LXW_PUSH_ATTRIBUTES_STR("footer", footer);

    lxw_xml_empty_tag(self->file, "c:pageMargins", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Write the <c:pageSetup> element.
 */
STATIC void
_chart_write_page_setup(lxw_chart *self)
{
    lxw_xml_empty_tag(self->file, "c:pageSetup", NULL);
}

/*
 * Write the <c:printSettings> element.
 */
STATIC void
_chart_write_print_settings(lxw_chart *self)
{
    lxw_xml_start_tag(self->file, "c:printSettings", NULL);

    /* Write the c:headerFooter element. */
    _chart_write_header_footer(self);

    /* Write the c:pageMargins element. */
    _chart_write_page_margins(self);

    /* Write the c:pageSetup element. */
    _chart_write_page_setup(self);

    lxw_xml_end_tag(self->file, "c:printSettings");
}

/***************************
 *
 *  WIP
 *
 */

/*
 * Write the <c:catAx> element. Usually the X axis.
 */
STATIC void
_chart_write_cat_axis(lxw_chart *self)
{
    lxw_xml_start_tag(self->file, "c:catAx", NULL);

    _chart_write_axis_id(self, self->axis_id_1);

    /* Write the c:scaling element. */
    _chart_write_scaling(self);

    /* Write the c:axPos element. */
    _chart_write_axis_pos(self, "l");

    /* Write the c:tickLblPos element. */
    _chart_write_tick_lbl_pos(self);

    /* Write the c:crossAx element. */
    _chart_write_cross_axis(self, self->axis_id_2);

    /* Write the c:crosses element. */
    _chart_write_crosses(self);

    /* Write the c:auto element. */
    _chart_write_auto(self);

    /* Write the c:lblAlgn element. */
    _chart_write_lbl_algn(self);

    /* Write the c:lblOffset element. */
    _chart_write_lbl_offset(self);

    lxw_xml_end_tag(self->file, "c:catAx");
}

/*
 * Write the <c:valAx> element.
 */
STATIC void
_chart_write_val_ax(lxw_chart *self)
{
    lxw_xml_start_tag(self->file, "c:valAx", NULL);

    _chart_write_axis_id(self, self->axis_id_2);

    /* Write the c:scaling element. */
    _chart_write_scaling(self);

    /* Write the c:axPos element. */
    _chart_write_axis_pos(self, "b");

    /* Write the c:majorGridlines element. */
    _chart_write_major_gridlines(self);

    /* Write the c:numFmt element. */
    _chart_write_num_fmt(self);

    /* Write the c:tickLblPos element. */
    _chart_write_tick_lbl_pos(self);

    /* Write the c:crossAx element. */
    _chart_write_cross_axis(self, self->axis_id_1);

    /* Write the c:crosses element. */
    _chart_write_crosses(self);

    /* Write the c:crossBetween element. */
    _chart_write_cross_between(self);

    lxw_xml_end_tag(self->file, "c:valAx");
}

/*****************************************************************************
 * Bar chart functions.
 */

/*
 * Write the <c:barDir> element.
 */
STATIC void
_chart_write_bar_dir(lxw_chart *self)
{
    struct xml_attribute_list attributes;
    struct xml_attribute *attribute;

    LXW_INIT_ATTRIBUTES();
    LXW_PUSH_ATTRIBUTES_STR("val", "bar");

    lxw_xml_empty_tag(self->file, "c:barDir", &attributes);

    LXW_FREE_ATTRIBUTES();
}

/*
 * Write the <c:barChart> element.
 */
STATIC void
_chart_write_bar_chart(lxw_chart *self)
{
    lxw_chart_series *series;

    lxw_xml_start_tag(self->file, "c:barChart", NULL);

    /* Write the c:barDir element. */
    _chart_write_bar_dir(self);

    /* Write the c:grouping element. */
    _chart_write_grouping(self);

    STAILQ_FOREACH(series, self->series, list_pointers) {
        /* Write the c:ser element. */
        _chart_write_ser(self, series);
    }

    /* Write the c:axId elements. */
    _chart_write_axis_ids(self);

    lxw_xml_end_tag(self->file, "c:barChart");
}

/*****************************************************************************
 * End of sub chart functions.
 */

/*
 * Write the chart type element.
 */
STATIC void
_chart_write_chart_type(lxw_chart *self)
{
    /* Write the c:barChart element. */
    _chart_write_bar_chart(self);
}

/*
 * Write the <c:plotArea> element.
 */
STATIC void
_chart_write_plot_area(lxw_chart *self)
{
    lxw_xml_start_tag(self->file, "c:plotArea", NULL);

    /* Write the c:layout element. */
    _chart_write_layout(self);

    /* Write the subclass chart type elements for primary and secondary axes. */
    _chart_write_chart_type(self);

}

/*
 * Write the <c:chart> element.
 */
STATIC void
_chart_write_chart(lxw_chart *self)
{
    lxw_xml_start_tag(self->file, "c:chart", NULL);

    /* Write the c:plotArea element. */
    _chart_write_plot_area(self);

    /* Write the c:catAx element. */
    _chart_write_cat_axis(self);

    /* Write the c:valAx element. */
    _chart_write_val_ax(self);

    lxw_xml_end_tag(self->file, "c:plotArea");

    /* Write the c:legend element. */
    _chart_write_legend(self);

    /* Write the c:plotVisOnly element. */
    _chart_write_plot_vis_only(self);

    lxw_xml_end_tag(self->file, "c:chart");
}

/*
 * Assemble and write the XML file.
 */
void
lxw_chart_assemble_xml_file(lxw_chart *self)
{
    /* Write the XML declaration. */
    _chart_xml_declaration(self);

    /* Write the c:chartSpace element. */
    _chart_write_chart_space(self);

    /* Write the c:lang element. */
    _chart_write_lang(self);

    /* Write the c:chart element. */
    _chart_write_chart(self);

    /* Write the c:printSettings element. */
    _chart_write_print_settings(self);

    lxw_xml_end_tag(self->file, "c:chartSpace");
}

/*****************************************************************************
 *
 * Public functions.
 *
 ****************************************************************************/

/*
 * Insert an image into the worksheet.
 */
int
chart_add_series(lxw_chart *self, lxw_chart_series *user_series)
{
    lxw_chart_series *series;

    if (!user_series) {
        LXW_WARN("chart_add_series(): series must be non-NULL");
        return -1;
    }

    /* Create a new object to hold the series. */
    series = calloc(1, sizeof(lxw_chart_series));
    RETURN_ON_MEM_ERROR(series, -1);

    memcpy(series, user_series, sizeof(lxw_chart_series));
    series->values.range = lxw_strdup(user_series->values.range);
    series->values.sheetname = lxw_strdup(user_series->values.sheetname);

    STAILQ_INSERT_TAIL(self->series, series, list_pointers);

    return 0;
}
