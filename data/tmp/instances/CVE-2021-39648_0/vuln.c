static ssize_t gadget_dev_desc_UDC_show(struct config_item *item, char *page)
{
	char *udc_name = to_gadget_info(item)->composite.gadget_driver.udc_name;

	return sprintf(page, "%s\n", udc_name ?: "");
}