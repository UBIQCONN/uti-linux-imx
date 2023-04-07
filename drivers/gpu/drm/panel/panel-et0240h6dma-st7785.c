// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2019, Amarula Solutions.
 * Author: Jagan Teki <jagan@amarulasolutions.com>
 */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>
#include <video/of_videomode.h>
#include <video/videomode.h>


#include <drm/drm_crtc.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>

/* Panel specific color-format bits */
#define COL_FMT_16BPP 0x55
#define COL_FMT_18BPP 0x66
#define COL_FMT_24BPP 0x77


struct st7785_panel_desc {
	const struct drm_display_mode *mode;
	unsigned int lanes;
	unsigned long flags;
	enum mipi_dsi_pixel_format format;
	const char *const *supply_names;
	unsigned int num_supplies;
	unsigned int panel_sleep_delay;
};

struct st7785 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	const struct st7785_panel_desc *desc;

	struct regulator_bulk_data *supplies;
	struct gpio_desc *reset;
	unsigned int sleep_delay;
};

static inline struct st7785 *panel_to_st7785(struct drm_panel *panel)
{
	return container_of(panel, struct st7785, panel);
}

static void st7785_init_sequence(struct st7785 *st7785)
{
	const struct drm_display_mode *mode = st7785->desc->mode;
	struct mipi_dsi_device *dsi = st7785->dsi;
	int ret;
	u8 format;

	msleep(5);

	dsi->channel=0;
	dev_info(st7785->panel.dev, "%s %d %d START\n", __func__, __LINE__,  dsi->channel);

	msleep(150);
	ret = mipi_dsi_dcs_write(dsi, MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);
	if (ret < 0) {
		dev_err(st7785->panel.dev, "exit_sleep_mode cmd failed ret = %d\n", ret);
		goto power_off;
	}
/*
	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]) { 0xB0, 0x10}, 2);
	if (ret < 0) {
		dev_err(st7785->panel.dev, "cmd 2 = %d\n", ret);
		goto power_off;
	}
*/
	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]) { 0x36, 0x00 }, 2);
	if (ret < 0) {
		dev_err(st7785->panel.dev, "cmd 2 = %d\n", ret);
		goto power_off;
	}

	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]) { 0x3A, 0x66 }, 2);
	if (ret < 0) {
		dev_err(st7785->panel.dev, "cmd 3 = %d\n", ret);
		goto power_off;
	}

	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]) { 0xB2,0x0C,0x0C,0x00,0x33,0x33 }, 6);
	if (ret < 0) {
		dev_err(st7785->panel.dev, "cmd 4 = %d\n", ret);
		goto power_off;
	}

	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]) { 0xb7, 0x75 }, 2);
	if (ret < 0) {
		dev_err(st7785->panel.dev, "cmd 5 = %d\n", ret);
		goto power_off;
	}

	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]) { 0xbb, 0x2d }, 2);
	if (ret < 0) {
		dev_err(st7785->panel.dev, "cmd 6 = %d\n", ret);
		goto power_off;
	}

	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]) { 0xc0, 0x2c }, 2);
	if (ret < 0) {
		dev_err(st7785->panel.dev, "cmd 7 = %d\n", ret);
		goto power_off;
	}

	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]) { 0xc2, 0x01 }, 2);
	if (ret < 0) {
		dev_err(st7785->panel.dev, "cmd 8 = %d\n", ret);
		goto power_off;
	}

	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]) { 0xc3, 0x13 }, 2);
	if (ret < 0) {
		dev_err(st7785->panel.dev, "cmd 9 = %d\n", ret);
		goto power_off;
	}

	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]) { 0xc4, 0x20 }, 2);
	if (ret < 0) {
		dev_err(st7785->panel.dev, "cmd 10 = %d\n", ret);
		goto power_off;
	}

	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]) { 0xc5, 0x1c }, 2);
	if (ret < 0) {
		dev_err(st7785->panel.dev, "cmd 11 = %d\n", ret);
		goto power_off;
	}

	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]) { 0xc6, 0x0f }, 2);
	if (ret < 0) {
		dev_err(st7785->panel.dev, "cmd 12 = %d\n", ret);
		goto power_off;
	}


	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]) { 0xd0, 0xa7 }, 2);
	if (ret < 0) {
		dev_err(st7785->panel.dev, "cmd 13 = %d\n", ret);
		goto power_off;
	}

/*
	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]) { 0xB0,0x00,0x10 }, 3);
	if (ret < 0) {
		dev_err(st7785->panel.dev, "cmd 14 = %d\n", ret);
		goto power_off;
	}

*/	

	
	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]) { 0xB0, 0x11}, 2);
	if (ret < 0) {
		dev_err(st7785->panel.dev, "cmd 2 = %d\n", ret);
		goto power_off;
	}


	ret = mipi_dsi_dcs_write(dsi, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
	if (ret < 0) {
		dev_err(st7785->panel.dev, "exit_sleep_mode cmd failed ret = %d\n", ret);
		goto power_off;
	}
	ret = mipi_dsi_dcs_write(dsi, MIPI_DCS_ENTER_INVERT_MODE, NULL, 0);

	if (ret < 0) {
		dev_err(st7785->panel.dev, "exit_sleep_mode cmd failed ret = %d\n", ret);
		goto power_off;
	}

	if (0)	
	{
		u8 output_read[3] = {0, 0, 0};
		ret = mipi_dsi_generic_read(dsi, (u8[]) {0x4}, 1, &output_read, 3);
		if (ret < 0) { 
		
			dev_err(st7785->panel.dev, "mipi_dsi_generic_read ret = %d\n", ret);
		
		}
		else
		{
			printk(KERN_INFO "read pixels [%x, %x, %x]", output_read[0], output_read[1], output_read[2]);

		}
	}	
//	mipi_dsi_dcs_get_power_mode(dsi, &format);

	msleep(st7785->sleep_delay);
	dev_info(st7785->panel.dev, "%s %d %x END\n", __func__, __LINE__, format);

power_off:
	return;
}

static int st7785_prepare(struct drm_panel *panel)
{
	struct st7785 *st7785 = panel_to_st7785(panel);
	int ret;

	msleep(150);

	dev_info(st7785->panel.dev, "%s %d\n", __func__, __LINE__);
//	st7785_init_sequence(st7785);
	if (st7785->reset) {
		gpiod_set_value_cansleep(st7785->reset, 0);
		dev_info(st7785->panel.dev, "%s\n", __func__);
		/*
		 * 50ms delay after reset-out, as per manufacturer initalization
		 * sequence.
		 */
		msleep(50);
	}

	return 0;
}

static int st7785_enable(struct drm_panel *panel)
{
	struct st7785 *st7785 = panel_to_st7785(panel);

//	ST7701_DSI(st7785, MIPI_DCS_SET_DISPLAY_ON, 0x00);
	st7785_init_sequence(st7785);

	return 0;
}

static int st7785_disable(struct drm_panel *panel)
{
	struct st7785 *st7785 = panel_to_st7785(panel);

	dev_info(st7785->panel.dev, "%s %d\n", __func__, __LINE__);
//	ST7701_DSI(st7785, MIPI_DCS_SET_DISPLAY_OFF, 0x00);

	return 0;
}

static int st7785_unprepare(struct drm_panel *panel)
{
	struct st7785 *st7785 = panel_to_st7785(panel);

	dev_info(st7785->panel.dev, "%s %d\n", __func__, __LINE__);
//	ST7701_DSI(st7785, MIPI_DCS_ENTER_SLEEP_MODE, 0x00);
	if (st7785->reset) {
		gpiod_set_value_cansleep(st7785->reset, 1);
		usleep_range(15000, 17000);
		gpiod_set_value_cansleep(st7785->reset, 0);
	}

	msleep(st7785->sleep_delay);


	/**
	 * During the Resetting period, the display will be blanked
	 * (The display is entering blanking sequence, which maximum
	 * time is 120 ms, when Reset Starts in Sleep Out –mode. The
	 * display remains the blank state in Sleep In –mode.) and
	 * then return to Default condition for Hardware Reset.
	 *
	 * So we need wait sleep_delay time to make sure reset completed.
	 */
	return 0;
}


static const u32 rad_bus_formats[] = {
	MEDIA_BUS_FMT_RGB888_1X24,
	MEDIA_BUS_FMT_RGB666_1X18,
	MEDIA_BUS_FMT_RGB565_1X16,
};

static const u32 rad_bus_flags = DRM_BUS_FLAG_DE_LOW |
				 DRM_BUS_FLAG_PIXDATA_SAMPLE_POSEDGE;

static int st7785_get_modes(struct drm_panel *panel,
			    struct drm_connector *connector)
{
	struct st7785 *st7785 = panel_to_st7785(panel);
	const struct drm_display_mode *desc_mode = st7785->desc->mode;
	struct drm_display_mode *mode;

	dev_info(st7785->panel.dev, "%s %d\n", __func__, __LINE__);
	mode = drm_mode_duplicate(connector->dev, desc_mode);
	if (!mode) {
		dev_err(&st7785->dsi->dev, "failed to add mode %ux%u@%u\n",
			desc_mode->hdisplay, desc_mode->vdisplay,
			drm_mode_vrefresh(desc_mode));
		return -ENOMEM;
	}

	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
/*	drm_mode_probed_add(connector, mode); */

	connector->display_info.width_mm = desc_mode->width_mm;
	connector->display_info.height_mm = desc_mode->height_mm;
	drm_mode_probed_add(connector, mode);
	connector->display_info.bus_flags = rad_bus_flags;

	drm_display_info_set_bus_formats(&connector->display_info,
					 rad_bus_formats,
					 ARRAY_SIZE(rad_bus_formats));
	dev_info(st7785->panel.dev, "%s %d\n", __func__, __LINE__);
	return 1;
}

static const struct drm_panel_funcs st7785_funcs = {
	.disable	= st7785_disable,
	.unprepare	= st7785_unprepare,
	.prepare	= st7785_prepare,
	.enable		= st7785_enable,
	.get_modes	= st7785_get_modes,
};

static const struct drm_display_mode ts8550b_mode = {
	.clock		= 7730,

	.hdisplay	= 240,
	.hsync_start	= 240 + 50,
	.hsync_end	= 240 + 50 + 10,
	.htotal		= 240 + 50 + 10 + 66,

	.vdisplay	= 320,
	.vsync_start	= 320 + 10,
	.vsync_end	= 320 + 10 + 10,
	.vtotal		= 320 + 10 + 10 + 22,

	.width_mm	= 366,
	.height_mm	= 352,

	.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
};

static const char * const ts8550b_supply_names[] = {
	"VCC",
	"IOVCC",
};

static const struct st7785_panel_desc ts8550b_desc = {
	.mode = &ts8550b_mode,
	.lanes = 1,
	.format = MIPI_DSI_FMT_RGB888,
	.flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE | MIPI_DSI_CLOCK_NON_CONTINUOUS | 
		      MIPI_DSI_MODE_LPM, 
/*	.flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE, */
		       
/*	.flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE | MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_NO_EOT_PACKET | MIPI_DSI_CLOCK_NON_CONTINUOUS | MIPI_DSI_MODE_LPM,  
	.flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_LPM, */
	.supply_names = ts8550b_supply_names,
	.num_supplies = ARRAY_SIZE(ts8550b_supply_names),
	.panel_sleep_delay = 80, /* panel need extra 80ms for sleep out cmd */
};

static int st7785_dsi_probe(struct mipi_dsi_device *dsi)
{
	const struct st7785_panel_desc *desc;
	struct st7785 *st7785;
	int ret;

	st7785 = devm_kzalloc(&dsi->dev, sizeof(*st7785), GFP_KERNEL);
	if (!st7785)
		return -ENOMEM;

	desc = of_device_get_match_data(&dsi->dev);
	dsi->mode_flags = desc->flags;
	dsi->format = desc->format;
	dsi->lanes = desc->lanes;

	st7785->reset = devm_gpiod_get_optional(&dsi->dev, "reset",
					       GPIOD_OUT_LOW |
					       GPIOD_FLAGS_BIT_NONEXCLUSIVE);
	if (IS_ERR(st7785->reset)) {
		ret = PTR_ERR(st7785->reset);
		dev_err(&dsi->dev, "Failed to get reset gpio (%d)\n", ret);
		return ret;
	}
	if (st7785->reset) {
		gpiod_set_value_cansleep(st7785->reset, 1);
		dev_info(&dsi->dev, "GPIO SET (%d)\n", ret);
	}	

	drm_panel_init(&st7785->panel, &dsi->dev, &st7785_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	/**
	 * Once sleep out has been issued, ST7701 IC required to wait 120ms
	 * before initiating new commands.
	 *
	 * On top of that some panels might need an extra delay to wait, so
	 * add panel specific delay for those cases. As now this panel specific
	 * delay information is referenced from those panel BSP driver, example
	 * ts8550b and there is no valid documentation for that.
	 */
	st7785->sleep_delay = 120 + desc->panel_sleep_delay;

	ret = drm_panel_of_backlight(&st7785->panel);
	if (ret)
		return ret;

	drm_panel_add(&st7785->panel);

	mipi_dsi_set_drvdata(dsi, st7785);
	st7785->dsi = dsi;
	st7785->desc = desc;

	dev_info(st7785->panel.dev, "%s %d\n", __func__, __LINE__);
	return mipi_dsi_attach(dsi);
}

static int st7785_dsi_remove(struct mipi_dsi_device *dsi)
{
	struct st7785 *st7785 = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&st7785->panel);

	return 0;
}

static const struct of_device_id st7785_of_match[] = {
	{ .compatible = "et0240h6dma,st7785", .data = &ts8550b_desc },
	{ }
};
MODULE_DEVICE_TABLE(of, st7785_of_match);

static struct mipi_dsi_driver st7785_dsi_driver = {
	.probe		= st7785_dsi_probe,
	.remove		= st7785_dsi_remove,
	.driver = {
		.name		= "st7785",
		.of_match_table	= st7785_of_match,
	},
};
module_mipi_dsi_driver(st7785_dsi_driver);

MODULE_AUTHOR("Jagan Teki <jagan@amarulasolutions.com>");
MODULE_DESCRIPTION("Sitronix ST7701 LCD Panel Driver");
MODULE_LICENSE("GPL");
