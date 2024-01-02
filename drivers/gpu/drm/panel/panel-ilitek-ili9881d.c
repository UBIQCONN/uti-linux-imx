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

enum ili9881d_op {
	ILI9881D_SWITCH_PAGE,
	ILI9881D_COMMAND,
};

struct ili9881d_instr {
	enum ili9881d_op	op;

	union arg {
		struct cmd {
			u8	cmd;
			u8	data;
		} cmd;
		u8	page;
	} arg;
};

struct ili9881d_panel_desc {
	const struct ili9881d_instr *init;
	const size_t init_length;
	const struct drm_display_mode *mode;
	unsigned int lanes;
	unsigned long flags;
	enum mipi_dsi_pixel_format format;
	const char *const *supply_names;
	unsigned int panel_sleep_delay;
};

struct ili9881d {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	const struct ili9881d_panel_desc *desc;

	struct gpio_desc *reset;
	unsigned int sleep_delay;
	struct mutex mutex;
};

#define ILI9881D_SWITCH_PAGE_INSTR(_page)	\
	{					\
		.op = ILI9881D_SWITCH_PAGE,	\
		.arg = {			\
			.page = (_page),	\
		},				\
	}

#define ILI9881D_COMMAND_INSTR(_cmd, _data)		\
	{						\
		.op = ILI9881D_COMMAND,		\
		.arg = {				\
			.cmd = {			\
				.cmd = (_cmd),		\
				.data = (_data),	\
			},				\
		},					\
	}

static const struct ili9881d_instr etml050015dha_init[] = {
	ILI9881D_SWITCH_PAGE_INSTR(3),
	ILI9881D_COMMAND_INSTR(0x01,0x00),
	ILI9881D_COMMAND_INSTR(0x02,0x00),
	ILI9881D_COMMAND_INSTR(0x03,0x73),
	ILI9881D_COMMAND_INSTR(0x04,0x00),
	ILI9881D_COMMAND_INSTR(0x05,0x00),
	ILI9881D_COMMAND_INSTR(0x06,0x0C),
	ILI9881D_COMMAND_INSTR(0x07,0x00),
	ILI9881D_COMMAND_INSTR(0x08,0x00),
	ILI9881D_COMMAND_INSTR(0x09,0x01),
	ILI9881D_COMMAND_INSTR(0x0a,0x01),
	ILI9881D_COMMAND_INSTR(0x0b,0x01),
	ILI9881D_COMMAND_INSTR(0x0c,0x01),
	ILI9881D_COMMAND_INSTR(0x0d,0x01),
	ILI9881D_COMMAND_INSTR(0x0e,0x01),
	ILI9881D_COMMAND_INSTR(0x0f,0x00),
	ILI9881D_COMMAND_INSTR(0x10,0x00), 
	ILI9881D_COMMAND_INSTR(0x11,0x00),
	ILI9881D_COMMAND_INSTR(0x12,0x00),
	ILI9881D_COMMAND_INSTR(0x13,0x00),
	ILI9881D_COMMAND_INSTR(0x14,0x00),
	ILI9881D_COMMAND_INSTR(0x15,0x00),
	ILI9881D_COMMAND_INSTR(0x16,0x00), 
	ILI9881D_COMMAND_INSTR(0x17,0x00), 
	ILI9881D_COMMAND_INSTR(0x18,0x00),
	ILI9881D_COMMAND_INSTR(0x19,0x00),
	ILI9881D_COMMAND_INSTR(0x1a,0x00),
	ILI9881D_COMMAND_INSTR(0x1b,0x00),
	ILI9881D_COMMAND_INSTR(0x1c,0x00),
	ILI9881D_COMMAND_INSTR(0x1d,0x00),
	ILI9881D_COMMAND_INSTR(0x1e,0x40),
	ILI9881D_COMMAND_INSTR(0x1f,0xC0),
	ILI9881D_COMMAND_INSTR(0x20,0x0A),
	ILI9881D_COMMAND_INSTR(0x21,0x05),	//03 --->0x05 
	ILI9881D_COMMAND_INSTR(0x22,0x0A),
	ILI9881D_COMMAND_INSTR(0x23,0x00),
	ILI9881D_COMMAND_INSTR(0x24,0x8C),
	ILI9881D_COMMAND_INSTR(0x25,0x8C),
	ILI9881D_COMMAND_INSTR(0x26,0x00),
	ILI9881D_COMMAND_INSTR(0x27,0x00),
	ILI9881D_COMMAND_INSTR(0x28,0x33),
	ILI9881D_COMMAND_INSTR(0x29,0x03),
	ILI9881D_COMMAND_INSTR(0x2a,0x00),
	ILI9881D_COMMAND_INSTR(0x2b,0x00),
	ILI9881D_COMMAND_INSTR(0x2c,0x00),
	ILI9881D_COMMAND_INSTR(0x2d,0x00),
	ILI9881D_COMMAND_INSTR(0x2e,0x00),
	ILI9881D_COMMAND_INSTR(0x2f,0x00),
	ILI9881D_COMMAND_INSTR(0x30,0x00),
	ILI9881D_COMMAND_INSTR(0x31,0x00),
	ILI9881D_COMMAND_INSTR(0x32,0x00),
	ILI9881D_COMMAND_INSTR(0x33,0x00),
	ILI9881D_COMMAND_INSTR(0x34,0x00),
	ILI9881D_COMMAND_INSTR(0x35,0x00),
	ILI9881D_COMMAND_INSTR(0x36,0x00),
	ILI9881D_COMMAND_INSTR(0x37,0x00),
	ILI9881D_COMMAND_INSTR(0x38,0x00),
	ILI9881D_COMMAND_INSTR(0x39,0x35),
	ILI9881D_COMMAND_INSTR(0x3A,0x01),
	ILI9881D_COMMAND_INSTR(0x3B,0x40),
	ILI9881D_COMMAND_INSTR(0x3C,0x00),
	ILI9881D_COMMAND_INSTR(0x3D,0x01),
	ILI9881D_COMMAND_INSTR(0x3E,0x00),
	ILI9881D_COMMAND_INSTR(0x3F,0x00),
	ILI9881D_COMMAND_INSTR(0x40,0x35),
	ILI9881D_COMMAND_INSTR(0x41,0xA8),
	ILI9881D_COMMAND_INSTR(0x42,0x00),
	ILI9881D_COMMAND_INSTR(0x43,0x40),  
	ILI9881D_COMMAND_INSTR(0x44,0x3F),     //1F TO 3F_ RESET KEEP LOW ALL GATE ON
	ILI9881D_COMMAND_INSTR(0x45,0x20),     //LVD???ALL GATE ON ?VGH
	ILI9881D_COMMAND_INSTR(0x46,0x00),

//GIP_2        
	ILI9881D_COMMAND_INSTR(0x50,0x01),
	ILI9881D_COMMAND_INSTR(0x51,0x23),
	ILI9881D_COMMAND_INSTR(0x52,0x45),
	ILI9881D_COMMAND_INSTR(0x53,0x67),
	ILI9881D_COMMAND_INSTR(0x54,0x89),
	ILI9881D_COMMAND_INSTR(0x55,0xAB),
	ILI9881D_COMMAND_INSTR(0x56,0x01),
	ILI9881D_COMMAND_INSTR(0x57,0x23),
	ILI9881D_COMMAND_INSTR(0x58,0x45),
	ILI9881D_COMMAND_INSTR(0x59,0x67),
	ILI9881D_COMMAND_INSTR(0x5a,0x89),
	ILI9881D_COMMAND_INSTR(0x5b,0xAB),
	ILI9881D_COMMAND_INSTR(0x5c,0xCD),
	ILI9881D_COMMAND_INSTR(0x5d,0xEF),

//GIP_3        
	ILI9881D_COMMAND_INSTR(0x5e,0x11),

	ILI9881D_COMMAND_INSTR(0x5f,0x0c),
	ILI9881D_COMMAND_INSTR(0x60,0x0d),
	ILI9881D_COMMAND_INSTR(0x61,0x0e),
	ILI9881D_COMMAND_INSTR(0x62,0x0f),
	ILI9881D_COMMAND_INSTR(0x63,0x06),
	ILI9881D_COMMAND_INSTR(0x64,0x07),
	ILI9881D_COMMAND_INSTR(0x65,0x02),
	ILI9881D_COMMAND_INSTR(0x66,0x02),
	ILI9881D_COMMAND_INSTR(0x67,0x02),
	ILI9881D_COMMAND_INSTR(0x68,0x02),
	ILI9881D_COMMAND_INSTR(0x69,0x02),
	ILI9881D_COMMAND_INSTR(0x6a,0x02),
	ILI9881D_COMMAND_INSTR(0x6b,0x02),
	ILI9881D_COMMAND_INSTR(0x6c,0x02),
	ILI9881D_COMMAND_INSTR(0x6d,0x02),
	ILI9881D_COMMAND_INSTR(0x6e,0x02),
	ILI9881D_COMMAND_INSTR(0x6f,0x02),
	ILI9881D_COMMAND_INSTR(0x70,0x02),
	ILI9881D_COMMAND_INSTR(0x71,0x02),
	ILI9881D_COMMAND_INSTR(0x72,0x02),
	ILI9881D_COMMAND_INSTR(0x73,0x01),
	ILI9881D_COMMAND_INSTR(0x74,0x00),

	ILI9881D_COMMAND_INSTR(0x75,0x0c),
	ILI9881D_COMMAND_INSTR(0x76,0x0d),
	ILI9881D_COMMAND_INSTR(0x77,0x0e),
	ILI9881D_COMMAND_INSTR(0x78,0x0f),
	ILI9881D_COMMAND_INSTR(0x79,0x06),
	ILI9881D_COMMAND_INSTR(0x7a,0x07),
	ILI9881D_COMMAND_INSTR(0x7b,0x02),
	ILI9881D_COMMAND_INSTR(0x7c,0x02),
	ILI9881D_COMMAND_INSTR(0x7d,0x02),
	ILI9881D_COMMAND_INSTR(0x7e,0x02),
	ILI9881D_COMMAND_INSTR(0x7f,0x02),
	ILI9881D_COMMAND_INSTR(0x80,0x02),
	ILI9881D_COMMAND_INSTR(0x81,0x02),
	ILI9881D_COMMAND_INSTR(0x82,0x02),
	ILI9881D_COMMAND_INSTR(0x83,0x02),
	ILI9881D_COMMAND_INSTR(0x84,0x02),
	ILI9881D_COMMAND_INSTR(0x85,0x02),
	ILI9881D_COMMAND_INSTR(0x86,0x02),
	ILI9881D_COMMAND_INSTR(0x87,0x02),
	ILI9881D_COMMAND_INSTR(0x88,0x02),
	ILI9881D_COMMAND_INSTR(0x89,0x01),
	ILI9881D_COMMAND_INSTR(0x8A,0x00),
	ILI9881D_SWITCH_PAGE_INSTR(4),
	ILI9881D_COMMAND_INSTR(0x68,0xDB),     //nonoverlap 18ns (VGH and VGL)
	ILI9881D_COMMAND_INSTR(0x6D,0x08),     //gvdd_isc[2:0]=0 (0.2uA) ???VREG1??
	ILI9881D_COMMAND_INSTR(0x70,0x00),     //VGH_MOD and VGH_DC CLKDIV disable
	ILI9881D_COMMAND_INSTR(0x71,0x00),     //VGL CLKDIV disable
	ILI9881D_COMMAND_INSTR(0x66,0x1E),     //VGH 4X
	ILI9881D_COMMAND_INSTR(0x3A,0x24),     //PS_EN OFF
	ILI9881D_COMMAND_INSTR(0x82,0x0A),     //VREF_VGH_MOD_CLPSEL 12V
	ILI9881D_COMMAND_INSTR(0x84,0x0A),     //VREF_VGH_CLPSEL 12V
	ILI9881D_COMMAND_INSTR(0x85,0x1D),     //VREF_VGL_CLPSEL 12V
	ILI9881D_COMMAND_INSTR(0x32,0xAC),     //???channel?power saving
	ILI9881D_COMMAND_INSTR(0x8C,0x80),     //sleep out Vcom disable???Vcom source???enable??????
	ILI9881D_COMMAND_INSTR(0x3C,0xF5),     //??Sample & Hold Function
	ILI9881D_COMMAND_INSTR(0x3A,0x24),     //PS_EN OFF       
	ILI9881D_COMMAND_INSTR(0xB5,0x02),     //GAMMA OP 
	ILI9881D_COMMAND_INSTR(0x31,0x25),     //SOURCE OP 
	ILI9881D_COMMAND_INSTR(0x88,0x33),     //VSP/VSN LVD Disable     
	ILI9881D_COMMAND_INSTR(0x38,0x01),  
	ILI9881D_COMMAND_INSTR(0x39,0x00),

	ILI9881D_SWITCH_PAGE_INSTR(1),
	ILI9881D_COMMAND_INSTR(0x22,0x0A),      
	ILI9881D_COMMAND_INSTR(0x31,0x00),     //column inversion     
	ILI9881D_COMMAND_INSTR(0x50,0x5C),     //VREG10UT 4.5  
	ILI9881D_COMMAND_INSTR(0x51,0x5C),     //VREG20UT -4.5
	ILI9881D_COMMAND_INSTR(0x53,0x4A),     //VC0M1      
	ILI9881D_COMMAND_INSTR(0x55,0x68),     //VC0M2        
	ILI9881D_COMMAND_INSTR(0x60,0x2B),     //SDT      
	ILI9881D_COMMAND_INSTR(0x61,0x00),     //CR    
	ILI9881D_COMMAND_INSTR(0x62,0x19),     //EQ
	ILI9881D_COMMAND_INSTR(0x63,0x00),     //PC

//REGISTER,56,00 
//Pos Register

	ILI9881D_COMMAND_INSTR(0xA0,0x0F),	
	ILI9881D_COMMAND_INSTR(0xA1,0x15),	
	ILI9881D_COMMAND_INSTR(0xA2,0x1C),	
	ILI9881D_COMMAND_INSTR(0xA3,0x0D),	
	ILI9881D_COMMAND_INSTR(0xA4,0x0E),	
	ILI9881D_COMMAND_INSTR(0xA5,0x1E),	
	ILI9881D_COMMAND_INSTR(0xA6,0x13),	
	ILI9881D_COMMAND_INSTR(0xA7,0x17),	
	ILI9881D_COMMAND_INSTR(0xA8,0x4A),	
	ILI9881D_COMMAND_INSTR(0xA9,0x1A),	
	ILI9881D_COMMAND_INSTR(0xAA,0x27),	
	ILI9881D_COMMAND_INSTR(0xAB,0x39),	
	ILI9881D_COMMAND_INSTR(0xAC,0x19),	
	ILI9881D_COMMAND_INSTR(0xAD,0x17),	
	ILI9881D_COMMAND_INSTR(0xAE,0x4E),	
	ILI9881D_COMMAND_INSTR(0xAF,0x23),	
	ILI9881D_COMMAND_INSTR(0xB0,0x29),	
	ILI9881D_COMMAND_INSTR(0xB1,0x3D),	
	ILI9881D_COMMAND_INSTR(0xB2,0x56),	
	ILI9881D_COMMAND_INSTR(0xB3,0x2F),	
//Neg Register
	ILI9881D_COMMAND_INSTR(0xC0,0x0F),	
	ILI9881D_COMMAND_INSTR(0xC1,0x17),	
	ILI9881D_COMMAND_INSTR(0xC2,0x1D),	
	ILI9881D_COMMAND_INSTR(0xC3,0x0D),	
	ILI9881D_COMMAND_INSTR(0xC4,0x0B),	
	ILI9881D_COMMAND_INSTR(0xC5,0x1B),	
	ILI9881D_COMMAND_INSTR(0xC6,0x10),	
	ILI9881D_COMMAND_INSTR(0xC7,0x16),	
	ILI9881D_COMMAND_INSTR(0xC8,0x52),	
	ILI9881D_COMMAND_INSTR(0xC9,0x1C),	
	ILI9881D_COMMAND_INSTR(0xCA,0x26),	
	ILI9881D_COMMAND_INSTR(0xCB,0x4B),	
	ILI9881D_COMMAND_INSTR(0xCC,0x17),	
	ILI9881D_COMMAND_INSTR(0xCD,0x16),	
	ILI9881D_COMMAND_INSTR(0xCE,0x4A),	
	ILI9881D_COMMAND_INSTR(0xCF,0x20),	
	ILI9881D_COMMAND_INSTR(0xD0,0x27),	
	ILI9881D_COMMAND_INSTR(0xD1,0x4B),	
	ILI9881D_COMMAND_INSTR(0xD2,0x59),	
	ILI9881D_COMMAND_INSTR(0xD3,0x39),	
	ILI9881D_SWITCH_PAGE_INSTR(0),
};

static inline struct ili9881d *panel_to_ili9881d(struct drm_panel *panel)
{
	return container_of(panel, struct ili9881d, panel);
}

/*
 * The panel seems to accept some private DCS commands that map
 * directly to registers.
 *
 * It is organised by page, with each page having its own set of
 * registers, and the first page looks like it's holding the standard
 * DCS commands.
 *
 * So before any attempt at sending a command or data, we have to be
 * sure if we're in the right page or not.
 */
static int ili9881d_switch_page(struct ili9881d *ctx, u8 page)
{
	u8 buf[4] = { 0xff, 0x98, 0x81, page };
	int ret;

	dev_dbg(&ctx->dsi->dev, "%s:%d -- %x \n", __func__, __LINE__, page);
	ret = mipi_dsi_dcs_write_buffer(ctx->dsi, buf, sizeof(buf));
	if (ret < 0)
		return ret;

	return 0;
}

static int ili9881d_send_cmd_data(struct ili9881d *ctx, u8 cmd, u8 data)
{
	u8 buf[2] = { cmd, data };
	int ret;

	dev_dbg(&ctx->dsi->dev, "%s:%d -- %x %x\n", __func__, __LINE__, cmd, data);
	ret = mipi_dsi_dcs_write_buffer(ctx->dsi, buf, sizeof(buf));
	if (ret < 0)
		return ret;

	return 0;
}

static int ili9881d_init_sequence(struct ili9881d *ctx)
{
	const struct drm_display_mode *mode = ctx->desc->mode;
	struct mipi_dsi_device *dsi = ctx->dsi;
	unsigned int i;
	int ret;
	u8 format;

	for (i = 0; i < ctx->desc->init_length; i++) {
		const struct ili9881d_instr *instr = &ctx->desc->init[i];

		if (instr->op == ILI9881D_SWITCH_PAGE)
			ret = ili9881d_switch_page(ctx, instr->arg.page);
		else if (instr->op == ILI9881D_COMMAND)
			ret = ili9881d_send_cmd_data(ctx, instr->arg.cmd.cmd,
						      instr->arg.cmd.data);

		if (ret){
			dev_info(&ctx->dsi->dev, "%s:%d error\n", __func__, __LINE__);
			return ret;
		}	
	}

	ret = ili9881d_switch_page(ctx, 0);
	if (ret)
		return ret;

	dev_info(&ctx->dsi->dev, "%s:%d \n", __func__, __LINE__);
	ret = mipi_dsi_dcs_set_tear_on(ctx->dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret)
		return ret;

	ret = mipi_dsi_dcs_exit_sleep_mode(ctx->dsi);
	if (ret)
		return ret;

//	mipi_dsi_dcs_set_display_on(ctx->dsi);
	msleep(ctx->sleep_delay);
	dev_info(ctx->panel.dev, "%s %d %x END\n", __func__, __LINE__, format);

power_off:
	return 0;
}

static int ili9881d_prepare(struct drm_panel *panel)
{
	struct ili9881d *ili9881d = panel_to_ili9881d(panel);
	int ret;

	mutex_lock(&ili9881d->mutex);
	msleep(5);

	dev_info(ili9881d->panel.dev, "%s %d\n", __func__, __LINE__);
	if (ili9881d->reset) {
		gpiod_set_value_cansleep(ili9881d->reset, 1);
		msleep(20);
		gpiod_set_value_cansleep(ili9881d->reset, 0);
		/*
		 * 50ms delay after reset-out, as per manufacturer initalization
		 * sequence.
		 */
		msleep(220);
	}

	mutex_unlock(&ili9881d->mutex);
	return 0;
}

static int ili9881d_enable(struct drm_panel *panel)
{
	struct ili9881d *ili9881d = panel_to_ili9881d(panel);

	mutex_lock(&ili9881d->mutex);
	dev_info(ili9881d->panel.dev, "%s %d", __func__, __LINE__);
	ili9881d_init_sequence(ili9881d);
	msleep(220);
	mipi_dsi_dcs_set_display_on(ili9881d->dsi);
	mutex_unlock(&ili9881d->mutex);
	return 0;
}

static int ili9881d_disable(struct drm_panel *panel)
{
	struct ili9881d *ili9881d = panel_to_ili9881d(panel);
	int ret;
	mutex_lock(&ili9881d->mutex);
	dev_info(ili9881d->panel.dev, "%s %d", __func__, __LINE__);

	ret=mipi_dsi_dcs_set_display_off(ili9881d->dsi);
	mutex_unlock(&ili9881d->mutex);

	return ret;
}

static int ili9881d_unprepare(struct drm_panel *panel)
{
	struct ili9881d *ili9881d = panel_to_ili9881d(panel);

	mutex_lock(&ili9881d->mutex);

	dev_info(ili9881d->panel.dev, "%s %d", __func__, __LINE__);
	mipi_dsi_dcs_enter_sleep_mode(ili9881d->dsi);
	if (ili9881d->reset) {
		gpiod_set_value_cansleep(ili9881d->reset, 1);
	}

	mutex_unlock(&ili9881d->mutex);

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

static int ili9881d_get_modes(struct drm_panel *panel,
			    struct drm_connector *connector)
{
	struct ili9881d *ili9881d = panel_to_ili9881d(panel);
	const struct drm_display_mode *desc_mode = ili9881d->desc->mode;
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, desc_mode);
	if (!mode) {
		dev_err(&ili9881d->dsi->dev, "failed to add mode %ux%u@%u\n",
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
	return 1;
}

static const struct drm_panel_funcs ili9881d_funcs = {
	.disable	= ili9881d_disable,
	.unprepare	= ili9881d_unprepare,
	.prepare	= ili9881d_prepare,
	.enable		= ili9881d_enable,
	.get_modes	= ili9881d_get_modes,
};

static const struct drm_display_mode etml050015dha_default_mode = {
	
	.clock		= 60000,

	.hdisplay	= 720,
	.hsync_start	= 720 + 100,
	.hsync_end	= 720 + 100 + 33,
	.htotal		= 720 + 100 + 33 + 100,

	.vdisplay	= 1280,
	.vsync_start	= 1280 + 20,
	.vsync_end	= 1280 + 20 + 2,
	.vtotal		= 1280 + 20 + 2+ 30,

	.width_mm	= 180,
	.height_mm	= 320,
	
	.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
};

static const struct ili9881d_panel_desc etml050015dha_desc = {
	.init = etml050015dha_init,
	.init_length = ARRAY_SIZE(etml050015dha_init),
	.mode = &etml050015dha_default_mode,
	.lanes = 4,
	.format = MIPI_DSI_FMT_RGB888,
	.flags = MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_VIDEO |
			   MIPI_DSI_CLOCK_NON_CONTINUOUS | MIPI_DSI_MODE_VIDEO_SYNC_PULSE | MIPI_DSI_MODE_LPM, 
	.panel_sleep_delay = 1, /* panel need extra 80ms for sleep out cmd */
};

static int ili9881d_dsi_probe(struct mipi_dsi_device *dsi)
{
	const struct ili9881d_panel_desc *desc;
	struct ili9881d *ili9881d;
	int ret;

	ili9881d = devm_kzalloc(&dsi->dev, sizeof(*ili9881d), GFP_KERNEL);
	if (!ili9881d)
		return -ENOMEM;

	desc = of_device_get_match_data(&dsi->dev);
	dsi->mode_flags = desc->flags;
	dsi->format = desc->format;
	dsi->lanes = desc->lanes;

	dsi->lanes = 4;

	mutex_init(&ili9881d->mutex);
	ili9881d->reset = devm_gpiod_get_optional(&dsi->dev, "reset",
					       GPIOD_OUT_LOW |
					       GPIOD_FLAGS_BIT_NONEXCLUSIVE);
	if (IS_ERR(ili9881d->reset)) {
		ret = PTR_ERR(ili9881d->reset);
		dev_err(&dsi->dev, "Failed to get reset gpio (%d)\n", ret);
		return ret;
	}
	if (ili9881d->reset) {
		gpiod_set_value_cansleep(ili9881d->reset, 0);
		msleep(1000);
		gpiod_set_value_cansleep(ili9881d->reset, 1);
		dev_info(&dsi->dev, "GPIO SET (%d)\n", ret);
	}	

	drm_panel_init(&ili9881d->panel, &dsi->dev, &ili9881d_funcs,
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
	ili9881d->sleep_delay = 120 + desc->panel_sleep_delay;

	ret = drm_panel_of_backlight(&ili9881d->panel);
	if (ret)
		return ret;

	drm_panel_add(&ili9881d->panel);

	mipi_dsi_set_drvdata(dsi, ili9881d);
	ili9881d->dsi = dsi;
	ili9881d->desc = desc;

	dev_info(ili9881d->panel.dev, "%s %d\n", __func__, __LINE__);
	return mipi_dsi_attach(dsi);
}

static int ili9881d_dsi_remove(struct mipi_dsi_device *dsi)
{
	struct ili9881d *ili9881d = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ili9881d->panel);

	return 0;
}

static const struct of_device_id ili9881d_of_match[] = {
	{ .compatible = "surelink,etml050015dha", .data = &etml050015dha_desc },
	{ }
};

MODULE_DEVICE_TABLE(of, ili9881d_of_match);

static struct mipi_dsi_driver ili9881d_dsi_driver = {
	.probe		= ili9881d_dsi_probe,
	.remove		= ili9881d_dsi_remove,
	.driver = {
		.name		= "ili9881d-dsi",
		.of_match_table	= ili9881d_of_match,
	},
};
module_mipi_dsi_driver(ili9881d_dsi_driver);

MODULE_AUTHOR("Jagan Teki <jagan@amarulasolutions.com>");
MODULE_DESCRIPTION("Sitronix ST7701 LCD Panel Driver");
MODULE_LICENSE("GPL");
